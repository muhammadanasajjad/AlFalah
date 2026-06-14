#include "ClayRenderer.hpp"
#include <android/log.h>
#include <string>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "CLAY_RENDERER", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "CLAY_RENDERER", __VA_ARGS__)

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

void ClayRenderer::Init(AAssetManager* mgr)
{
    Destroy();

    if (!mgr) {
        LOGE("no asset manager");
        return;
    }

    std::string vsSrc = ReadAsset(mgr, "shaders/clay.vert.glsl");
    std::string fsSrc = ReadAsset(mgr, "shaders/clay.frag.glsl");
    if (vsSrc.empty() || fsSrc.empty()) {
        LOGE("failed to load clay shader sources");
        return;
    }

    const char* vss = vsSrc.c_str();
    const char* fss = fsSrc.c_str();

    GLuint v = Compile(GL_VERTEX_SHADER, vss);
    GLuint f = Compile(GL_FRAGMENT_SHADER, fss);
    if (!v || !f) {
        if (v) glDeleteShader(v);
        if (f) glDeleteShader(f);
        LOGE("clay shader compilation failed");
        return;
    }

    program = Link(v, f);
    glDeleteShader(v);
    glDeleteShader(f);

    if (!program) {
        LOGE("clay program linking failed");
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

    LOGI("init ok (program=%u)", program);
}

void ClayRenderer::Destroy()
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

void ClayRenderer::SetResolution(int w, int h)
{
    width = w;
    height = h;
}

static void SetScissor(int height, Clay_BoundingBox bbox)
{
    glScissor((int)bbox.x, height - (int)(bbox.y + bbox.height),
              (int)bbox.width, (int)bbox.height);
}

void ClayRenderer::Render(const Clay_RenderCommandArray& commands)
{
    if (!program) return;

    glUseProgram(program);
    glBindVertexArray(vao);

    if (uResolution >= 0)
        glUniform2f(uResolution, (float)width, (float)height);

    for (int32_t i = 0; i < commands.length; ++i) {
        const Clay_RenderCommand& cmd = commands.internalArray[i];
        const Clay_BoundingBox& bb = cmd.boundingBox;

        switch (cmd.commandType) {
        case CLAY_RENDER_COMMAND_TYPE_RECTANGLE:
            DrawRect(bb.x, bb.y, bb.width, bb.height,
                     cmd.renderData.rectangle.backgroundColor,
                     cmd.renderData.rectangle.cornerRadius);
            break;

        case CLAY_RENDER_COMMAND_TYPE_BORDER:
            DrawBorder(bb.x, bb.y, bb.width, bb.height,
                       cmd.renderData.border);
            break;

        case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START:
            glEnable(GL_SCISSOR_TEST);
            SetScissor(height, bb);
            break;

        case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END:
            glDisable(GL_SCISSOR_TEST);
            break;

        default:
            break;
        }
    }
}

void ClayRenderer::DrawRect(float x, float y, float w, float h, Clay_Color color,
                            Clay_CornerRadius corners)
{
    float verts[] = {
        x,     y,
        x + w, y,
        x + w, y + h,

        x,     y,
        x + w, y + h,
        x,     y + h
    };

    if (uColor >= 0)
        glUniform4f(uColor, color.r / 255.0f, color.g / 255.0f,
                            color.b / 255.0f, color.a / 255.0f);
    if (uRect >= 0)
        glUniform4f(uRect, x, y, w, h);
    if (uCorners >= 0)
        glUniform4f(uCorners, corners.topLeft, corners.topRight,
                    corners.bottomRight, corners.bottomLeft);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);

    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void ClayRenderer::DrawBorder(float x, float y, float w, float h,
                              const Clay_BorderRenderData& border)
{
    Clay_Color bc = border.color;
    Clay_CornerRadius zero = {};

    if (border.width.top > 0)
        DrawRect(x, y, w, border.width.top, bc, zero);

    if (border.width.bottom > 0)
        DrawRect(x, y + h - border.width.bottom, w, border.width.bottom, bc, zero);

    if (border.width.left > 0)
        DrawRect(x, y + border.width.top, border.width.left,
                 h - border.width.top - border.width.bottom, bc, zero);

    if (border.width.right > 0)
        DrawRect(x + w - border.width.right, y + border.width.top,
                 border.width.right, h - border.width.top - border.width.bottom, bc, zero);
}
