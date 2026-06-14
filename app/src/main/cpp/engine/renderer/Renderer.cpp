#include "Renderer.hpp"
#include <android/log.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "RENDERER", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "RENDERER", __VA_ARGS__)

static const char* vs = R"(#version 300 es
layout(location = 0) in vec2 aPos;

void main()
{
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

static const char* fs = R"(#version 300 es
precision mediump float;
out vec4 color;

void main()
{
    color = vec4(0.2, 0.8, 0.2, 1.0);
}
)";

static GLuint Compile(GLenum type, const char* src)
{
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);

    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(s, sizeof(log), nullptr, log);
        LOGE("shader compile error (%s): %s",
             type == GL_VERTEX_SHADER ? "vertex" : "fragment", log);
        glDeleteShader(s);
        return 0;
    }
    return s;
}

static GLuint Link(GLuint v, GLuint f)
{
    GLuint p = glCreateProgram();
    glAttachShader(p, v);
    glAttachShader(p, f);
    glLinkProgram(p);

    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(p, sizeof(log), nullptr, log);
        LOGE("program link error: %s", log);
        glDeleteProgram(p);
        return 0;
    }
    return p;
}

void Renderer::Destroy()
{
    if (program) glDeleteProgram(program);
    if (vbo) glDeleteBuffers(1, &vbo);
    if (vao) glDeleteVertexArrays(1, &vao);
    program = 0;
    vbo = 0;
    vao = 0;
}

void Renderer::Init()
{
    Destroy();

    GLuint v = Compile(GL_VERTEX_SHADER, vs);
    GLuint f = Compile(GL_FRAGMENT_SHADER, fs);
    if (!v || !f) {
        if (v) glDeleteShader(v);
        if (f) glDeleteShader(f);
        LOGE("shader compilation failed");
        return;
    }

    program = Link(v, f);
    glDeleteShader(v);
    glDeleteShader(f);

    if (!program) {
        LOGE("program linking failed");
        return;
    }

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 2, nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

    LOGI("init ok (program=%u)", program);
}

void Renderer::Resize(int width, int height)
{
    glViewport(0, 0, width, height);
}

void Renderer::BeginFrame()
{
    glClearColor(0.08f, 0.08f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (!program) return;
    glUseProgram(program);
    glBindVertexArray(vao);
}

void Renderer::EndFrame()
{
}

void Renderer::DrawRect(float x, float y, float w, float h)
{
    if (!program) return;

    float verts[] = {
            x,     y,
            x + w, y,
            x + w, y + h,

            x,     y,
            x + w, y + h,
            x,     y + h
    };

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);

    glDrawArrays(GL_TRIANGLES, 0, 6);
}
