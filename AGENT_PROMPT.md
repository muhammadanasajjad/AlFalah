# Agent Prompt: Quran Page-Specific Font System

## Goal

Build a FontManager + CSV-driven page system so each Quran ayah renders with the correct page-specific Uthmani font (`p{N}.ttf`), loaded lazily as the user scrolls.

## Current State (HEAD = `feefe88` "image fix")

The repo is at a clean starting point with:

- `app/src/main/cpp/native-lib.cpp` — Arabic text renders using hardcoded fontId 1 (`g_arabicFontAtlas` loaded from `p1.ttf`). All 114 surah JSONs parsed by `QuranDatabase`. Only surah 1 is displayed.
- `app/src/main/cpp/engine/renderer/ClayRenderer.hpp/.cpp` — Fixed array `FontAtlas* fontAtlases[4]` (`kMaxFontAtlases = 4`), RTL hardcoded as `fid == 1`.
- `app/src/main/cpp/quran/QuranDatabase.hpp/.cpp` — Does NOT have `pageNumber` on `Word`, no CSV parsing.
- `app/src/main/cpp/engine/renderer/FontAtlas.hpp/.cpp` — Single font loader via FreeType + HarfBuzz. Has `Load(AAssetManager*, path, size)` for assets and `LoadFromFile(path, size)` for files.
- `app/src/main/cpp/engine/renderer/ImageLoader.hpp/.cpp` — PNG → GL texture, static cache, `Destroy()` at start of `Init()` (already fixed in HEAD).

## Assets Available

- `assets/fonts/quranicFonts/qpc/p1.ttf` through `p604.ttf` — page-specific Uthmani fonts
- `assets/quran/surahsQPC/qpc-v2-15-lines.csv` — maps word_id ranges to page_number (columns: `page_number`, `line_number`, `line_type`, `is_centered`, `first_word_id`, `last_word_id`, `surah_number`)
- `assets/quran/surahsQPC/surah_001.json` through `surah_114.json` — Quran word data, each entry has `id` (global monotonic), `ayah` (string), `word` (string), `text` (string)

## What Was Reverted

This is what we implemented and then reverted. The full diff is below.

### Files Created
- `engine/FontManager.hpp` / `.cpp` — string-keyed font registry, `LoadSystem(name, size, rtl)`, `LoadAsset(key, size, rtl)`, `Register(fontId, renderer)`

### Files Modified
- `native-lib.cpp` — Replaced `g_fontAtlas` / `g_arabicFontAtlas` globals with `g_fontManager`. `MeasureText` uses `IsRtl(fontId)`. Added `EnsurePageFont()`, `g_arabicFallbackFontId`, `g_lastScrollOffset`. `LayoutQuranPage` uses scroll-position-based progressive font loading.
- `ClayRenderer.hpp` / `.cpp` — `fontAtlases` changed from `FontAtlas*[4]` to `unordered_map<uint16_t, FontAtlas*>`. Added `fontIsRtl` parallel map. `SetFontAtlas(uint16_t, FontAtlas*, bool)` overload. `DrawText` looks up maps.
- `QuranDatabase.hpp` / `.cpp` — `Word::pageNumber`, `mWordToPage` map, `BuildPageMap()` CSV parser.

### Known Issues That Were NOT Fixed
- Only surah 4 was loaded (changed from surah 1 during testing)
- The renderer showed "tofu" (□ boxes) for Arabic when the page font wasn't loaded yet, because fontId 0 (Roboto) was used as fallback.

## Key Constraints & Gotchas

1. **C++20, Android NDK, OpenGL ES 3.0**. Build via `./gradlew assembleDebug`. CMake uses `file(GLOB_RECURSE SOURCES .../*.cpp)` — new `.cpp` files are auto-picked up but may require `touch CMakeLists.txt` if cache is stale.

2. **Clay 0.14** — `Clay_GetScrollOffset()` only works when called INSIDE a scroll container's `CLAY(...)` block (during designated initializer evaluation). It finds the offset by matching the currently-open layout element. You CAN capture it via the comma operator:
   ```cpp
   .childOffset = (g_lastScrollOffset = Clay_GetScrollOffset())
   ```

3. **picojson** — Quran JSON fields `ayah` and `word` are strings (quoted `"1"`), NOT numbers. Use `get<std::string>()` + `std::stoi()`. The `id` field IS a number, use `get<double>()`.

4. **FontAtlas ownership** — FontAtlas is not movable (stores `this` pointer in HarfBuzz). Must be heap-allocated (`new`/`delete`). The FontManager owns all FontAtlas instances and destroys them in `Destroy()`.

5. **Font loading is synchronous and slow** (~50-100ms per font from assets). Loading 30 fonts blocks for 1.5+ seconds. Must be progressive (load 3-5 per frame).

6. **AAssetManager*** — Only valid during `nativeOnSurfaceCreated`. Store it in FontManager for lazy font loading during layout.

7. **Lint/typecheck** — No linter or typecheck step provided. Just build with `./gradlew assembleDebug`.

## Architecture Requirements

### FontManager
```
FontManager
  Init(AAssetManager*)
  LoadSystem(name, size, rtl) → fontId   // tries kFontPaths, then assets/ as fallback
  LoadAsset(key, size, rtl) → fontId     // key = path relative to assets/fonts/, e.g. "quranicFonts/qpc/p1.ttf"
  GetFontId(key) → fontId or kInvalidFontId
  GetAtlas(fontId) → FontAtlas*
  IsRtl(fontId) → bool
  Register(fontId, ClayRenderer&)        // adds atlas + RTL flag to renderer
  RegisterAll(ClayRenderer&)
  Destroy()                              // deletes all atlases
```

### ClayRenderer Changes
- Replace `FontAtlas* fontAtlases[4]` with `unordered_map<uint16_t, FontAtlas*> fontAtlases`
- Add `unordered_map<uint16_t, bool> fontIsRtl` parallel map
- `SetFontAtlas(uint16_t fontId, FontAtlas* atlas, bool isRtl)` — stores in both maps
- `DrawText()` — lookup atlas by fontId in map, fallback to fontId 0 entry if not found; RTL from `fontIsRtl` map

### QuranDatabase Changes
- Add `int32_t pageNumber = 0` to `Word`
- Add `unordered_map<int32_t, int32_t> mWordToPage` (word_id → page_number)
- `BuildPageMap(mgr)` — parse CSV, for each `ayah` row add entries for all word IDs in [first, last] → page_number
- Call `BuildPageMap()` before loading surahs in `Load()`
- During `LoadSurah()`, for each word, look up `mWordToPage[wordId]` and set `w.pageNumber`

### LayoutQuranPage Approach (use surah 1 for now, not 4)
```
sLoaded block:
  - Build ayahLines, ayahStrings from surah data
  - Store ayahPageNumbers per ayah
  - Initialize ayahFontIds with kInvalidFontId
  - Collect all unique page numbers
  - Load first 3 pages synchronously (so initial view is correct)
  - Queue remaining for progressive loading

Each frame before CLAY layout:
  - Progressively load 5 pending page fonts via EnsurePageFont()
  - Update ayahFontIds entries for each loaded page

In the layout loop:
  - For each ayah, use ayahFontIds[i] if loaded, else use a reliable Arabic fallback font (load p1.ttf at startup for this purpose)
  - NEVER use fontId 0 (Roboto) as fallback for Arabic text — it shows □ boxes
```

### Arabic Fallback Requirement
Load `p1.ttf` at startup as a dedicated Arabic fallback font (separate fontId from Roboto). Use this as the fallback for any ayah whose page font isn't loaded yet. This ensures Arabic characters always render correctly, even during the progressive loading window.

## File List to Create/Modify

| File | Action |
|---|---|
| `engine/FontManager.hpp` | Create |
| `engine/FontManager.cpp` | Create |
| `engine/renderer/ClayRenderer.hpp` | Modify |
| `engine/renderer/ClayRenderer.cpp` | Modify |
| `quran/QuranDatabase.hpp` | Modify |
| `quran/QuranDatabase.cpp` | Modify |
| `native-lib.cpp` | Modify |

## Build & Verify

```bash
touch app/src/main/cpp/CMakeLists.txt
./gradlew assembleDebug
```
