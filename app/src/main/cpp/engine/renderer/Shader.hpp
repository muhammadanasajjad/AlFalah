//#pragma once
//#include <GLES3/gl3.h>
//#include <android/asset_manager.h>
//
//class Renderer {
//public:
//    void Init();
//    void Resize(int width, int height);
//    void BeginFrame();
//    void EndFrame();
//    void DrawRect(float x, float y, float w, float h,
//                  float tl = 0, float tr = 0, float br = 0, float bl = 0);
//    void SetAssetManager(AAssetManager* mgr);
//
//private:
//    void Destroy();
//
//    AAssetManager* assetManager = nullptr;
//    int width = 0;
//    int height = 0;
//    GLuint program = 0;
//    GLuint vbo = 0;
//    GLuint vao = 0;
//    GLint uResolution = -1;
//    GLint uColor = -1;
//    GLint uRect = -1;
//    GLint uCorners = -1;
//};