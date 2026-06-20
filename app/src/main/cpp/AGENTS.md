# AGENTS Context — AlFalah CPP

## Project Overview
Android app (OpenGL ES 3.0) rendering UI via **Clay 0.14** immediate-mode library.  
C++ native code lives in `app/src/main/cpp/`, bridged to Java/Kotlin via JNI.

## Build
- CMake + Gradle (`./gradlew assembleDebug`)
- C++20, depends on FreeType + HarfBuzz (vendored in `third_party/`)
- All `.cpp` files under `app/src/main/cpp/` are globbed automatically

## Key Files
| File | Role |
|---|---|
| `native-lib.cpp` | All layout, touch, page switching, scroll logic |
| `engine/renderer/Renderer.hpp` | OpenGL ES 3.0 renderer |
| `engine/renderer/ClayRenderer.hpp` | Clay → OpenGL render bridge |
| `third_party/clay/clay.h` | Clay 0.14 (single-header UI lib) |
| `MainActivity.java` | Android activity, GLSurfaceView, touch forwarding |
| `quran/QuranDatabase.hpp` / `.cpp` | Quran data: surahs, ayahs, words, surah metadata (names, pages) |

## Clay Version — 0.14 (critical details)

### Sizing
- `CLAY_SIZING_GROW(N)` where N is the **min** size; the **max** defaults to 0 in the compound literal.
- **BUT** during `Clay__ConfigureOpenElementPtr` (~L1936), if `minMax.max <= 0`, it's bumped to `CLAY__MAXFLOAT` → `GROW(0)` effectively means "grow from 0 up to infinity". This is why `CLAY_SIZING_GROW(0)` works on the cross-axis.
- The `Clay_SizingMinMax` union stores `min` and `max` as `float`. The `CLAY_SIZING_GROW(...)` macro uses designated initializer `{ .minMax = { __VA_ARGS__ } }` → unprovided fields are zero-initialized.

### Scroll Containers
- `.clip.horizontal` / `.clip.vertical` + `.childOffset = Clay_GetScrollOffset()` enables scroll.
- `Clay_GetScrollOffset()` returns a **reference** (pointer) to the internal `scrollContainerDatas[i].scrollPosition` which persists across frames.
- `Clay_UpdateScrollContainers()` must be called **after** `Clay_SetPointerState()` and **before** `Clay_BeginLayout()` each frame.
- The scroll container's `contentSize` is populated during `Clay_EndLayout` render-command generation (~L3133). `canScrollVertically` requires `contentSize.height > container dimensions.height`.

### Pointer / Hit-Testing
- `Clay_SetPointerState(pos, down)` resets `pointerOverIds` and re-populates it by DFS-traversing the **previous frame's** layout tree (stored in `layoutElementTreeRoots`).
- Bounding boxes are read from the **hashmap** (set during previous frame's `Clay_EndLayout` at ~L3091).
- Elements inside a clip container are only added to `pointerOverIds` if the pointer is also inside the **clip parent's** bounding box (checked via `layoutElementClipElementIds`).
- The scroll container itself has `clipElementId = 0` (no clip parent), so only its own bounding box matters for pointer detection.

### Memory Model
- `Clay__InitializeEphemeralMemory()` (called at `Clay_BeginLayout`) **reallocates** `layoutElements`, `layoutElementChildren`, `layoutElementTreeRoots`, etc. from the arena each frame.
- Hashmap entries store **pointers** into the layout elements array — these are updated each frame as elements are re-created.
- `scrollContainerDatas` is **persistent** memory (allocated once in `Clay__InitializePersistentMemory`). The `layoutElement` pointer in `scrollContainerData` is refreshed each frame during `Clay__ConfigureOpenElementPtr` (~L2171).

## Page System
```cpp
enum class Page { Home, Quran, SurahSelection };
static Page g_currentPage;
static int g_selectedSurah = 1; // 1-114
```
- `LayoutHomePage()` — dashboard with "Qur'an" card (→ SurahSelection).
- `LayoutQuranPage()` — scrollable Ayah list for `g_selectedSurah` with "← Back" button.
- `LayoutSurahSelectionPage()` — scrollable list of 114 surahs (English name + Arabic name + page badge). Uses `CLAY_IDI("SurahRow", i)` for per-row IDs. Tapping a row sets `g_selectedSurah = i+1` and navigates to `Page::Quran`.
- Navigation:
  ```
  Home → (tap "Qur'an") → SurahSelection → (tap surah) → Quran
                         ↑                                   │
                         └─────────── ("← Back") ←───────────┘
                               ("← Back" on SurahSelection) → Home
  ```
- Page switching on tap detected in `nativeOnDrawFrame` after `Clay_EndLayout`:
  ```cpp
  if (g_wasPointerDown && !g_pointerDown && !g_pointerMovedSignificantly) {
      // per-page Clay_PointerOver checks
  }
  ```
- `LayoutQuranPage` reloads ayah data when `g_selectedSurah` changes (via `s_lastSurah` static).

## Surah Metadata (QuranDatabase)
- `SurahInfo` struct: `nameSimple`, `nameArabic`, `startPage`, `endPage`, `verseCount`, `revelationPlace`.
- Loaded from `quran-metadata-surah-name.csv` (114 rows) and page CSV's `surah_name` lines.
- Access via `g_quranDb.GetSurahInfo(surahNumber)` / `GetSurahStartPage(surahNumber)`.

## Touch Handling
```
MainActivity.java (OnTouchListener)
  → nativeOnTouch(x, y, down) [JNI]
    → g_pointerPos, g_pointerDown, g_pointerDownPos, g_pointerMovedSignificantly
      → nativeOnDrawFrame:
          1. Clay_SetPointerState(g_pointerPos, g_pointerDown)
          2. Clay_UpdateScrollContainers(true, {0,0}, g_deltaTime)
          3. Clay_BeginLayout() / Layout / Clay_EndLayout()
          4. Movement check: if pointer moved >15dp from g_pointerDownPos → g_pointerMovedSignificantly=true
          5. Tap detection: g_wasPointerDown && !g_pointerDown && !g_pointerMovedSignificantly
```

### Tap Detection
A tap is only recognised when the finger moves <15dp while pressed. This prevents scroll gestures on the surah list (or any scroll container) from accidentally triggering navigation.

### **Critical: ACTION_MOVE must send `down=true`**
The Java touch handler MUST pass `true` for both `ACTION_DOWN` and `ACTION_MOVE`.  
If `ACTION_MOVE` sends `down=false`, Clay releases the pointer, `Clay_UpdateScrollContainers` sees `isPointerActive=false`, and drag scrolling immediately stops + momentum calculation triggers with delta zero.

## Clay_String Lifetime
Clay stores a **raw pointer** (`Clay_String.chars`) to string data — it does NOT copy.  
Strings passed via `CLAY_STRING()` must outlive the layout frame.  
The `LayoutQuranPage` pattern uses `static std::vector<std::string>` + `static std::vector<Clay_String>` built once to keep pointers valid across frames.

## Known Caveats
- `Clay_PointerOver()` reads from `pointerOverIds` populated by the **most recent** `Clay_SetPointerState` call — do not call it *after* `Clay_EndLayout` expecting fresh frame data (it uses previous-frame bounding boxes).
- `Clay_GetElementData(id)` returns the hashmap entry's bounding box from the **most recent** `Clay_EndLayout`, not from the current frame's layout.
- `CLAY_IDI(...)` (indexed ID) must be used when creating many elements with the same base ID (e.g., list rows).
