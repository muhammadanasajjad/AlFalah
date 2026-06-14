#include <jni.h>
#include "engine/renderer/Renderer.hpp"

static Renderer g_renderer;

extern "C" JNIEXPORT void JNICALL
Java_com_primaveradev_alfalah_MainActivity_nativeOnSurfaceCreated(
        JNIEnv*, jobject)
{
    g_renderer.Init();
}

extern "C" JNIEXPORT void JNICALL
Java_com_primaveradev_alfalah_MainActivity_nativeOnSurfaceChanged(
        JNIEnv*, jobject, jint w, jint h)
{
    g_renderer.Resize(w, h);
}

extern "C" JNIEXPORT void JNICALL
Java_com_primaveradev_alfalah_MainActivity_nativeOnDrawFrame(
        JNIEnv*, jobject)
{
    g_renderer.BeginFrame();

    // TEST RECTANGLE (ignore Clay first)
//    g_renderer.DrawRect(100, 100, 300, 150);
    g_renderer.DrawRect(-0.5f, -0.5f, 1.0f, 1.0f);

    g_renderer.EndFrame();
}
