#pragma once
#include <GLES3/gl3.h>
#include <android/asset_manager.h>
#include <string>
#include <unordered_map>
#include "../../third_party/clay/clay.h"

struct ImageInfo {
    GLuint texture = 0;
    int width = 0;
    int height = 0;
};

class ImageLoader {
public:
    static void Init(AAssetManager* mgr);
    static void* Get(const char* path);
    static int GetWidth(const char* path);
    static int GetHeight(const char* path);
    static void FixAspectRatios(Clay_RenderCommandArray& commands);
    static void Destroy();

private:
    static GLuint Load(const char* path, int* outW = nullptr, int* outH = nullptr);
    static AAssetManager* sMgr;
    static std::unordered_map<std::string, ImageInfo> sCache;
    static std::unordered_map<GLuint, std::pair<int, int>> sTexToDims;
};
