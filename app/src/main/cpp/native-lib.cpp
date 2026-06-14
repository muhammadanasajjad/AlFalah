#include <jni.h>
#include <android/log.h>
#include <android/asset_manager_jni.h>
#include <malloc.h>
#include "engine/renderer/Renderer.hpp"
#include "engine/renderer/ClayRenderer.hpp"
#include "engine/renderer/FontAtlas.hpp"
#include "engine/renderer/ImageLoader.hpp"
#include "third_party/clay/clay.h"

#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "NATIVE", __VA_ARGS__)

static Renderer g_renderer;
static ClayRenderer g_clayRenderer;
static FontAtlas g_fontAtlas;
static uint64_t g_clayArenaSize = 0;
static void* g_clayArena = nullptr;
static float g_density = 1.0f;
static float g_statusBarHeightDp = 0;
static Clay_Color bg = { 19, 19, 10, 255 };
static Clay_Color bg1 = { 31, 31, 22, 255 };
static Clay_Color accent = { 0, 128, 66, 255 };
static Clay_Color accent1 = { 2, 100, 53, 255 };
static Clay_Color fg = { 255, 255, 255, 255 };

static const char* kFontPaths[] = {
    "/system/fonts/NotoSans-Regular.ttf",
    "/system/fonts/Roboto-Regular.ttf",
    "/system/fonts/RobotoCondensed-Regular.ttf",
    "/system/fonts/Roboto-Light.ttf",
    "/system/fonts/DroidSans.ttf",
};

static Clay_Dimensions MeasureText(Clay_StringSlice text,
                                   Clay_TextElementConfig* config,
                                   void*)
{
    if (!config) return { 0, 0 };
    g_fontAtlas.SetSize((float)config->fontSize * g_density);

    hb_font_t* hbFont = g_fontAtlas.GetHbFont();
    if (!hbFont) return { 0, 0 };

    hb_buffer_t* buf = hb_buffer_create();
    hb_buffer_add_utf8(buf, text.chars, text.length, 0, text.length);
    hb_buffer_set_direction(buf, HB_DIRECTION_LTR);
    hb_buffer_set_script(buf, HB_SCRIPT_LATIN);
    hb_buffer_set_language(buf, hb_language_from_string("en", 2));
    hb_shape(hbFont, buf, nullptr, 0);

    unsigned int count = 0;
    hb_glyph_info_t* info = hb_buffer_get_glyph_infos(buf, &count);
    hb_glyph_position_t* pos = hb_buffer_get_glyph_positions(buf, &count);

    float total = 0;
    for (unsigned int i = 0; i < count; ++i)
        total += (float)pos[i].x_advance / 64.0f;

    hb_buffer_destroy(buf);

    return { total / g_density, g_fontAtlas.GetLineHeight() / g_density };
}

extern "C" JNIEXPORT void JNICALL
Java_com_primaveradev_alfalah_MainActivity_nativeOnSurfaceCreated(
        JNIEnv* env, jobject, jobject assetManager, jfloat density)
{
    g_density = density;

    AAssetManager* mgr = AAssetManager_fromJava(env, assetManager);

    g_renderer.SetAssetManager(mgr);
    g_renderer.Init();

    g_clayRenderer.Init(mgr);
    g_clayRenderer.SetDensity(g_density);

    ImageLoader::Init(mgr);

    // Load font from system paths
    bool fontOk = false;
    for (const char* fp : kFontPaths) {
        if (g_fontAtlas.LoadFromFile(fp, (float)(16 * g_density))) {
            fontOk = true;
            break;
        }
    }
    // Fallback: bundled font in APK assets
    if (!fontOk && g_fontAtlas.Load(mgr, "fonts/Roboto-Regular.ttf", (float)(16 * g_density))) {
        fontOk = true;
    }
    if (fontOk) {
        g_clayRenderer.SetFontAtlas(&g_fontAtlas);
    } else {
        LOGE("ALL FONT LOADING FAILED — no text will render");
    }

    if (!g_clayArena) {
        g_clayArenaSize = Clay_MinMemorySize();
        g_clayArena = malloc(g_clayArenaSize);
        Clay_Initialize(
            Clay_CreateArenaWithCapacityAndMemory(g_clayArenaSize, g_clayArena),
            (Clay_Dimensions){ 0, 0 },
            (Clay_ErrorHandler){ 0 });
        Clay_SetMeasureTextFunction(MeasureText, nullptr);
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_primaveradev_alfalah_MainActivity_nativeOnSurfaceChanged(
        JNIEnv*, jobject, jint w, jint h)
{
    g_renderer.Resize(w, h);
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
Java_com_primaveradev_alfalah_MainActivity_nativeOnDrawFrame(
        JNIEnv*, jobject)
{
    g_renderer.BeginFrame();

    Clay_BeginLayout();
    {
        // Demo: overflow hidden container
        CLAY(
            CLAY_ID("Cropper"),
            {
                .layout = {
                    .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) },
                    .padding = {
                            .left = 24,
                            .right = 24,
                            .top = (uint16_t)(24 + (int)g_statusBarHeightDp),
                            .bottom = 24
                    },
                    .childGap = 12,
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
                            .padding = CLAY_PADDING_ALL(24),
                            .childGap = 12,
                            .layoutDirection = CLAY_TOP_TO_BOTTOM,
                        },
                        .backgroundColor = bg1,
                        .cornerRadius = {24, 24, 24, 24},
                        .clip = { .horizontal = true, .vertical = true }
                    },
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
                    CLAY(
                        CLAY_ID("QuranImage"),
                        {
                            .layout = {
                                .sizing = { CLAY_SIZING_PERCENT(0.70), CLAY_SIZING_FIXED(70) }
                            },
                            .cornerRadius = { 8, 8, 8, 8 },
                            .image = { .imageData = ImageLoader::Get("images/quran.png") },
                            .floating = {
                                .offset = { 10, 10 },
                                .attachPoints = {
                                    .element = CLAY_ATTACH_POINT_RIGHT_BOTTOM,
                                    .parent = CLAY_ATTACH_POINT_RIGHT_BOTTOM,
                                },
                                .attachTo = CLAY_ATTACH_TO_PARENT,
                            },
                        }
                    ) {}
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
                                .padding = CLAY_PADDING_ALL(24),
                            },
                            .backgroundColor = bg1,
                            .cornerRadius = {24, 24, 24, 24},
                            .clip = { .horizontal = true, .vertical = true }
                        },
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
                        CLAY_ID("AlbumsContainer"),
                        {
                            .layout = {
                                .sizing = { CLAY_SIZING_GROW(1), CLAY_SIZING_GROW(1) },
                                .padding = CLAY_PADDING_ALL(24),
                            },
                            .backgroundColor = bg1,
                            .cornerRadius = {24, 24, 24, 24},
                            .clip = { .horizontal = true, .vertical = true }
                        },
                    ) {
                        CLAY_TEXT(
                            CLAY_STRING("Albums"),
                            CLAY_TEXT_CONFIG({
                                 .textColor = fg,
                                 .fontId = 0,
                                 .fontSize = 24,
                                 .wrapMode = CLAY_TEXT_WRAP_WORDS,
                            })
                        );}
                }
            }
            // Tall child that gets clipped by parent
            CLAY(
                CLAY_ID("TallChild"),
                {
                    .layout = {
                        .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(300) }
                    },
                    .backgroundColor = { 255, 255, 60, 255 },
                    .cornerRadius = { 12, 12, 12, 12 }
                }
            ) {
                CLAY(
                    CLAY_ID("TestImage"),
                    {
                        .layout = {
                            .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(100) }
                        },
                        .cornerRadius = { 8, 8, 8, 8 },
                        .image = { .imageData = ImageLoader::Get("images/tasbih.png") }
                    }
                ) {}
            }

        }
    }
    Clay_RenderCommandArray commands = Clay_EndLayout(0);
    ImageLoader::FixAspectRatios(commands);

    g_clayRenderer.Render(commands);

    g_renderer.EndFrame();
}
