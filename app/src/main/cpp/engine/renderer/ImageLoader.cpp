#include "ImageLoader.hpp"
#include <android/log.h>
#include <vector>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "IMGLOADER", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "IMGLOADER", __VA_ARGS__)

#include "../../third_party/lodepng/lodepng.h"

AAssetManager* ImageLoader::sMgr = nullptr;
std::unordered_map<std::string, GLuint> ImageLoader::sCache;

void ImageLoader::Init(AAssetManager* mgr)
{
    sMgr = mgr;
}

void* ImageLoader::Get(const char* path)
{
    auto it = sCache.find(path);
    if (it != sCache.end())
        return (void*)(uintptr_t)it->second;

    GLuint tex = Load(path);
    if (tex)
        sCache[path] = tex;
    return (void*)(uintptr_t)tex;
}

GLuint ImageLoader::Load(const char* path)
{
    if (!sMgr) {
        LOGE("ImageLoader not initialised with AAssetManager");
        return 0;
    }

    AAsset* asset = AAssetManager_open(sMgr, path, AASSET_MODE_BUFFER);
    if (!asset) {
        LOGE("failed to open image asset: %s", path);
        return 0;
    }

    const void* buf = AAsset_getBuffer(asset);
    off_t len = AAsset_getLength(asset);

    std::vector<unsigned char> pixels;
    unsigned w = 0, h = 0;
    unsigned err = lodepng::decode(pixels, w, h,
        static_cast<const unsigned char*>(buf), (size_t)len);
    AAsset_close(asset);

    if (err) {
        LOGE("lodepng decode failed for %s: %s", path, lodepng_error_text(err));
        return 0;
    }

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (int)w, (int)h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    LOGI("loaded texture %u from %s (%ux%u)", tex, path, w, h);
    return tex;
}

void ImageLoader::Destroy()
{
    for (auto& kv : sCache) {
        if (kv.second)
            glDeleteTextures(1, &kv.second);
    }
    sCache.clear();
}
