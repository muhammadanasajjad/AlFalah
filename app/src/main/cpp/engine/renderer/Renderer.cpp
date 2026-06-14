#include "Renderer.hpp"
#include <android/log.h>
#include <string>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "RENDERER", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "RENDERER", __VA_ARGS__)

static std::string ReadAsset(AAssetManager* mgr, const char* path)
{
    AAsset* asset = AAssetManager_open(mgr, path, AASSET_MODE_BUFFER);
    if (!asset) {
        LOGE("failed to open asset: %s", path);
        return {};
    }
    const void* data = AAsset_getBuffer(asset);
    off_t len = AAsset_getLength(asset);
    std::string result(static_cast<const char*>(data), len);
    AAsset_close(asset);
    return result;
}

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
    uResolution = -1;
    uColor = -1;
    uRect = -1;
    uCorners = -1;
}

void Renderer::SetAssetManager(AAssetManager* mgr)
{
    assetManager = mgr;
}

void Renderer::Init()
{
    Destroy();

    if (!assetManager) {
        LOGE("no asset manager set");
        return;
    }

    std::string vsSrc = ReadAsset(assetManager, "shaders/rect.vert.glsl");
    std::string fsSrc = ReadAsset(assetManager, "shaders/rect.frag.glsl");
    if (vsSrc.empty() || fsSrc.empty()) {
        LOGE("failed to load shader sources");
        return;
    }

    const char* vss = vsSrc.c_str();
    const char* fss = fsSrc.c_str();

    GLuint v = Compile(GL_VERTEX_SHADER, vss);
    GLuint f = Compile(GL_FRAGMENT_SHADER, fss);
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

    uResolution = glGetUniformLocation(program, "uResolution");
    uColor = glGetUniformLocation(program, "uColor");
    uRect = glGetUniformLocation(program, "uRect");
    uCorners = glGetUniformLocation(program, "uCorners");

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 2, nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

    LOGI("init ok (program=%u, uResolution=%d)", program, uResolution);
}

void Renderer::Resize(int width, int height)
{
    this->width = width;
    this->height = height;
    glViewport(0, 0, width, height);
}

void Renderer::BeginFrame()
{
    glClearColor(0.08f, 0.08f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (!program) return;
    glUseProgram(program);
    glBindVertexArray(vao);

    if (uResolution >= 0)
        glUniform2f(uResolution, (float)width, (float)height);
}

void Renderer::EndFrame()
{
}

void Renderer::DrawRect(float x, float y, float w, float h,
                        float tl, float tr, float br, float bl)
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

    if (uColor >= 0)
        glUniform4f(uColor, 0.2f, 0.8f, 0.2f, 1.0f);
    if (uRect >= 0)
        glUniform4f(uRect, x, y, w, h);
    if (uCorners >= 0)
        glUniform4f(uCorners, tl, tr, br, bl);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);

    glDrawArrays(GL_TRIANGLES, 0, 6);
}
