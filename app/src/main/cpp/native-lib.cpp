#include <jni.h>
#include <android/log.h>
#include <android/asset_manager_jni.h>
#include <malloc.h>
#include <vector>
#include <string>
#include <cstring>
#include <chrono>
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

static JavaVM* g_jvm = nullptr;

extern "C" jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    g_jvm = vm;
    return JNI_VERSION_1_6;
}

static void CallJavaPlayAyah(int surah, int ayah) {
    LOGI("CallJavaPlayAyah(%d, %d)", surah, ayah);
    if (!g_jvm) { LOGI("  g_jvm is null"); return; }
    JNIEnv* env;
    bool needDetach = false;
    jint ret = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (ret != JNI_OK) {
        g_jvm->AttachCurrentThread(&env, nullptr);
        needDetach = true;
        LOGI("  attached to JNI thread");
    }
    jclass clazz = env->FindClass("com/primaveradev/alfalah/MainActivity");
    if (clazz) {
        LOGI("  found MainActivity class");
        jmethodID method = env->GetStaticMethodID(clazz, "playAyah", "(II)V");
        if (method) {
            LOGI("  calling playAyah static method");
            env->CallStaticVoidMethod(clazz, method, surah, ayah);
            LOGI("  playAyah returned");
        } else {
            LOGI("  could not find playAyah method!");
        }
    } else {
        LOGI("  could not find MainActivity class!");
    }
    if (needDetach) {
        g_jvm->DetachCurrentThread();
    }
}

static int CallJavaGetReciterCount() {
    if (!g_jvm) return 0;
    JNIEnv* env;
    bool needDetach = false;
    jint ret = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (ret != JNI_OK) {
        g_jvm->AttachCurrentThread(&env, nullptr);
        needDetach = true;
    }
    jclass clazz = env->FindClass("com/primaveradev/alfalah/MainActivity");
    int count = 0;
    if (clazz) {
        jmethodID method = env->GetStaticMethodID(clazz, "getReciterCount", "()I");
        if (method) count = env->CallStaticIntMethod(clazz, method);
    }
    if (needDetach) g_jvm->DetachCurrentThread();
    return count;
}

static std::string CallJavaGetReciterName(int index) {
    if (!g_jvm) return "";
    JNIEnv* env;
    bool needDetach = false;
    jint ret = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (ret != JNI_OK) {
        g_jvm->AttachCurrentThread(&env, nullptr);
        needDetach = true;
    }
    jclass clazz = env->FindClass("com/primaveradev/alfalah/MainActivity");
    std::string name;
    if (clazz) {
        jmethodID method = env->GetStaticMethodID(clazz, "getReciterName", "(I)Ljava/lang/String;");
        if (method) {
            jstring jName = (jstring)env->CallStaticObjectMethod(clazz, method, index);
            if (jName) {
                const char* utf = env->GetStringUTFChars(jName, nullptr);
                if (utf) { name = utf; env->ReleaseStringUTFChars(jName, utf); }
                env->DeleteLocalRef(jName);
            }
        }
    }
    if (needDetach) g_jvm->DetachCurrentThread();
    return name;
}

static void CallJavaSetReciter(int index) {
    if (!g_jvm) return;
    JNIEnv* env;
    bool needDetach = false;
    jint ret = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (ret != JNI_OK) {
        g_jvm->AttachCurrentThread(&env, nullptr);
        needDetach = true;
    }
    jclass clazz = env->FindClass("com/primaveradev/alfalah/MainActivity");
    if (clazz) {
        jmethodID method = env->GetStaticMethodID(clazz, "setReciter", "(I)V");
        if (method) env->CallStaticVoidMethod(clazz, method, index);
    }
    if (needDetach) g_jvm->DetachCurrentThread();
}

static void CallJavaTogglePlayPause() {
    if (!g_jvm) return;
    JNIEnv* env;
    bool needDetach = false;
    jint ret = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (ret != JNI_OK) {
        g_jvm->AttachCurrentThread(&env, nullptr);
        needDetach = true;
    }
    jclass clazz = env->FindClass("com/primaveradev/alfalah/MainActivity");
    if (clazz) {
        jmethodID method = env->GetStaticMethodID(clazz, "togglePlayPause", "()V");
        if (method) env->CallStaticVoidMethod(clazz, method);
    }
    if (needDetach) g_jvm->DetachCurrentThread();
}

static void CallJavaStop() {
    if (!g_jvm) return;
    JNIEnv* env;
    bool needDetach = false;
    jint ret = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (ret != JNI_OK) {
        g_jvm->AttachCurrentThread(&env, nullptr);
        needDetach = true;
    }
    jclass clazz = env->FindClass("com/primaveradev/alfalah/MainActivity");
    if (clazz) {
        jmethodID method = env->GetStaticMethodID(clazz, "stopPlayback", "()V");
        if (method) env->CallStaticVoidMethod(clazz, method);
    }
    if (needDetach) g_jvm->DetachCurrentThread();
}

static void CallJavaNextAyah() {
    if (!g_jvm) return;
    JNIEnv* env;
    bool needDetach = false;
    jint ret = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (ret != JNI_OK) {
        g_jvm->AttachCurrentThread(&env, nullptr);
        needDetach = true;
    }
    jclass clazz = env->FindClass("com/primaveradev/alfalah/MainActivity");
    if (clazz) {
        jmethodID method = env->GetStaticMethodID(clazz, "nextAyah", "()V");
        if (method) env->CallStaticVoidMethod(clazz, method);
    }
    if (needDetach) g_jvm->DetachCurrentThread();
}

static void CallJavaPrevAyah() {
    if (!g_jvm) return;
    JNIEnv* env;
    bool needDetach = false;
    jint ret = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (ret != JNI_OK) {
        g_jvm->AttachCurrentThread(&env, nullptr);
        needDetach = true;
    }
    jclass clazz = env->FindClass("com/primaveradev/alfalah/MainActivity");
    if (clazz) {
        jmethodID method = env->GetStaticMethodID(clazz, "prevAyah", "()V");
        if (method) env->CallStaticVoidMethod(clazz, method);
    }
    if (needDetach) g_jvm->DetachCurrentThread();
}

static ClayRenderer g_clayRenderer;
static FontManager g_fontManager;
static uint16_t g_arabicFallbackFontId = FontManager::kInvalidFontId;
static uint16_t g_roboto12 = FontManager::kInvalidFontId;
static uint16_t g_roboto15 = FontManager::kInvalidFontId;
static uint16_t g_roboto18 = FontManager::kInvalidFontId;
static uint16_t g_roboto22 = FontManager::kInvalidFontId;
static uint16_t g_roboto24 = FontManager::kInvalidFontId;
static uint64_t g_clayArenaSize = 0;
static void* g_clayArena = nullptr;
static float g_density = 1.0f;
static float g_statusBarHeightDp = 0;
static float g_screenWidthDp = 0;
static float g_screenHeightDp = 0;
static Clay_Color bg = { 19, 19, 10, 255 };
static Clay_Color bg1 = { 31, 31, 22, 255 };
static Clay_Color accent = { 0, 128, 66, 255 };
static Clay_Color accent1 = { 2, 100, 53, 255 };
static Clay_Color fg = { 255, 255, 255, 255 };
static Clay_Color fg1 = { 148, 166, 177, 255 };

enum class Page { Home, Quran, SurahSelection };
static Page g_currentPage = Page::Home;
static int g_selectedSurah = 1;

static Clay_Vector2 g_pointerPos = {0, 0};
static Clay_Vector2 g_pointerDownPos = {0, 0};
static bool g_pointerDown = false;
static bool g_wasPointerDown = false;
static bool g_pointerMovedSignificantly = false;
static double g_pointerDownTime = 0;
static bool g_longPressFired = false;

// Double-tap disambiguation: buffer single tap and delay execution
static bool g_pendingTapActive = false;
static double g_pendingTapTime = 0;
static Page g_pendingTapTargetPage = Page::Home;
static int g_pendingTapSelectedSurah = 0;

static bool g_showAyahMenu = false;
static int g_menuSurah = 0;
static int g_menuAyahNumber = 0;
static bool g_menuActivatedThisTouch = false;
static float g_menuPosX = 0, g_menuPosY = 0;
static std::string g_menuInfoString;
static std::vector<std::string> g_ayahTexts;
static int g_totalAyahs = 0;

static float g_deltaTime = 0.016f;
static const float TAP_MOVE_THRESHOLD = 10.0f;
static const double TAP_MAX_DURATION_MS = 300.0;
static const double LONG_PRESS_DURATION_MS = 500.0;
static const double DOUBLE_TAP_WINDOW_MS = 300.0;
static QuranDatabase g_quranDb;
static int g_surfaceVersion = 0;

enum class QuranDisplayMode { Standard, Mushaf };
static QuranDisplayMode g_quranDisplayMode = QuranDisplayMode::Standard;
static int g_quranModeVersion = 0;

static int g_reciterIndex = 0;
static int g_reciterCount = 0;
static std::string g_reciterName;

static bool g_isPlaying = false;
static bool g_isPaused = false;

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

    atlas->SetSize(config->fontSize * g_density);

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

    {
        float dpSizes[] = {12, 15, 16, 18, 22, 24};
        for (float dp : dpSizes) {
            uint16_t fid = g_fontManager.LoadSystem("Roboto", dp * g_density, false);
            if (fid != FontManager::kInvalidFontId)
                g_fontManager.Register(fid, g_clayRenderer);
        }
        g_roboto12 = g_fontManager.GetFontId("Roboto", 12 * g_density);
        g_roboto15 = g_fontManager.GetFontId("Roboto", 15 * g_density);
        g_roboto18 = g_fontManager.GetFontId("Roboto", 18 * g_density);
        g_roboto22 = g_fontManager.GetFontId("Roboto", 22 * g_density);
        g_roboto24 = g_fontManager.GetFontId("Roboto", 24 * g_density);
    }

    // Load Arabic fallback font (p1.ttf) for Quran text at render size (28dp)
    g_arabicFallbackFontId = g_fontManager.LoadAsset(
        "fonts/quranicFonts/qpc/p1.ttf", 28.0f * g_density, true);
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

    g_reciterCount = CallJavaGetReciterCount();
    g_reciterName = CallJavaGetReciterName(0);
}

extern "C" JNIEXPORT void JNICALL
Java_com_primaveradev_alfalah_MainActivity_nativeOnSurfaceChanged(
        JNIEnv*, jobject, jint w, jint h)
{
//    g_renderer.Resize(w, h);
    g_clayRenderer.SetResolution(w, h);
    g_screenWidthDp = (float)w / g_density;
    g_screenHeightDp = (float)h / g_density;
    Clay_SetLayoutDimensions((Clay_Dimensions){ g_screenWidthDp, g_screenHeightDp });
}

extern "C" JNIEXPORT void JNICALL
Java_com_primaveradev_alfalah_MainActivity_nativeSetStatusBarHeight(
        JNIEnv*, jobject, jfloat heightDp)
{
    g_statusBarHeightDp = heightDp;
}

extern "C" JNIEXPORT void JNICALL
Java_com_primaveradev_alfalah_MainActivity_nativeOnPlaybackStateChanged(
        JNIEnv*, jobject, jboolean active, jboolean paused)
{
    g_isPlaying = active;
    g_isPaused = paused;
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
        g_pointerDownTime = std::chrono::duration<double>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        g_longPressFired = false;
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
                              .fontId = g_roboto24,
                              .fontSize = 24,
                             .wrapMode = CLAY_TEXT_WRAP_WORDS,
                        })
                    );

                    CLAY_TEXT(
                        CLAY_STRING("Read and explore\nthe Qur'an"),
                        CLAY_TEXT_CONFIG({
                             .textColor = fg,
                             .fontId = g_roboto12,
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
                                  .fontId = g_roboto24,
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
                                  .fontId = g_roboto24,
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
                              .fontId = g_roboto24,
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
                              .fontId = g_roboto24,
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
    static std::vector<std::string> surahNames;
    static std::vector<Clay_String> surahNamesStrings;
    static std::vector<std::string> surahNumbers;
    static std::vector<Clay_String> surahNumbersStrings;
    static std::vector<std::string> surahDetails;
    static std::vector<Clay_String> surahDetailsStrings;
    static bool sBuilt = false;
    static int sLastVersion = 0;

    if (sLastVersion != g_surfaceVersion) {
        sBuilt = false;
        sLastVersion = g_surfaceVersion;
    }

    if (!sBuilt) {
        surahNames.resize(114);
        surahNamesStrings.resize(114);
        surahNumbers.resize(114);
        surahNumbersStrings.resize(114);
        surahDetails.resize(114);
        surahDetailsStrings.resize(114);
        for (int i = 0; i < 114; ++i) {
            const SurahInfo& info = g_quranDb.GetSurahInfo(i + 1);
            surahNames[i] = info.nameSimple;
            surahNumbers[i] = std::to_string(i + 1);
            surahDetails[i] = info.revelationPlace == "makkah" ? "Makki" : "Madani";
            surahDetails[i] += " • " + std::to_string(info.verseCount) + " Verses";
            surahNamesStrings[i] = {
                .isStaticallyAllocated = false,
                .length = (int)surahNames[i].length(),
                .chars = surahNames[i].c_str()
            };
            surahNumbersStrings[i] = {
                .isStaticallyAllocated = false,
                .length = (int)surahNumbers[i].length(),
                .chars = surahNumbers[i].c_str()
            };
            surahDetailsStrings[i] = {
                .isStaticallyAllocated = false,
                .length = (int)surahDetails[i].length(),
                .chars = surahDetails[i].c_str()
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
                            .fontId = g_roboto18,
                            .fontSize = 18,
                        .wrapMode = CLAY_TEXT_WRAP_WORDS,
                    })
                );
            }

            CLAY_TEXT(
                CLAY_STRING("Select Surah"),
                CLAY_TEXT_CONFIG({
                    .textColor = fg,
                    .fontId = g_roboto22,
                    .fontSize = 22,
                    .wrapMode = CLAY_TEXT_WRAP_WORDS,
                })
            );
        }

        CLAY(
            CLAY_ID("SurahListScroll"),
            {
                .layout = {
                    .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(1) },
                    .padding = CLAY_PADDING_ALL(8),
                    .childGap = 8,
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                },
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
                            .childGap = 12,
                            .childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
                            .layoutDirection = CLAY_LEFT_TO_RIGHT
                        },
                        .cornerRadius = {10, 10, 10, 10},
                    }
                ) {
                    CLAY(
                        CLAY_IDI("SurahNumber", i),
                        {
                            .layout = {
                                .sizing = {CLAY_SIZING_FIXED(42),
                                           CLAY_SIZING_FIXED(42)},
                                .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER}
                            },
                            .image = {
                                .imageData = ImageLoader::Get("images/starNew.png")
                            }
                        }
                    ) {
                        CLAY_TEXT(
                            surahNumbersStrings[i],
                            CLAY_TEXT_CONFIG({
                                 .textColor = fg,
                                 .fontId = g_roboto15,
                                 .fontSize = 15,
                                 .wrapMode = CLAY_TEXT_WRAP_WORDS,
                            })
                        );
                    }
                    CLAY(
                        CLAY_IDI("SurahEnglishSide", i),
                        {
                            .layout = {
                                .childGap = 6,
                                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                            },
                        }) {
                        CLAY_TEXT(
                            surahNamesStrings[i],
                            CLAY_TEXT_CONFIG({
                                 .textColor = fg,
                                 .fontId = g_roboto18,
                                 .fontSize = 18,
                                 .wrapMode = CLAY_TEXT_WRAP_WORDS,
                            })
                        );
                        CLAY_TEXT(
                            surahDetailsStrings[i],
                            CLAY_TEXT_CONFIG({
                                 .textColor = fg1,
                                 .fontId = g_roboto12,
                                 .fontSize = 12,
                                 .wrapMode = CLAY_TEXT_WRAP_WORDS,
                            })
                        );
                    }
                    CLAY(
                        CLAY_ID("SurahRowSpacer"),
                        {
                            .layout = {
                                .sizing = { .width = CLAY_SIZING_GROW() }
                            }
                    }) {}
                    // TODO: add arabic here with custom font
//                    CLAY_TEXT(
//                        surahDetailsStrings[i],
//                        CLAY_TEXT_CONFIG({
//                             .textColor = fg1,
//                             .fontId = g_roboto12,
//                             .fontSize = 12,
//                             .wrapMode = CLAY_TEXT_WRAP_WORDS,
//                        })
//                    );
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
    static int lastVersion = 0;
    static int s_lastSurah = 0;

    if (lastVersion != g_surfaceVersion || s_lastSurah != g_selectedSurah) {
        sLoaded = false;
        ayahLines.clear();
        ayahStrings.clear();
        ayahFontIds.clear();
        pendingPages.clear();
        pendingIdx = 0;
        g_totalAyahs = 0;
        lastVersion = g_surfaceVersion;
        s_lastSurah = g_selectedSurah;
    }

    if (!sLoaded) {
        const Surah& surah = g_quranDb.GetSurah(g_selectedSurah);
        g_totalAyahs = (int)surah.ayahs.size();
        ayahLines.reserve(g_totalAyahs);
        ayahStrings.reserve(g_totalAyahs);

        std::unordered_set<int32_t> uniquePages;
        std::vector<int32_t> ayahPageNumbers;
        ayahPageNumbers.reserve(g_totalAyahs);

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

        ayahFontIds.assign(g_totalAyahs, FontManager::kInvalidFontId);
        pendingPages.clear();

        g_ayahTexts = ayahLines;
        // Rebuild ayahStrings to point to g_ayahTexts data (same memory as long-press check uses)
        ayahStrings.clear();
        for (auto& s : g_ayahTexts) {
            ayahStrings.push_back({
                .isStaticallyAllocated = false,
                .length = (int)s.length(),
                .chars = s.c_str()
            });
        }

        // Create fontIds for each unique page (lazy, not loaded yet)
        for (int32_t page : uniquePages) {
            char key[64];
            snprintf(key, sizeof(key), "fonts/quranicFonts/qpc/p%d.ttf", page);
            uint16_t fid = g_fontManager.GetOrCreateFontId(key, 28.0f * g_density, true);
            for (int i = 0; i < g_totalAyahs; ++i) {
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
            uint16_t fid = g_fontManager.GetFontId(key, 28.0f * g_density);
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
        uint16_t fid = g_fontManager.GetFontId(key, 28.0f * g_density);
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
                    .left = 12,
                    .right = 12,
                    .top = (uint16_t)(12 + (int)g_statusBarHeightDp),
                    .bottom = 12
                },
                .childGap = 12,
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
            .backgroundColor = bg,
        }
    ) {
        CLAY(
            CLAY_ID("QuranTopBar"),
            {
                .layout = {
                    .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(48) },
                    .childGap = 12,
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                }
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
                        .fontId = g_roboto18,
                        .fontSize = 18,
                        .wrapMode = CLAY_TEXT_WRAP_WORDS,
                    })
                );
            }

            { // reciter toggle
                static std::string s_lastRN;
                static Clay_String s_str;
                if (s_lastRN != g_reciterName) {
                    s_lastRN = g_reciterName;
                    s_str = { false, (int)g_reciterName.length(), g_reciterName.c_str() };
                }
                CLAY(
                    CLAY_ID("QuranReciterToggle"),
                    {
                        .layout = {
                            .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(48) },
                            .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                        },
                    }
                ) {
                    CLAY_TEXT(
                        s_str,
                        CLAY_TEXT_CONFIG({
                            .textColor = accent,
                            .fontId = g_roboto18,
                            .fontSize = 18,
                            .wrapMode = CLAY_TEXT_WRAP_WORDS,
                        })
                    );
                }
            }

            CLAY(
                CLAY_ID("QuranModeToggle"),
                {
                    .layout = {
                        .sizing = { CLAY_SIZING_GROW(1), CLAY_SIZING_FIXED(48) },
                        .childAlignment = { .x = CLAY_ALIGN_X_RIGHT, .y = CLAY_ALIGN_Y_CENTER },
                    },
                }
            ) {
                static Clay_String sToggleLabel = {
                    .isStaticallyAllocated = true,
                    .length = 6,
                    .chars = "Mushaf"
                };
                CLAY_TEXT(
                    sToggleLabel,
                    CLAY_TEXT_CONFIG({
                        .textColor = accent,
                        .fontId = g_roboto18,
                        .fontSize = 18,
                        .wrapMode = CLAY_TEXT_WRAP_WORDS,
                    })
                );
            }
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
                    },
                }
            ) {
                for (int i = 0; i < g_totalAyahs; ++i) {
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

        if (g_isPlaying || g_isPaused) {
            CLAY(
                CLAY_ID("ControlsBar"),
                {
                    .layout = {
                        .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(48) },
                        .layoutDirection = CLAY_LEFT_TO_RIGHT,
                        .padding = { .top = 6, .bottom = 6 },
                    },
                    .backgroundColor = bg1,
                    .cornerRadius = {12, 12, 12, 12},
                }
            ) {
                CLAY(CLAY_ID("ControlsSpacer0"), {
                    .layout = { .sizing = {CLAY_SIZING_GROW(0)} },
                }) {}
                CLAY(CLAY_ID("AudioStop"), {
                    .layout = { .sizing = {CLAY_SIZING_FIXED(36), CLAY_SIZING_FIXED(36)} },
                    .image = { .imageData = ImageLoader::Get("images/stopGreen.png") },
                }) {}
                CLAY(CLAY_ID("ControlsSpacer1"), {
                    .layout = { .sizing = {CLAY_SIZING_GROW(0)} },
                }) {}
                CLAY(CLAY_ID("AudioPrev"), {
                    .layout = { .sizing = {CLAY_SIZING_FIXED(36), CLAY_SIZING_FIXED(36)} },
                    .image = { .imageData = ImageLoader::Get("images/backGreen.png") },
                }) {}
                CLAY(CLAY_ID("ControlsSpacer2"), {
                    .layout = { .sizing = {CLAY_SIZING_GROW(0)} },
                }) {}
                CLAY(CLAY_ID("AudioPlayPause"), {
                    .layout = { .sizing = {CLAY_SIZING_FIXED(36), CLAY_SIZING_FIXED(36)} },
                    .image = { .imageData = ImageLoader::Get(g_isPaused ? "images/playGreen.png" : "images/pauseGreen.png") },
                }) {}
                CLAY(CLAY_ID("ControlsSpacer3"), {
                    .layout = { .sizing = {CLAY_SIZING_GROW(0)} },
                }) {}
                CLAY(CLAY_ID("AudioNext"), {
                    .layout = { .sizing = {CLAY_SIZING_FIXED(36), CLAY_SIZING_FIXED(36)} },
                    .image = { .imageData = ImageLoader::Get("images/forwardGreen.png") },
                }) {}
                CLAY(CLAY_ID("ControlsSpacer4"), {
                    .layout = { .sizing = {CLAY_SIZING_GROW(0)} },
                }) {}
            }
        }
    }
}

static float MeasureMushafWidth(const std::string& text, uint16_t fontId, float fontSize)
{
    FontAtlas* atlas = g_fontManager.GetAtlas(fontId);
    if (!atlas) return 0;
    atlas->SetSize(fontSize * g_density);
    hb_font_t* hbFont = atlas->GetHbFont();
    if (!hbFont) return 0;
    hb_buffer_t* buf = hb_buffer_create();
    hb_buffer_add_utf8(buf, text.c_str(), (int)text.length(), 0, (int)text.length());
    hb_buffer_set_direction(buf, HB_DIRECTION_RTL);
    hb_buffer_set_script(buf, HB_SCRIPT_ARABIC);
    hb_buffer_set_language(buf, hb_language_from_string("ar", 2));
    hb_shape(hbFont, buf, nullptr, 0);
    unsigned int count = 0;
    hb_glyph_position_t* pos = hb_buffer_get_glyph_positions(buf, &count);
    float total = 0;
    for (unsigned int i = 0; i < count; ++i)
        total += (float)pos[i].x_advance / 64.0f;
    hb_buffer_destroy(buf);
    // RTL x_advance is negative; take absolute value for width
    return (total < 0 ? -total : total) / g_density;
}

struct MushafCacheEntry {
    std::vector<std::string> lineTexts;
    std::vector<bool> lineCentered;
    std::vector<int32_t> linePages;
    std::vector<uint16_t> lineFontIds;
    std::vector<float> lineFontSizes;
    std::vector<int32_t> lineSectionStart;
    std::vector<std::string> sectionTexts;
    std::vector<int32_t> sectionAyahs;
    int totalLines = 0;
    int totalSections = 0;
};

static std::vector<std::string> g_mushafSectionTexts;
static std::vector<int32_t> g_mushafSectionAyahs;

static void LayoutQuranPageMushaf()
{
    static std::unordered_map<int32_t, MushafCacheEntry> s_cache;
    static int s_lastVersion = 0;
    static float s_lastScreenWidth = 0;
    static std::vector<Clay_String> s_sectionStrings;

    if (s_lastVersion != g_surfaceVersion || s_lastScreenWidth != g_screenWidthDp) {
        s_cache.clear();
        s_lastVersion = g_surfaceVersion;
        s_lastScreenWidth = g_screenWidthDp;
    }

    auto it = s_cache.find(g_selectedSurah);
    if (it == s_cache.end()) {
        MushafCacheEntry entry;

        const SurahInfo& info = g_quranDb.GetSurahInfo(g_selectedSurah);
        int32_t startPage = info.startPage;
        int32_t endPage = info.endPage;
        LOGI("Mushaf: surah %d pages %d-%d", g_selectedSurah, startPage, endPage);

        std::unordered_set<int32_t> uniquePages;
        float availWidth = g_screenWidthDp - 12.0f - 12.0f - 15.0f - 15.0f;

        for (int32_t page = startPage; page <= endPage; ++page) {
            const PageLayout& layout = g_quranDb.GetPageLayout(page);
            if (layout.lines.empty()) {
                LOGI("Mushaf: page %d no layout", page);
                continue;
            }

            for (const auto& pl : layout.lines) {
                if (pl.surahNumber != g_selectedSurah) continue;

                entry.lineSectionStart.push_back((int32_t)entry.sectionTexts.size());
                std::string fullText;
                int32_t ayahNum = 1;

                if (pl.lineType == "surah_name") {
                    const SurahInfo& sinfo = g_quranDb.GetSurahInfo(pl.surahNumber);
                    fullText = sinfo.nameArabic;
                    entry.sectionTexts.push_back(fullText);
                    entry.sectionAyahs.push_back(1);
                } else if (pl.lineType == "basmallah") {
                    fullText = "\xD8\xA8\xD9\x90\xD8\xB3\xD9\x92\xD9\x85\xD9\x90 \xD8\xA7\xD9\x84\xD9\x84\xD9\x91\xD9\x87\xD9\x90 \xD8\xA7\xD9\x84\xD8\xB1\xD9\x91\xD8\xAD\xD9\x92\xD9\x85\xD9\x8E\xD9\x86\xD9\x90 \xD8\xA7\xD9\x84\xD8\xB1\xD9\x91\xD8\xAD\xD9\x90\xD9\x8A\xD9\x85\xD9\x90";
                    entry.sectionTexts.push_back(fullText);
                    entry.sectionAyahs.push_back(1);
                } else if (pl.lineType == "ayah") {
                    std::vector<std::pair<std::string, int32_t>> wordAyahs;
                    for (int32_t wid = pl.firstWordId; wid <= pl.lastWordId; ++wid) {
                        const std::string* wt = g_quranDb.GetWordText(wid);
                        if (wt) {
                            if (!fullText.empty()) fullText += ' ';
                            fullText += *wt;
                            wordAyahs.push_back({*wt, g_quranDb.GetWordAyah(wid)});
                        }
                    }
                    if (wordAyahs.empty()) continue;

                    std::vector<std::string> groupTexts;
                    std::vector<int32_t> groupAyahs;
                    int32_t curAyah = wordAyahs[0].second;
                    std::string curGroup = wordAyahs[0].first;
                    for (size_t wi = 1; wi < wordAyahs.size(); ++wi) {
                        if (wordAyahs[wi].second == curAyah) {
                            curGroup += ' ';
                            curGroup += wordAyahs[wi].first;
                        } else {
                            groupTexts.push_back(curGroup);
                            groupAyahs.push_back(curAyah);
                            curAyah = wordAyahs[wi].second;
                            curGroup = wordAyahs[wi].first;
                        }
                    }
                    groupTexts.push_back(curGroup);
                    groupAyahs.push_back(curAyah);
                    ayahNum = groupAyahs.front();

                    for (int gi = (int)groupTexts.size() - 1; gi >= 0; --gi) {
                        entry.sectionTexts.push_back(std::move(groupTexts[gi]));
                        entry.sectionAyahs.push_back(groupAyahs[gi]);
                    }
                }

                entry.lineTexts.push_back(std::move(fullText));
                entry.lineCentered.push_back(pl.isCentered);
                entry.linePages.push_back(page);
                uniquePages.insert(page);
            }
        }

        entry.totalLines = (int)entry.lineTexts.size();
        entry.totalSections = (int)entry.sectionTexts.size();

        entry.lineFontIds.assign(entry.totalLines, FontManager::kInvalidFontId);
        entry.lineFontSizes.assign(entry.totalLines, 28.0f);

        for (int32_t page : uniquePages) {
            char key[64];
            snprintf(key, sizeof(key), "fonts/quranicFonts/qpc/p%d.ttf", page);
            uint16_t fid = g_fontManager.GetOrCreateFontId(key, 28.0f * g_density, true);
            if (fid != FontManager::kInvalidFontId &&
                g_fontManager.EnsureFontLoaded(fid)) {
                g_fontManager.Register(fid, g_clayRenderer);
            }
            for (int i = 0; i < entry.totalLines; ++i) {
                if (entry.linePages[i] == page) {
                    entry.lineFontIds[i] = fid;
                }
            }
        }

        for (int32_t page : uniquePages) {
            uint16_t pageFontId = FontManager::kInvalidFontId;
            for (int i = 0; i < entry.totalLines; ++i) {
                if (entry.linePages[i] == page) {
                    pageFontId = entry.lineFontIds[i];
                    break;
                }
            }
            if (pageFontId == FontManager::kInvalidFontId) continue;

            float bestSize = 28.0f;
            for (float size = 28.0f; size >= 15.0f; size -= 0.5f) {
                bool allFit = true;
                for (int i = 0; i < entry.totalLines; ++i) {
                    if (entry.linePages[i] != page) continue;
                    float w = MeasureMushafWidth(entry.lineTexts[i], pageFontId, size);
                    if (w > availWidth) { allFit = false; break; }
                }
                if (allFit) { bestSize = size; break; }
            }
            for (int i = 0; i < entry.totalLines; ++i) {
                if (entry.linePages[i] == page) {
                    entry.lineFontSizes[i] = bestSize;
                }
            }
        }

        it = s_cache.emplace(g_selectedSurah, std::move(entry)).first;
    }

    const MushafCacheEntry& cache = it->second;
    g_mushafSectionTexts = cache.sectionTexts;
    g_mushafSectionAyahs = cache.sectionAyahs;

    s_sectionStrings.clear();
    s_sectionStrings.reserve(cache.totalSections);
    for (const auto& s : g_mushafSectionTexts) {
        s_sectionStrings.push_back({
            .isStaticallyAllocated = false,
            .length = (int)s.length(),
            .chars = s.c_str()
        });
    }

    CLAY(
        CLAY_ID("Root"),
        {
            .layout = {
                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                .padding = {
                    .left = 12,
                    .right = 12,
                    .top = (uint16_t)(12 + (int)g_statusBarHeightDp),
                    .bottom = 12
                },
                .childGap = 12,
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
            .backgroundColor = bg,
        }
    ) {
        CLAY(
            CLAY_ID("QuranTopBar"),
            {
                .layout = {
                    .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(48) },
                    .childGap = 12,
                    .layoutDirection = CLAY_LEFT_TO_RIGHT,
                }
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
                        .fontId = g_roboto18,
                        .fontSize = 18,
                        .wrapMode = CLAY_TEXT_WRAP_WORDS,
                    })
                );
            }

            { // reciter toggle
                static std::string s_lastRN;
                static Clay_String s_str;
                if (s_lastRN != g_reciterName) {
                    s_lastRN = g_reciterName;
                    s_str = { false, (int)g_reciterName.length(), g_reciterName.c_str() };
                }
                CLAY(
                    CLAY_ID("QuranReciterToggle"),
                    {
                        .layout = {
                            .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(48) },
                            .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                        },
                    }
                ) {
                    CLAY_TEXT(
                        s_str,
                        CLAY_TEXT_CONFIG({
                            .textColor = accent,
                            .fontId = g_roboto18,
                            .fontSize = 18,
                            .wrapMode = CLAY_TEXT_WRAP_WORDS,
                        })
                    );
                }
            }

            CLAY(
                CLAY_ID("QuranModeToggle"),
                {
                    .layout = {
                        .sizing = { CLAY_SIZING_GROW(1), CLAY_SIZING_FIXED(48) },
                        .childAlignment = { .x = CLAY_ALIGN_X_RIGHT, .y = CLAY_ALIGN_Y_CENTER },
                    },
                }
            ) {
                static Clay_String sToggleLabel = {
                    .isStaticallyAllocated = true,
                    .length = 8,
                    .chars = "Standard"
                };
                CLAY_TEXT(
                    sToggleLabel,
                    CLAY_TEXT_CONFIG({
                        .textColor = accent,
                        .fontId = g_roboto18,
                        .fontSize = 18,
                        .wrapMode = CLAY_TEXT_WRAP_WORDS,
                    })
                );
            }
        }

        CLAY(
            CLAY_ID("MushafScrollContainer"),
            {
                .layout = {
                    .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                    .padding = CLAY_PADDING_ALL(15),
                    .childGap = 8,
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                },
                .cornerRadius = {24, 24, 24, 24},
                .clip = {
                    .vertical = true,
                    .childOffset = Clay_GetScrollOffset()
                },
            }
        ) {
            for (int i = 0; i < cache.totalLines; ++i) {
                if (i > 0 && cache.linePages[i] != cache.linePages[i - 1]) {
                    CLAY(
                        CLAY_IDI("PageSep", i),
                        {
                            .layout = {
                                .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(24) },
                                .layoutDirection = CLAY_LEFT_TO_RIGHT,
                            },
                        }
                    ) {
                        CLAY(
                            CLAY_IDI("PageSepLeft", i),
                            {
                                .layout = {
                                    .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(2) },
                                },
                                .backgroundColor = bg1,
                            }
                        ) {}

                        CLAY(
                            CLAY_IDI("PageSepRight", i),
                            {
                                .layout = {
                                    .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(2) },
                                },
                                .backgroundColor = bg1,
                            }
                        ) {}
                    }
                }

                uint16_t fid = cache.lineFontIds[i];
                if (fid == FontManager::kInvalidFontId || !g_fontManager.GetAtlas(fid)) {
                    fid = g_arabicFallbackFontId;
                }

                int sectionStart = cache.lineSectionStart[i];
                int sectionEnd = (i + 1 < cache.totalLines)
                    ? cache.lineSectionStart[i + 1] : cache.totalSections;

                CLAY(
                    CLAY_IDI("MushafLine", i),
                    {
                        .layout = {
                            .sizing = { CLAY_SIZING_GROW(0) },
                            .childAlignment = {
                                .x = cache.lineCentered[i] ? CLAY_ALIGN_X_CENTER : CLAY_ALIGN_X_RIGHT,
                            },
                        },
                    }
                ) {
                    for (int s = sectionStart; s < sectionEnd; ++s) {
                        CLAY_TEXT(
                            s_sectionStrings[s],
                            CLAY_TEXT_CONFIG({
                                .textColor = fg,
                                .fontId = fid,
                                .fontSize = cache.lineFontSizes[i],
                                .textAlignment = cache.lineCentered[i] ? CLAY_TEXT_ALIGN_CENTER : CLAY_TEXT_ALIGN_RIGHT,
                            })
                        );
                    }
                }
            }
        }

        if (g_isPlaying || g_isPaused) {
            CLAY(
                CLAY_ID("ControlsBar"),
                {
                    .layout = {
                        .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(48) },
                        .layoutDirection = CLAY_LEFT_TO_RIGHT,
                        .padding = { .top = 6, .bottom = 6 },
                    },
                    .backgroundColor = bg1,
                    .cornerRadius = {12, 12, 12, 12},
                }
            ) {
                CLAY(CLAY_ID("ControlsSpacer0"), {
                    .layout = { .sizing = {CLAY_SIZING_GROW(0)} },
                }) {}
                CLAY(CLAY_ID("AudioStop"), {
                    .layout = { .sizing = {CLAY_SIZING_FIXED(36), CLAY_SIZING_FIXED(36)} },
                    .image = { .imageData = ImageLoader::Get("images/stopGreen.png") },
                }) {}
                CLAY(CLAY_ID("ControlsSpacer1"), {
                    .layout = { .sizing = {CLAY_SIZING_GROW(0)} },
                }) {}
                CLAY(CLAY_ID("AudioPrev"), {
                    .layout = { .sizing = {CLAY_SIZING_FIXED(36), CLAY_SIZING_FIXED(36)} },
                    .image = { .imageData = ImageLoader::Get("images/backGreen.png") },
                }) {}
                CLAY(CLAY_ID("ControlsSpacer2"), {
                    .layout = { .sizing = {CLAY_SIZING_GROW(0)} },
                }) {}
                CLAY(CLAY_ID("AudioPlayPause"), {
                    .layout = { .sizing = {CLAY_SIZING_FIXED(36), CLAY_SIZING_FIXED(36)} },
                    .image = { .imageData = ImageLoader::Get(g_isPaused ? "images/playGreen.png" : "images/pauseGreen.png") },
                }) {}
                CLAY(CLAY_ID("ControlsSpacer3"), {
                    .layout = { .sizing = {CLAY_SIZING_GROW(0)} },
                }) {}
                CLAY(CLAY_ID("AudioNext"), {
                    .layout = { .sizing = {CLAY_SIZING_FIXED(36), CLAY_SIZING_FIXED(36)} },
                    .image = { .imageData = ImageLoader::Get("images/forwardGreen.png") },
                }) {}
                CLAY(CLAY_ID("ControlsSpacer4"), {
                    .layout = { .sizing = {CLAY_SIZING_GROW(0)} },
                }) {}
            }
        }
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_primaveradev_alfalah_MainActivity_nativeOnDrawFrame(
        JNIEnv*, jobject)
{
    Clay_SetPointerState(g_pointerPos, g_pointerDown);
    if (!g_showAyahMenu)
        Clay_UpdateScrollContainers(true, (Clay_Vector2){0, 0}, g_deltaTime);

    Clay_BeginLayout();
    {
        switch (g_currentPage) {
            case Page::Home:
                LayoutHomePage();
                break;
            case Page::Quran:
                if (g_quranDisplayMode == QuranDisplayMode::Mushaf)
                    LayoutQuranPageMushaf();
                else
                    LayoutQuranPage();
                break;
            case Page::SurahSelection:
                LayoutSurahSelectionPage();
                break;
        }

        if (g_showAyahMenu) {
            const float menuW = (24*2) * g_density, menuH = 24*g_density;
            float menuX = g_menuPosX - menuW/2;
            float menuY = g_menuPosY - menuH;
            if (menuX < 8) menuX = 8;
            if (menuX + menuW > g_screenWidthDp - 8) menuX = g_screenWidthDp - menuW - 8;
            if (menuY < g_statusBarHeightDp + 8) menuY = g_menuPosY + 20;
            if (menuY + menuH > g_screenHeightDp - 8) menuY = g_screenHeightDp - menuH - 8;

            g_menuInfoString = g_quranDb.GetSurahInfo(g_menuSurah).nameSimple
                            + " " + std::to_string(g_menuAyahNumber);
            Clay_String menuStr = {
                .isStaticallyAllocated = false,
                .length = (int32_t)g_menuInfoString.length(),
                .chars = g_menuInfoString.c_str()
            };

            CLAY(
                CLAY_ID("MenuBackdrop"),
                {
                    .layout = {
                        .sizing = { CLAY_SIZING_GROW(1), CLAY_SIZING_GROW(1) },
                    },
                    .floating = {
                        .zIndex = 10,
                        .attachTo = CLAY_ATTACH_TO_PARENT,
                        .pointerCaptureMode = CLAY_POINTER_CAPTURE_MODE_CAPTURE,
                    }
                }
            ) {
                CLAY(
                    CLAY_ID("AyahMenu"),
                    {
                        .layout = {
//                            .sizing = {  },
                            .padding = CLAY_PADDING_ALL(14),
                            .childGap = 10,
                            .layoutDirection = CLAY_LEFT_TO_RIGHT,
                        },
                        .backgroundColor = bg1,
                        .cornerRadius = {16, 16, 16, 16},
                        .floating = {
                            .offset = { menuX, menuY },
                            .zIndex = 11,
                            .attachTo = CLAY_ATTACH_TO_PARENT,
                        },
                    }
                ) {
                    CLAY(
                        CLAY_ID("MenuPlayButton"),
                        {
                            .layout = {
                                .sizing = {CLAY_SIZING_FIXED(24), CLAY_SIZING_FIXED(24) },
                                .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                            },
                            .image = {
                                .imageData = ImageLoader::Get("images/playGreen.png")
                            },
                        }
                    ) {}
                    CLAY(
                        CLAY_ID("MenuBookmarkButton"),
                        {
                            .layout = {
                                .sizing = {CLAY_SIZING_FIXED(24), CLAY_SIZING_FIXED(24) },
                                .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                            },
                            .image = {
                                .imageData = ImageLoader::Get("images/bookmarkGreen.png")
                            },
                        }
                    ) {}
                }
            }
        }
    }
    Clay_RenderCommandArray commands = Clay_EndLayout(0);

    double now = std::chrono::duration<double>(
        std::chrono::steady_clock::now().time_since_epoch()).count();

    if (g_pointerDown) {
        float dx = g_pointerPos.x - g_pointerDownPos.x;
        float dy = g_pointerPos.y - g_pointerDownPos.y;
        if (dx * dx + dy * dy > TAP_MOVE_THRESHOLD * TAP_MOVE_THRESHOLD) {
            g_pointerMovedSignificantly = true;
        }
    }

    // Long press: finger held still past threshold
    if (g_pointerDown && !g_pointerMovedSignificantly && !g_longPressFired) {
        double elapsedMs = (now - g_pointerDownTime) * 1000.0;
        if (elapsedMs >= LONG_PRESS_DURATION_MS) {
            g_longPressFired = true;
            if (g_currentPage == Page::Quran) {
                if (g_quranDisplayMode == QuranDisplayMode::Mushaf && !g_mushafSectionTexts.empty()) {
                    LOGI("long press mushaf: scanning %d commands", commands.length);
                    for (int32_t ci = 0; ci < commands.length; ++ci) {
                        const Clay_RenderCommand& cmd = commands.internalArray[ci];
                        if (cmd.commandType != CLAY_RENDER_COMMAND_TYPE_TEXT) continue;
                        const auto& bb = cmd.boundingBox;
                        if (g_pointerPos.x >= bb.x && g_pointerPos.x < bb.x + bb.width &&
                            g_pointerPos.y >= bb.y && g_pointerPos.y < bb.y + bb.height) {
                            const char* ptr = cmd.renderData.text.stringContents.chars;
                            for (int i = 0; i < (int)g_mushafSectionTexts.size(); ++i) {
                                const std::string& section = g_mushafSectionTexts[i];
                                if (!section.empty() && ptr >= section.data() && ptr < section.data() + section.length()) {
                                    LOGI("  -> matched mushaf section %d ayah %d", i, g_mushafSectionAyahs[i]);
                                    g_showAyahMenu = true;
                                    g_menuSurah = g_selectedSurah;
                                    g_menuAyahNumber = g_mushafSectionAyahs[i];
                                    g_menuActivatedThisTouch = true;
                                    g_menuPosX = g_pointerPos.x;
                                    g_menuPosY = g_pointerPos.y;
                                    break;
                                }
                            }
                            if (g_showAyahMenu) break;
                        }
                    }
                    if (!g_showAyahMenu) LOGI("  no mushaf section matched");
                } else if (!g_ayahTexts.empty()) {
                    LOGI("long press: scanning %d commands", commands.length);
                    for (int32_t ci = 0; ci < commands.length; ++ci) {
                        const Clay_RenderCommand& cmd = commands.internalArray[ci];
                        if (cmd.commandType != CLAY_RENDER_COMMAND_TYPE_TEXT) continue;
                        const auto& bb = cmd.boundingBox;
                        if (g_pointerPos.x >= bb.x && g_pointerPos.x < bb.x + bb.width &&
                            g_pointerPos.y >= bb.y && g_pointerPos.y < bb.y + bb.height) {
                            const char* ptr = cmd.renderData.text.stringContents.chars;
                            LOGI("  text cmd at (%.0f,%.0f) chars=%p len=%d", bb.x, bb.y, ptr, cmd.renderData.text.stringContents.length);
                            for (int i = 0; i < g_totalAyahs; ++i) {
                                const std::string& ayah = g_ayahTexts[i];
                                if (!ayah.empty() && ptr >= ayah.data() && ptr < ayah.data() + ayah.length()) {
                                    LOGI("  -> matched ayah %d", i);
                                    const Surah& surah = g_quranDb.GetSurah(g_selectedSurah);
                                    g_showAyahMenu = true;
                                    g_menuSurah = g_selectedSurah;
                                    g_menuAyahNumber = surah.ayahs[i].ayahNumber;
                                    g_menuActivatedThisTouch = true;
                                    g_menuPosX = g_pointerPos.x;
                                    g_menuPosY = g_pointerPos.y;
                                    break;
                                }
                            }
                            if (g_showAyahMenu) break;
                        }
                    }
                    if (!g_showAyahMenu) LOGI("  no ayah matched");
                }
            }
        }
    }

    // On finger release: check for tap or double-tap
    if (g_wasPointerDown && !g_pointerDown) {
        double elapsedMs = (now - g_pointerDownTime) * 1000.0;
        bool isTap = !g_pointerMovedSignificantly && elapsedMs < TAP_MAX_DURATION_MS;

        // Menu tap handling (fires on any tap while menu is visible)
        if (g_showAyahMenu && isTap) {
            if (Clay_PointerOver(CLAY_ID("MenuPlayButton"))) {
                LOGI("play button tapped: surah=%d ayah=%d",
                     g_menuSurah, g_menuAyahNumber);
                CallJavaPlayAyah(g_menuSurah, g_menuAyahNumber);
                g_showAyahMenu = false;
            } else if (Clay_PointerOver(CLAY_ID("MenuBackdrop"))) {
                LOGI("menu backdrop tapped — closing menu");
                g_showAyahMenu = false;
            }
        }
        // Normal navigation tap (only when menu is NOT active)
        else if (isTap && !g_showAyahMenu) {
            Page targetPage = g_currentPage;
            int selectedSurah = 0;

            switch (g_currentPage) {
                case Page::Home:
                    if (Clay_PointerOver(CLAY_ID("QuranButtonContainer")))
                        targetPage = Page::SurahSelection;
                    else
                        isTap = false;
                    break;
                case Page::Quran:
                    if (Clay_PointerOver(CLAY_ID("QuranBackButton"))) {
                        if (g_isPlaying || g_isPaused) CallJavaStop();
                        targetPage = Page::SurahSelection;
                    } else if (Clay_PointerOver(CLAY_ID("QuranReciterToggle"))) {
                        if (g_isPlaying || g_isPaused) CallJavaStop();
                        g_reciterIndex = (g_reciterIndex + 1) % g_reciterCount;
                        CallJavaSetReciter(g_reciterIndex);
                        g_reciterName = CallJavaGetReciterName(g_reciterIndex);
                        isTap = true;
                        targetPage = Page::Quran;
                    } else if (Clay_PointerOver(CLAY_ID("QuranModeToggle"))) {
                        if (g_quranDisplayMode == QuranDisplayMode::Standard)
                            g_quranDisplayMode = QuranDisplayMode::Mushaf;
                        else
                            g_quranDisplayMode = QuranDisplayMode::Standard;
                        ++g_quranModeVersion;
                        isTap = true;
                        targetPage = Page::Quran;
                    } else if (g_isPlaying || g_isPaused) {
                        if (Clay_PointerOver(CLAY_ID("AudioStop"))) {
                            CallJavaStop();
                            isTap = true;
                            targetPage = Page::Quran;
                        } else if (Clay_PointerOver(CLAY_ID("AudioPrev"))) {
                            CallJavaPrevAyah();
                            isTap = true;
                            targetPage = Page::Quran;
                        } else if (Clay_PointerOver(CLAY_ID("AudioPlayPause"))) {
                            CallJavaTogglePlayPause();
                            isTap = true;
                            targetPage = Page::Quran;
                        } else if (Clay_PointerOver(CLAY_ID("AudioNext"))) {
                            CallJavaNextAyah();
                            isTap = true;
                            targetPage = Page::Quran;
                        } else
                            isTap = false;
                    } else
                        isTap = false;
                    break;
                case Page::SurahSelection:
                    if (Clay_PointerOver(CLAY_ID("SurahBackButton")))
                        targetPage = Page::Home;
                    else {
                        isTap = false;
                        for (int i = 0; i < 114; ++i) {
                            if (Clay_PointerOver(CLAY_IDI("SurahRow", i))) {
                                targetPage = Page::Quran;
                                selectedSurah = i + 1;
                                isTap = true;
                                break;
                            }
                        }
                    }
                    break;
            }

            if (isTap) {
                // Check if this is the second tap of a double-tap
                if (g_pendingTapActive &&
                    (now - g_pendingTapTime) * 1000.0 < DOUBLE_TAP_WINDOW_MS) {
                    g_pendingTapActive = false;
                    g_currentPage = targetPage;
                    if (selectedSurah > 0)
                        g_selectedSurah = selectedSurah;
                } else {
                    // Buffer as a pending single tap — wait for double-tap window
                    g_pendingTapActive = true;
                    g_pendingTapTime = now;
                    g_pendingTapTargetPage = targetPage;
                    g_pendingTapSelectedSurah = selectedSurah;
                }
            }
        }

        g_menuActivatedThisTouch = false;
    }

    // Execute pending single tap if double-tap window expired
    if (g_pendingTapActive) {
        double elapsedMs = (now - g_pendingTapTime) * 1000.0;
        if (elapsedMs >= DOUBLE_TAP_WINDOW_MS) {
            g_pendingTapActive = false;
            g_currentPage = g_pendingTapTargetPage;
            if (g_pendingTapSelectedSurah > 0)
                g_selectedSurah = g_pendingTapSelectedSurah;
        }
    }

    g_wasPointerDown = g_pointerDown;

    ImageLoader::FixAspectRatios(commands);
    g_clayRenderer.Render(commands);
}
