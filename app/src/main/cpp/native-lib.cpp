#include <jni.h>
#include <android/log.h>
#include <android/asset_manager_jni.h>
#include <malloc.h>
#include <vector>
#include <string>
#include <cstring>
#include <unordered_set>
#include "engine/renderer/Renderer.hpp"
#include "engine/renderer/ClayRenderer.hpp"
#include "engine/renderer/FontAtlas.hpp"
#include "engine/renderer/ImageLoader.hpp"
#include "engine/FontManager.hpp"
#include "quran/QuranDatabase.hpp"
#include "third_party/clay/clay.h"

#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "NATIVE", __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  "NATIVE", __VA_ARGS__)

static ClayRenderer g_clayRenderer;
static FontManager g_fontManager;
static uint16_t g_arabicFallbackFontId = FontManager::kInvalidFontId;
static uint64_t g_clayArenaSize = 0;
static void* g_clayArena = nullptr;
static float g_density = 1.0f;
static float g_statusBarHeightDp = 0;
static Clay_Color bg = { 19, 19, 10, 255 };
static Clay_Color bg1 = { 31, 31, 22, 255 };
static Clay_Color accent = { 0, 128, 66, 255 };
static Clay_Color accent1 = { 2, 100, 53, 255 };
static Clay_Color fg = { 255, 255, 255, 255 };

enum class Page { Home, Quran, SurahSelection };
static Page g_currentPage = Page::Home;
static int g_selectedSurah = 1;

static Clay_Vector2 g_pointerPos = {0, 0};
static Clay_Vector2 g_pointerDownPos = {0, 0};
static bool g_pointerDown = false;
static bool g_wasPointerDown = false;
static bool g_pointerMovedSignificantly = false;
static float g_deltaTime = 0.016f;
static const float TAP_MOVE_THRESHOLD = 15.0f;
static QuranDatabase g_quranDb;
static int g_surfaceVersion = 0;

static Clay_Dimensions MeasureText(Clay_StringSlice text,
                                    Clay_TextElementConfig* config,
                                    void*)
{
    if (!config) return { 0, 0 };

    FontAtlas* atlas = g_fontManager.GetAtlas(config->fontId);
    if (!atlas) return { 0, 0 };

    bool isRtl = g_fontManager.IsRtl(config->fontId);
    hb_direction_t dir = isRtl ? HB_DIRECTION_RTL : HB_DIRECTION_LTR;
    hb_script_t script = isRtl ? HB_SCRIPT_ARABIC : HB_SCRIPT_LATIN;
    const char* lang = isRtl ? "ar" : "en";

    atlas->SetSize((float)config->fontSize * g_density);

    hb_font_t* hbFont = atlas->GetHbFont();
    if (!hbFont) return { 0, 0 };

    hb_buffer_t* buf = hb_buffer_create();
    hb_buffer_add_utf8(buf, text.chars, text.length, 0, text.length);
    hb_buffer_set_direction(buf, dir);
    hb_buffer_set_script(buf, script);
    hb_buffer_set_language(buf, hb_language_from_string(lang, 2));
    hb_shape(hbFont, buf, nullptr, 0);

    unsigned int count = 0;
    hb_glyph_info_t* info = hb_buffer_get_glyph_infos(buf, &count);
    hb_glyph_position_t* pos = hb_buffer_get_glyph_positions(buf, &count);

    float total = 0;
    for (unsigned int i = 0; i < count; ++i)
        total += (float)pos[i].x_advance / 64.0f;

    hb_buffer_destroy(buf);

    return { total / g_density, atlas->GetLineHeight() / g_density };
}

extern "C" JNIEXPORT void JNICALL
Java_com_primaveradev_alfalah_MainActivity_nativeOnSurfaceCreated(
        JNIEnv* env, jobject, jobject assetManager, jfloat density)
{
    g_density = density;
    ++g_surfaceVersion;

    AAssetManager* mgr = AAssetManager_fromJava(env, assetManager);

//    g_renderer.SetAssetManager(mgr);
//    g_renderer.Init();

    g_clayRenderer.Init(mgr);
    g_clayRenderer.SetDensity(g_density);

    ImageLoader::Init(mgr);

    g_fontManager.Init(mgr);

    // Load system font for UI text
    uint16_t uiFontId = g_fontManager.LoadSystem("Roboto", 16.0f * g_density, false);
    if (uiFontId != FontManager::kInvalidFontId) {
        g_fontManager.Register(uiFontId, g_clayRenderer);
        LOGI("loaded UI font (fontId=%u)", uiFontId);
    } else {
        LOGE("ALL FONT LOADING FAILED — no text will render");
    }

    // Load Arabic fallback font (p1.ttf) for Quran text
    g_arabicFallbackFontId = g_fontManager.LoadAsset(
        "fonts/quranicFonts/qpc/p1.ttf", 16.0f * g_density, true);
    if (g_arabicFallbackFontId != FontManager::kInvalidFontId) {
        g_fontManager.Register(g_arabicFallbackFontId, g_clayRenderer);
        LOGI("loaded Arabic fallback font (fontId=%u)", g_arabicFallbackFontId);
    }

    g_quranDb.Load(mgr);

    if (!g_clayArena) {
        g_clayArenaSize = Clay_MinMemorySize();
        g_clayArena = malloc(g_clayArenaSize);
        Clay_Initialize(
            Clay_CreateArenaWithCapacityAndMemory(g_clayArenaSize, g_clayArena),
            (Clay_Dimensions){ 0, 0 },
            (Clay_ErrorHandler){ 0 });
    }
    Clay_SetMeasureTextFunction(MeasureText, nullptr);
}

extern "C" JNIEXPORT void JNICALL
Java_com_primaveradev_alfalah_MainActivity_nativeOnSurfaceChanged(
        JNIEnv*, jobject, jint w, jint h)
{
//    g_renderer.Resize(w, h);
    g_clayRenderer.SetResolution(w, h);
    Clay_SetLayoutDimensions((Clay_Dimensions){ (float)w / g_density, (float)h / g_density });
}

extern "C" JNIEXPORT void JNICALL
Java_com_primaveradev_alfalah_MainActivity_nativeSetStatusBarHeight(
        JNIEnv*, jobject, jfloat heightDp)
{
    g_statusBarHeightDp = heightDp;
}

extern "C" JNIEXPORT void JNICALL
Java_com_primaveradev_alfalah_MainActivity_nativeOnTouch(
        JNIEnv*, jobject, jfloat x, jfloat y, jboolean down)
{
    g_pointerPos.x = x / g_density;
    g_pointerPos.y = y / g_density;

    if (down && !g_pointerDown) {
        g_pointerDownPos = g_pointerPos;
        g_pointerMovedSignificantly = false;
    }
    g_pointerDown = down;
}

static void LayoutHomePage()
{
    CLAY(
        CLAY_ID("Root"),
        {
            .layout = {
                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                .padding = {
                        .left = 24,
                        .right = 24,
                        .top = (uint16_t)(24 + (int)g_statusBarHeightDp),
                        .bottom = 24
                },
                .childGap = 24,
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
            .backgroundColor = bg,
        }
    ) {
        CLAY(
            CLAY_ID("TopRowContainer"),
            {
                .layout = {
                    .sizing = { CLAY_SIZING_GROW(0) },
                    .childGap = 24,
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                }
            }
        ) {
            CLAY(
                CLAY_ID("QuranButtonContainer"),
                {
                    .layout = {
                        .sizing = { CLAY_SIZING_GROW(1), CLAY_SIZING_FIXED(250) },
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    },
                    .backgroundColor = bg1,
                    .cornerRadius = {24, 24, 24, 24},
                    .clip = { .horizontal = true, .vertical = true },
                }
            ) {
                CLAY(
                    CLAY_ID("QuranText"),
                    {
                        .layout = {
                            .padding = { .left = 24, .top = 24 },
                            .childGap = 12,
                            .layoutDirection = CLAY_TOP_TO_BOTTOM,
                        }
                    }
                ) {
                    CLAY_TEXT(
                        CLAY_STRING("Qur'an"),
                        CLAY_TEXT_CONFIG({
                             .textColor = fg,
                             .fontId = 0,
                             .fontSize = 24,
                             .wrapMode = CLAY_TEXT_WRAP_WORDS,
                        })
                    );

                    CLAY_TEXT(
                        CLAY_STRING("Read and explore\nthe Qur'an"),
                        CLAY_TEXT_CONFIG({
                             .textColor = fg,
                             .fontId = 0,
                             .fontSize = 12,
                             .wrapMode = CLAY_TEXT_WRAP_WORDS,
                        })
                    );
                }

                CLAY(
                    CLAY_ID("ImageRow"),
                    {
                        .layout = {
                            .sizing = {
                                CLAY_SIZING_GROW(1),
                                CLAY_SIZING_FIXED(70)
                            },
                            .padding = {.left = 8},
                            .layoutDirection = CLAY_LEFT_TO_RIGHT,
                        }
                    }
                ) {
                    CLAY(
                        CLAY_ID("QuranImage"),
                        {
                            .layout = {
                                .sizing = {
                                    CLAY_SIZING_PERCENT(1.20),
                                }
                            },
                            .cornerRadius = {8,8,8,8},
                            .image = {
                                .imageData = ImageLoader::Get("images/quran.png")
                            }
                        }
                    ) {}
                }
            }

            CLAY(
                CLAY_ID("TasbihAlbumsContainer"),
                {
                    .layout = {
                        .sizing = { CLAY_SIZING_GROW(1), CLAY_SIZING_GROW(0) },
                        .childGap = 24,
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    },
                }
            ) {
                CLAY(
                    CLAY_ID("TasbihContainer"),
                    {
                        .layout = {
                            .sizing = { CLAY_SIZING_GROW(1), CLAY_SIZING_GROW(1) },
                            .layoutDirection = CLAY_TOP_TO_BOTTOM,
                        },
                        .backgroundColor = bg1,
                        .cornerRadius = {24, 24, 24, 24},
                        .clip = { .horizontal = true, .vertical = true }
                    },
                ) {
                    CLAY(
                        CLAY_ID("TasbihText"),
                        {
                            .layout = {
                                    .padding = {.left = 24, .top = 24}
                            }
                        }
                    ) {
                        CLAY_TEXT(
                            CLAY_STRING("Tasbih"),
                            CLAY_TEXT_CONFIG({
                                 .textColor = fg,
                                 .fontId = 0,
                                 .fontSize = 24,
                                 .wrapMode = CLAY_TEXT_WRAP_WORDS,
                             })
                        );
                    }
                    CLAY(
                        CLAY_ID("TasbihImageRow"),
                        {
                            .layout = {
                                .sizing = {
                                    CLAY_SIZING_GROW(1),
                                    CLAY_SIZING_FIXED(60)
                                },
                                .padding = {.left = 20, .right = 10, .top = 15},
                                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                            },
                        }
                    ) {
                        CLAY(
                            CLAY_ID("TasbihImage"),
                            {
                                .layout = {
                                    .sizing = {
                                        CLAY_SIZING_GROW(1),
                                    }
                                },
                                .image = {
                                    .imageData = ImageLoader::Get("images/tasbih.png")
                                }
                            }
                        ) {}
                    }
                }

                CLAY(
                    CLAY_ID("AlbumsContainer"),
                    {
                        .layout = {
                            .sizing = { CLAY_SIZING_GROW(1), CLAY_SIZING_GROW(1) },
                            .layoutDirection = CLAY_TOP_TO_BOTTOM,
                        },
                        .backgroundColor = bg1,
                        .cornerRadius = {24, 24, 24, 24},
                        .clip = { .horizontal = true, .vertical = true }
                    },
                ) {
                    CLAY(
                        CLAY_ID("AlbumsText"),
                        {
                            .layout = {
                                .padding = {.left = 24, .top = 24}
                            }
                        }
                    ) {
                        CLAY_TEXT(
                            CLAY_STRING("Albums"),
                            CLAY_TEXT_CONFIG({
                                 .textColor = fg,
                                 .fontId = 0,
                                 .fontSize = 24,
                                 .wrapMode = CLAY_TEXT_WRAP_WORDS,
                             })
                        );
                    }
                    CLAY(
                        CLAY_ID("AlbumsImageRow"),
                        {
                            .layout = {
                                .sizing = {
                                    CLAY_SIZING_GROW(1),
                                    CLAY_SIZING_FIXED(60)
                                },
                                .padding = {.left = 20, .right = 10, .top = 15},
                                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                            },
                        }
                    ) {
                        CLAY(
                            CLAY_ID("AlbumsImage"),
                            {
                                .layout = {
                                    .sizing = {
                                        CLAY_SIZING_GROW(1),
                                    }
                                },
                                .image = {
                                    .imageData = ImageLoader::Get("images/tasbih.png")
                                }
                            }
                        ) {}
                    }
                }
            }
        }

        CLAY(
            CLAY_ID("TimesStatsContainer"),
            {
                .layout = {
                    .sizing = { CLAY_SIZING_GROW(1), CLAY_SIZING_FIXED((250 - 24) / 2) },
                    .childGap = 24,
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                },
            }
        ) {
            CLAY(
                CLAY_ID("TimesContainer"),
                {
                    .layout = {
                        .sizing = { CLAY_SIZING_GROW(1), CLAY_SIZING_GROW(1) },
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    },
                    .backgroundColor = bg1,
                    .cornerRadius = {24, 24, 24, 24},
                    .clip = { .horizontal = true, .vertical = true }
                },
            ) {
                CLAY(
                    CLAY_ID("TimesText"),
                    {
                        .layout = {
                            .padding = {.left = 24, .top = 24}
                        }
                    }
                ) {
                    CLAY_TEXT(
                        CLAY_STRING("Times"),
                        CLAY_TEXT_CONFIG({
                             .textColor = fg,
                             .fontId = 0,
                             .fontSize = 24,
                             .wrapMode = CLAY_TEXT_WRAP_WORDS,
                         })
                    );
                }
                CLAY(
                    CLAY_ID("TimesImageRow"),
                    {
                        .layout = {
                            .sizing = {
                                CLAY_SIZING_GROW(1),
                                CLAY_SIZING_FIXED(60)
                            },
                            .padding = {.left = 20, .right = 10, .top = 15},
                            .layoutDirection = CLAY_TOP_TO_BOTTOM,
                        },
                    }
                ) {
                    CLAY(
                        CLAY_ID("TimesImage"),
                        {
                            .layout = {
                                .sizing = {
                                        CLAY_SIZING_GROW(1),
                                }
                            },
                            .image = {
                                .imageData = ImageLoader::Get("images/tasbih.png")
                            }
                        }
                    ) {}
                }
            }

            CLAY(
                CLAY_ID("StatsContainer"),
                {
                    .layout = {
                        .sizing = { CLAY_SIZING_GROW(1), CLAY_SIZING_GROW(1) },
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    },
                    .backgroundColor = bg1,
                    .cornerRadius = {24, 24, 24, 24},
                    .clip = { .horizontal = true, .vertical = true }
                },
            ) {
                CLAY(
                    CLAY_ID("StatsText"),
                    {
                        .layout = {
                            .padding = {.left = 24, .top = 24}
                        }
                    }
                ) {
                    CLAY_TEXT(
                        CLAY_STRING("Stats"),
                        CLAY_TEXT_CONFIG({
                             .textColor = fg,
                             .fontId = 0,
                             .fontSize = 24,
                             .wrapMode = CLAY_TEXT_WRAP_WORDS,
                         })
                    );
                }
                CLAY(
                    CLAY_ID("StatsImageRow"),
                    {
                        .layout = {
                            .sizing = {
                                CLAY_SIZING_GROW(1),
                                CLAY_SIZING_FIXED(60)
                            },
                            .padding = {.left = 20, .right = 10, .top = 15},
                            .layoutDirection = CLAY_TOP_TO_BOTTOM,
                        },
                    }
                ) {
                    CLAY(
                        CLAY_ID("StatsImage"),
                        {
                            .layout = {
                                .sizing = {
                                    CLAY_SIZING_GROW(1),
                                }
                            },
                            .image = {
                                .imageData = ImageLoader::Get("images/tasbih.png")
                            }
                        }
                    ) {}
                }
            }
        }
    }
}

static void LayoutSurahSelectionPage()
{
    static std::vector<std::string> rowLabels;
    static std::vector<std::string> pageTexts;
    static std::vector<Clay_String> rowLabelStrings;
    static std::vector<Clay_String> pageTextStrings;
    static bool sBuilt = false;
    static int sLastVersion = 0;

    if (sLastVersion != g_surfaceVersion) {
        sBuilt = false;
        sLastVersion = g_surfaceVersion;
    }

    if (!sBuilt) {
        rowLabels.resize(114);
        pageTexts.resize(114);
        rowLabelStrings.resize(114);
        pageTextStrings.resize(114);
        for (int i = 0; i < 114; ++i) {
            const SurahInfo& info = g_quranDb.GetSurahInfo(i + 1);
            rowLabels[i] = std::to_string(i + 1) + ". " + info.nameSimple;
            if (info.startPage == info.endPage) {
                pageTexts[i] = "Pg " + std::to_string(info.startPage);
            } else {
                pageTexts[i] = "Pg " + std::to_string(info.startPage) + "-" + std::to_string(info.endPage);
            }
            rowLabelStrings[i] = {
                .isStaticallyAllocated = false,
                .length = (int)rowLabels[i].length(),
                .chars = rowLabels[i].c_str()
            };
            pageTextStrings[i] = {
                .isStaticallyAllocated = false,
                .length = (int)pageTexts[i].length(),
                .chars = pageTexts[i].c_str()
            };
        }
        sBuilt = true;
    }

    Clay_Color rowBg = { 31, 31, 22, 255 };
    Clay_Color pageBg = { 45, 45, 32, 255 };

    CLAY(
        CLAY_ID("SurahSelectionRoot"),
        {
            .layout = {
                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                .padding = {
                    .left = 16,
                    .right = 16,
                    .top = (uint16_t)(16 + (int)g_statusBarHeightDp),
                    .bottom = 16
                },
                .childGap = 12,
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
            .backgroundColor = bg,
        }
    ) {
        CLAY(
            CLAY_ID("SurahSelectionTopBar"),
            {
                .layout = {
                    .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(48) },
                    .childGap = 12,
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                }
            }
        ) {
            CLAY(
                CLAY_ID("SurahBackButton"),
                {
                    .layout = {
                        .sizing = { CLAY_SIZING_FIXED(100), CLAY_SIZING_FIXED(48) },
                    },
                    .backgroundColor = bg1,
                    .cornerRadius = {12, 12, 12, 12},
                }
            ) {
                CLAY_TEXT(
                    CLAY_STRING("<- Back"),
                    CLAY_TEXT_CONFIG({
                        .textColor = fg,
                        .fontId = 0,
                        .fontSize = 18,
                        .wrapMode = CLAY_TEXT_WRAP_WORDS,
                    })
                );
            }

            CLAY_TEXT(
                CLAY_STRING("Select Surah"),
                CLAY_TEXT_CONFIG({
                    .textColor = fg,
                    .fontId = 0,
                    .fontSize = 22,
                    .wrapMode = CLAY_TEXT_WRAP_WORDS,
                })
            );
        }

        CLAY(
            CLAY_ID("SurahListScroll"),
            {
                .layout = {
                    .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                    .padding = CLAY_PADDING_ALL(8),
                    .childGap = 2,
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                },
                .backgroundColor = bg1,
                .cornerRadius = {24, 24, 24, 24},
                .clip = {
                    .vertical = true,
                    .childOffset = Clay_GetScrollOffset()
                },
            }
        ) {
            for (int i = 0; i < 114; ++i) {
                const SurahInfo& info = g_quranDb.GetSurahInfo(i + 1);
                Clay_String arabicStr = {
                    .isStaticallyAllocated = false,
                    .length = (int)info.nameArabic.length(),
                    .chars = info.nameArabic.c_str()
                };

                CLAY(
                    CLAY_IDI("SurahRow", i),
                    {
                        .layout = {
                            .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(52) },
                            .padding = { .left = 12, .right = 8 },
                            .childGap = 8,
                            .layoutDirection = CLAY_LEFT_TO_RIGHT,
                        },
                        .backgroundColor = rowBg,
                        .cornerRadius = {10, 10, 10, 10},
                    }
                ) {
                    CLAY_TEXT(
                        rowLabelStrings[i],
                        CLAY_TEXT_CONFIG({
                            .textColor = fg,
                            .fontId = 0,
                            .fontSize = 15,
                            .wrapMode = CLAY_TEXT_WRAP_WORDS,
                        })
                    );
                }
            }
        }
    }
}

static void LayoutQuranPage()
{
    static bool sLoaded = false;
    static std::vector<std::string> ayahLines;
    static std::vector<Clay_String> ayahStrings;
    static std::vector<uint16_t> ayahFontIds;
    static std::vector<int32_t> pendingPages;
    static int pendingIdx = 0;
    static int totalAyahs = 0;
    static int lastVersion = 0;
    static int s_lastSurah = 0;

    if (lastVersion != g_surfaceVersion || s_lastSurah != g_selectedSurah) {
        sLoaded = false;
        ayahLines.clear();
        ayahStrings.clear();
        ayahFontIds.clear();
        pendingPages.clear();
        pendingIdx = 0;
        totalAyahs = 0;
        lastVersion = g_surfaceVersion;
        s_lastSurah = g_selectedSurah;
    }

    if (!sLoaded) {
        const Surah& surah = g_quranDb.GetSurah(g_selectedSurah);
        totalAyahs = (int)surah.ayahs.size();
        ayahLines.reserve(totalAyahs);
        ayahStrings.reserve(totalAyahs);

        std::unordered_set<int32_t> uniquePages;
        std::vector<int32_t> ayahPageNumbers;
        ayahPageNumbers.reserve(totalAyahs);

        for (const auto& ayah : surah.ayahs) {
            std::string line;
            int32_t page = 0;
            for (const auto& w : ayah.words) {
                if (!line.empty()) line += ' ';
                line += w.text;
                LOGI("%s", w.text.c_str());
                LOGI("%d", w.pageNumber);
                if (w.pageNumber > 0) page = w.pageNumber;
            }
            ayahLines.push_back(std::move(line));
            ayahPageNumbers.push_back(page);
            if (page > 0) uniquePages.insert(page);
        }

        for (auto& s : ayahLines) {
            LOGI("%s", s.c_str());
            ayahStrings.push_back({
                .isStaticallyAllocated = false,
                .length = (int)s.length(),
                .chars = s.c_str()
            });
        }

        ayahFontIds.assign(totalAyahs, FontManager::kInvalidFontId);
        pendingPages.clear();

        // Create fontIds for each unique page (lazy, not loaded yet)
        for (int32_t page : uniquePages) {
            char key[64];
            snprintf(key, sizeof(key), "fonts/quranicFonts/qpc/p%d.ttf", page);
            uint16_t fid = g_fontManager.GetOrCreateFontId(key, 16.0f * g_density, true);
            g_fontManager.Register(fid, g_clayRenderer);
            for (int i = 0; i < totalAyahs; ++i) {
                if (ayahPageNumbers[i] == page) {
                    ayahFontIds[i] = fid;
                }
            }
            pendingPages.push_back(page);
        }

        // Load first 3 page fonts synchronously
        int toLoad = std::min(3, (int)pendingPages.size());
        for (int i = 0; i < toLoad; ++i) {
            char key[64];
            snprintf(key, sizeof(key), "fonts/quranicFonts/qpc/p%d.ttf", pendingPages[i]);
            uint16_t fid = g_fontManager.GetFontId(key);
            if (fid != FontManager::kInvalidFontId &&
                g_fontManager.EnsureFontLoaded(fid)) {
                g_fontManager.Register(fid, g_clayRenderer);
            }
        }
        pendingIdx = toLoad;

        sLoaded = true;
    }

    // Progressively load 5 pending page fonts per frame
    for (int i = 0; i < 5 && pendingIdx < (int)pendingPages.size(); ++i, ++pendingIdx) {
        char key[64];
        snprintf(key, sizeof(key), "fonts/quranicFonts/qpc/p%d.ttf", pendingPages[pendingIdx]);
        uint16_t fid = g_fontManager.GetFontId(key);
        if (fid != FontManager::kInvalidFontId &&
            g_fontManager.EnsureFontLoaded(fid)) {
            g_fontManager.Register(fid, g_clayRenderer);
        }
    }

    CLAY(
        CLAY_ID("Root"),
        {
            .layout = {
                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                .padding = {
                        .left = 24,
                        .right = 24,
                        .top = (uint16_t)(24 + (int)g_statusBarHeightDp),
                        .bottom = 24
                },
                .childGap = 24,
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
            .backgroundColor = bg,
        }
    ) {
        CLAY(
            CLAY_ID("QuranBackButton"),
            {
                .layout = {
                    .sizing = { CLAY_SIZING_FIXED(100), CLAY_SIZING_FIXED(48) },
                },
                .backgroundColor = bg1,
                .cornerRadius = {12, 12, 12, 12},
            }
        ) {
            CLAY_TEXT(
                CLAY_STRING("<- Back"),
                CLAY_TEXT_CONFIG({
                    .textColor = fg,
                    .fontId = 0,
                    .fontSize = 18,
                    .wrapMode = CLAY_TEXT_WRAP_WORDS,
                })
            );
        }

        CLAY(
            CLAY_ID("QuranScrollContainer"),
            {
                .layout = {
                    .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                    .padding = CLAY_PADDING_ALL(15),
                    .childGap = 16,
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                },
                .backgroundColor = bg1,
                .cornerRadius = {24, 24, 24, 24},
                .clip = {
                    .vertical = true,
                    .childOffset = Clay_GetScrollOffset()
                },
            }
        ) {
            CLAY_TEXT_CONTAINER(
                CLAY_ID("AyahContainer"),
                {
                    .layout = {
                        .sizing = { CLAY_SIZING_GROW(0) },
                        .childAlignment = { .x = CLAY_ALIGN_X_RIGHT },
                    }
                }
            ) {
                for (int i = 0; i < totalAyahs; ++i) {
                    // Use page-specific font if loaded, otherwise fall back to Arabic fallback
                    uint16_t fid = ayahFontIds[i];
                    if (fid == FontManager::kInvalidFontId || !g_fontManager.GetAtlas(fid)) {
                        fid = g_arabicFallbackFontId;
                    }

                    CLAY_TEXT_SPAN(
                        ayahStrings[i],
                        CLAY_TEXT_CONFIG({
                            .textColor = fg,
                            .fontId = fid,
                            .fontSize = 28,
                            .textAlignment = CLAY_TEXT_ALIGN_RIGHT,
                        })
                    );
                }
            }
        }
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_primaveradev_alfalah_MainActivity_nativeOnDrawFrame(
        JNIEnv*, jobject)
{
    Clay_SetPointerState(g_pointerPos, g_pointerDown);
    Clay_UpdateScrollContainers(true, (Clay_Vector2){0, 0}, g_deltaTime);

    Clay_BeginLayout();
    {
        switch (g_currentPage) {
            case Page::Home:
                LayoutHomePage();
                break;
            case Page::Quran:
                LayoutQuranPage();
                break;
            case Page::SurahSelection:
                LayoutSurahSelectionPage();
                break;
        }
    }
    Clay_RenderCommandArray commands = Clay_EndLayout(0);

    if (g_pointerDown) {
        float dx = g_pointerPos.x - g_pointerDownPos.x;
        float dy = g_pointerPos.y - g_pointerDownPos.y;
        if (dx * dx + dy * dy > TAP_MOVE_THRESHOLD * TAP_MOVE_THRESHOLD) {
            g_pointerMovedSignificantly = true;
        }
    }

    if (g_wasPointerDown && !g_pointerDown && !g_pointerMovedSignificantly) {
        switch (g_currentPage) {
            case Page::Home:
                if (Clay_PointerOver(CLAY_ID("QuranButtonContainer"))) {
                    g_currentPage = Page::SurahSelection;
                }
                break;
            case Page::Quran:
                if (Clay_PointerOver(CLAY_ID("QuranBackButton"))) {
                    g_currentPage = Page::SurahSelection;
                }
                break;
            case Page::SurahSelection:
                if (Clay_PointerOver(CLAY_ID("SurahBackButton"))) {
                    g_currentPage = Page::Home;
                }
                for (int i = 0; i < 114; ++i) {
                    if (Clay_PointerOver(CLAY_IDI("SurahRow", i))) {
                        g_selectedSurah = i + 1;
                        g_currentPage = Page::Quran;
                        break;
                    }
                }
                break;
        }
    }
    g_wasPointerDown = g_pointerDown;

    ImageLoader::FixAspectRatios(commands);
    g_clayRenderer.Render(commands);
}
