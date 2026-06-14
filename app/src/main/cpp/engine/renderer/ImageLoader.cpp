#include "ImageLoader.hpp"
#include <android/log.h>
#include <vector>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "IMGLOADER", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "IMGLOADER", __VA_ARGS__)

#include "../../third_party/lodepng/lodepng.h"

AAssetManager* ImageLoader::sMgr = nullptr;
std::unordered_map<std::string, ImageInfo> ImageLoader::sCache;
std::unordered_map<GLuint, std::pair<int, int>> ImageLoader::sTexToDims;

void ImageLoader::Init(AAssetManager* mgr)
{
    sMgr = mgr;
}

void* ImageLoader::Get(const char* path)
{
    auto it = sCache.find(path);
    if (it != sCache.end())
        return (void*)(uintptr_t)it->second.texture;

    ImageInfo info;
    info.texture = Load(path, &info.width, &info.height);
    if (info.texture) {
        sCache[path] = info;
        sTexToDims[info.texture] = { info.width, info.height };
    }
    return (void*)(uintptr_t)info.texture;
}

int ImageLoader::GetWidth(const char* path)
{
    auto it = sCache.find(path);
    if (it == sCache.end()) return 0;
    return it->second.width;
}

int ImageLoader::GetHeight(const char* path)
{
    auto it = sCache.find(path);
    if (it == sCache.end()) return 0;
    return it->second.height;
}

GLuint ImageLoader::Load(const char* path, int* outW, int* outH)
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

    if (outW) *outW = (int)w;
    if (outH) *outH = (int)h;
    return tex;
}

void ImageLoader::FixAspectRatios(Clay_RenderCommandArray& commands)
{
    for (int32_t i = 0; i < commands.length; ++i) {
        Clay_RenderCommand& cmd = commands.internalArray[i];
        if (cmd.commandType != CLAY_RENDER_COMMAND_TYPE_IMAGE) continue;
        if (!cmd.renderData.image.imageData) continue;

        GLuint tex = (GLuint)(uintptr_t)cmd.renderData.image.imageData;
        auto it = sTexToDims.find(tex);
        if (it == sTexToDims.end()) continue;

        float imgW = (float)it->second.first;
        float imgH = (float)it->second.second;
        if (imgW == 0) continue;

        float w = cmd.boundingBox.width;
        cmd.boundingBox.height = w * (imgH / imgW);
    }
}

void ImageLoader::Destroy()
{
    for (auto& kv : sCache) {
        if (kv.second.texture)
            glDeleteTextures(1, &kv.second.texture);
    }
    sCache.clear();
    sTexToDims.clear();
}
