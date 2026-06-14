#include <jni.h>
#include <android/asset_manager_jni.h>
#include <malloc.h>
#include "engine/renderer/Renderer.hpp"
#include "engine/renderer/ClayRenderer.hpp"
#include "third_party/clay/clay.h"

static Renderer g_renderer;
static ClayRenderer g_clayRenderer;
static uint64_t g_clayArenaSize = 0;
static void* g_clayArena = nullptr;

static Clay_Dimensions MeasureText(Clay_StringSlice, Clay_TextElementConfig*, void*)
{
    return { 0, 0 };
}

extern "C" JNIEXPORT void JNICALL
Java_com_primaveradev_alfalah_MainActivity_nativeOnSurfaceCreated(
        JNIEnv* env, jobject, jobject assetManager)
{
    AAssetManager* mgr = AAssetManager_fromJava(env, assetManager);

    g_renderer.SetAssetManager(mgr);
    g_renderer.Init();

    g_clayRenderer.Init(mgr);

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
    Clay_SetLayoutDimensions((Clay_Dimensions){ (float)w, (float)h });
}

extern "C" JNIEXPORT void JNICALL
Java_com_primaveradev_alfalah_MainActivity_nativeOnDrawFrame(
        JNIEnv*, jobject)
{
    g_renderer.BeginFrame();

    Clay_BeginLayout();
    {
        CLAY(
            CLAY_ID("Panel"),
            {
                .layout = {
                    .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_FIXED(200) },
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    .padding = CLAY_PADDING_ALL(16)
                },
                .backgroundColor = { 255, 60, 80, 255 }
            }
        ) {
            CLAY(
                CLAY_ID("Child"),
                {
                    .layout = {
                        .sizing = { CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0) }
                    },
                    .backgroundColor = { 120, 80, 60, 255 },
                    .cornerRadius = { 100, 100, 25, 25 }
                }
            ) {}
            CLAY_TEXT(
                    CLAY_STRING("Hello World"),
                    CLAY_TEXT_CONFIG({
                                             .textColor = { 255, 255, 255, 255 },
                                             .fontSize = 18,
                                             .fontId = 0
                                     })
            );
        }
    }
    Clay_RenderCommandArray commands = Clay_EndLayout(0);

    g_clayRenderer.Render(commands);

    g_renderer.EndFrame();
}
