#pragma once
#include <GLES3/gl3.h>

class Renderer {
public:
    void Init();
    void Resize(int width, int height);
    void BeginFrame();
    void EndFrame();
    void DrawRect(float x, float y, float w, float h);

private:
    void Destroy();

    GLuint program = 0;
    GLuint vbo = 0;
    GLuint vao = 0;
};