#pragma once
#include <GLES3/gl3.h>
#include <android/asset_manager.h>
#include "../../third_party/clay/clay.h"

class ClayRenderer {
public:
    void Init(AAssetManager* mgr);
    void Destroy();
    void SetResolution(int width, int height);
    void Render(const Clay_RenderCommandArray& commands);

private:
    void DrawRect(float x, float y, float w, float h, Clay_Color color,
                  Clay_CornerRadius corners);
    void DrawBorder(float x, float y, float w, float h, const Clay_BorderRenderData& border);

    GLuint program = 0;
    GLuint vao = 0;
    GLuint vbo = 0;
    GLint uResolution = -1;
    GLint uColor = -1;
    GLint uRect = -1;
    GLint uCorners = -1;
    int width = 0;
    int height = 0;
};
