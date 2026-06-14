#pragma once
#include <GLES3/gl3.h>
#include <android/asset_manager.h>
#include "../../third_party/clay/clay.h"
#include <vector>

class FontAtlas;

struct ScissorRect {
    int x, y, w, h;
};

class ClayRenderer {
public:
    void Init(AAssetManager* mgr);
    void Destroy();
    void SetResolution(int width, int height);
    void SetFontAtlas(FontAtlas* atlas);
    void SetDensity(float d);
    void Render(const Clay_RenderCommandArray& commands);

private:
    void InitTextProgram(AAssetManager* mgr);
    void DrawRect(float x, float y, float w, float h, Clay_Color color,
                  Clay_CornerRadius corners);
    void DrawImage(float x, float y, float w, float h, GLuint texture,
                   Clay_Color tint, Clay_CornerRadius corners);
    void DrawBorder(float x, float y, float w, float h, const Clay_BorderRenderData& border);
    void DrawText(const Clay_RenderCommand& cmd);

    void PushScissor(int x, int y, int w, int h);
    void PopScissor();

    // Rect / image / border program
    GLuint program = 0;
    GLuint vao = 0;
    GLuint vbo = 0;
    GLint uResolution = -1;
    GLint uColor = -1;
    GLint uRect = -1;
    GLint uCorners = -1;
    GLint uTexture = -1;
    GLint uUseTexture = -1;

    // Text program
    GLuint textProgram = 0;
    GLuint textVAO = 0;
    GLuint textVBO = 0;
    GLint tuViewport = -1;
    GLint tuColor = -1;
    GLint tuFontAtlas = -1;

    FontAtlas* fontAtlas = nullptr;
    int width = 0;
    int height = 0;
    float density = 1.0f;
    std::vector<ScissorRect> scissorStack;
};
