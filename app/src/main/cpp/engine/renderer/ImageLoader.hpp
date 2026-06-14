#pragma once
#include <GLES3/gl3.h>
#include <android/asset_manager.h>
#include <string>
#include <unordered_map>

class ImageLoader {
public:
    static void Init(AAssetManager* mgr);
    static void* Get(const char* path);
    static void Destroy();

private:
    static GLuint Load(const char* path);
    static AAssetManager* sMgr;
    static std::unordered_map<std::string, GLuint> sCache;
};
