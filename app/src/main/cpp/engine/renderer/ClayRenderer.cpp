#include "ClayRenderer.hpp"
#include "FontAtlas.hpp"
#include <android/log.h>
#include <string>
#include <vector>
#include <cstdint>
#include <algorithm>

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

void ClayRenderer::InitTextProgram(AAssetManager* mgr)
{
    std::string vs = ReadAsset(mgr, "shaders/text.vert.glsl");
    std::string fs = ReadAsset(mgr, "shaders/text.frag.glsl");
    if (vs.empty() || fs.empty()) return;

    const char* vss = vs.c_str();
    const char* fss = fs.c_str();

    GLuint v = Compile(GL_VERTEX_SHADER, vss);
    GLuint f = Compile(GL_FRAGMENT_SHADER, fss);
    if (!v || !f) {
        if (v) glDeleteShader(v);
        if (f) glDeleteShader(f);
        return;
    }

    textProgram = Link(v, f);
    glDeleteShader(v);
    glDeleteShader(f);
    if (!textProgram) return;

    tuViewport = glGetUniformLocation(textProgram, "uViewport");
    tuColor = glGetUniformLocation(textProgram, "uColor");
    tuFontAtlas = glGetUniformLocation(textProgram, "uFontAtlas");
    tuClipRect = glGetUniformLocation(textProgram, "uClipRect");
    tuClipCorners = glGetUniformLocation(textProgram, "uClipCorners");
    tuClipEnabled = glGetUniformLocation(textProgram, "uClipEnabled");

    glGenVertexArrays(1, &textVAO);
    glBindVertexArray(textVAO);

    glGenBuffers(1, &textVBO);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 1024 * 4, nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    LOGI("text program ok (%u)", textProgram);
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
    uTexture = glGetUniformLocation(program, "uTexture");
    uUseTexture = glGetUniformLocation(program, "uUseTexture");
    uClipRect = glGetUniformLocation(program, "uClipRect");
    uClipCorners = glGetUniformLocation(program, "uClipCorners");
    uClipEnabled = glGetUniformLocation(program, "uClipEnabled");

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 2, nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

    InitTextProgram(mgr);

    LOGI("init ok (program=%u)", program);
}

void ClayRenderer::Destroy()
{
    if (program) glDeleteProgram(program);
    if (vbo) glDeleteBuffers(1, &vbo);
    if (vao) glDeleteVertexArrays(1, &vao);
    program = 0; vbo = 0; vao = 0;
    uResolution = uColor = uRect = uCorners = uTexture = uUseTexture = -1;
    uClipRect = uClipCorners = uClipEnabled = -1;

    if (textProgram) glDeleteProgram(textProgram);
    if (textVBO) glDeleteBuffers(1, &textVBO);
    if (textVAO) glDeleteVertexArrays(1, &textVAO);
    textProgram = 0; textVBO = 0; textVAO = 0;
    tuViewport = tuColor = tuFontAtlas = -1;
    tuClipRect = tuClipCorners = tuClipEnabled = -1;

    fontAtlas = nullptr;
    scissorStack.clear();
}

void ClayRenderer::SetResolution(int w, int h)
{
    width = w; height = h;
}

void ClayRenderer::SetFontAtlas(FontAtlas* atlas)
{
    fontAtlas = atlas;
}

void ClayRenderer::SetDensity(float d)
{
    density = d;
}

void ClayRenderer::PushScissor(int x, int y, int w, int h, Clay_CornerRadius cornerRadius)
{
    if (!scissorStack.empty()) {
        const ScissorRect& p = scissorStack.back();
        int r1 = x + w, b1 = y + h;
        int r0 = p.x + p.w, b0 = p.y + p.h;
        x = std::max(x, p.x);
        y = std::max(y, p.y);
        r1 = std::min(r1, r0);
        b1 = std::min(b1, b0);
        w = std::max(0, r1 - x);
        h = std::max(0, b1 - y);
    }
    scissorStack.push_back({ x, y, w, h, cornerRadius });
    glEnable(GL_SCISSOR_TEST);
    glScissor(x, height - y - h, w, h);
}

void ClayRenderer::PopScissor()
{
    if (!scissorStack.empty())
        scissorStack.pop_back();

    if (scissorStack.empty()) {
        glDisable(GL_SCISSOR_TEST);
    } else {
        const ScissorRect& p = scissorStack.back();
        glScissor(p.x, height - p.y - p.h, p.w, p.h);
    }
}

void ClayRenderer::ApplyClipUniforms(bool isText)
{
    GLint locRect    = isText ? tuClipRect    : uClipRect;
    GLint locCorners = isText ? tuClipCorners : uClipCorners;
    GLint locEnabled = isText ? tuClipEnabled : uClipEnabled;

    if (scissorStack.empty()) {
        if (locEnabled >= 0) glUniform1i(locEnabled, 0);
        return;
    }

    const ScissorRect& top = scissorStack.back();

    if (locEnabled >= 0) glUniform1i(locEnabled, 1);
    if (locRect >= 0)
        glUniform4f(locRect, (float)top.x, (float)top.y, (float)top.w, (float)top.h);
    if (locCorners >= 0)
        glUniform4f(locCorners,
                    top.cornerRadius.topLeft * density, top.cornerRadius.topRight * density,
                    top.cornerRadius.bottomRight * density, top.cornerRadius.bottomLeft * density);
}

void ClayRenderer::Render(const Clay_RenderCommandArray& commands)
{
    if (!program) return;

    for (int32_t i = 0; i < commands.length; ++i) {
        const Clay_RenderCommand& cmd = commands.internalArray[i];
        const Clay_BoundingBox& bb = cmd.boundingBox;

        switch (cmd.commandType) {
            case CLAY_RENDER_COMMAND_TYPE_RECTANGLE:
                glUseProgram(program);
                glBindVertexArray(vao);
                if (uUseTexture >= 0)
                    glUniform1i(uUseTexture, 0);
                if (uResolution >= 0)
                    glUniform2f(uResolution, (float)width, (float)height);
                ApplyClipUniforms(false);
                DrawRect(bb.x, bb.y, bb.width, bb.height,
                         cmd.renderData.rectangle.backgroundColor,
                         cmd.renderData.rectangle.cornerRadius);
                break;

            case CLAY_RENDER_COMMAND_TYPE_IMAGE: {
                GLuint tex = (GLuint)(uintptr_t)cmd.renderData.image.imageData;
                if (!tex) break;
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glUseProgram(program);
                glBindVertexArray(vao);
                if (uUseTexture >= 0)
                    glUniform1i(uUseTexture, 1);
                if (uTexture >= 0)
                    glUniform1i(uTexture, 0);
                if (uResolution >= 0)
                    glUniform2f(uResolution, (float)width, (float)height);
                ApplyClipUniforms(false);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, tex);
                DrawImage(bb.x, bb.y, bb.width, bb.height, tex,
                          cmd.renderData.image.backgroundColor,
                          cmd.renderData.image.cornerRadius);
                glDisable(GL_BLEND);
                break;
            }

            case CLAY_RENDER_COMMAND_TYPE_BORDER:
                glUseProgram(program);
                glBindVertexArray(vao);
                if (uUseTexture >= 0)
                    glUniform1i(uUseTexture, 0);
                if (uResolution >= 0)
                    glUniform2f(uResolution, (float)width, (float)height);
                ApplyClipUniforms(false);
                DrawBorder(bb.x, bb.y, bb.width, bb.height,
                           cmd.renderData.border);
                break;

            case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START: {
                int sx = (int)(bb.x * density);
                int sy = (int)(bb.y * density);
                int sw = (int)(bb.width * density);
                int sh = (int)(bb.height * density);

                Clay_CornerRadius radius = {0, 0, 0, 0};

                // Look ahead for this element's own RECTANGLE command
                for (int32_t j = i + 1; j < commands.length; ++j) {
                    const Clay_RenderCommand& future = commands.internalArray[j];
                    if (future.commandType == CLAY_RENDER_COMMAND_TYPE_SCISSOR_END)
                        break; // left this element's subtree without finding it
                    if (future.commandType == CLAY_RENDER_COMMAND_TYPE_RECTANGLE &&
                        future.id == cmd.id) {
                        radius = future.renderData.rectangle.cornerRadius;
                        break;
                    }
                }

                PushScissor(sx, sy, sw, sh, radius);
                break;
            }

            case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END:
                PopScissor();
                break;

            case CLAY_RENDER_COMMAND_TYPE_TEXT:
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                DrawText(cmd);
                glDisable(GL_BLEND);
                break;

            default:
                break;
        }
    }
}

void ClayRenderer::DrawRect(float x, float y, float w, float h, Clay_Color color,
                            Clay_CornerRadius corners)
{
    float rx = x * density, ry = y * density;
    float rw = w * density, rh = h * density;

    float verts[] = {
            rx,     ry,
            rx + rw, ry,
            rx + rw, ry + rh,
            rx,     ry,
            rx + rw, ry + rh,
            rx,     ry + rh
    };

    if (uColor >= 0)
        glUniform4f(uColor, color.r / 255.0f, color.g / 255.0f,
                    color.b / 255.0f, color.a / 255.0f);
    if (uRect >= 0)
        glUniform4f(uRect, rx, ry, rw, rh);
    if (uCorners >= 0)
        glUniform4f(uCorners, corners.topLeft * density, corners.topRight * density,
                    corners.bottomRight * density, corners.bottomLeft * density);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void ClayRenderer::DrawImage(float x, float y, float w, float h, GLuint texture,
                             Clay_Color tint, Clay_CornerRadius corners)
{
    float rx = x * density, ry = y * density;
    float rw = w * density, rh = h * density;

    float verts[] = {
            rx,     ry,
            rx + rw, ry,
            rx + rw, ry + rh,
            rx,     ry,
            rx + rw, ry + rh,
            rx,     ry + rh
    };

    if (uColor >= 0)
        glUniform4f(uColor, tint.r / 255.0f, tint.g / 255.0f,
                    tint.b / 255.0f, tint.a / 255.0f);
    if (uRect >= 0)
        glUniform4f(uRect, rx, ry, rw, rh);
    if (uCorners >= 0)
        glUniform4f(uCorners, corners.topLeft * density, corners.topRight * density,
                    corners.bottomRight * density, corners.bottomLeft * density);

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

void ClayRenderer::DrawText(const Clay_RenderCommand& cmd)
{
    if (!textProgram || !fontAtlas) return;

    const Clay_TextRenderData& textData = cmd.renderData.text;
    Clay_StringSlice str = textData.stringContents;
    Clay_Color c = textData.textColor;

    fontAtlas->SetSize((float)textData.fontSize * density);

    std::vector<float> verts;
    verts.reserve(256 * 4 * 4);

    float x0 = cmd.boundingBox.x * density;
    float y0 = cmd.boundingBox.y * density;
    float baselineX = x0;
    float baselineY = y0 + fontAtlas->GetAscender();

    uint32_t codepoint;
    int pos = 0;
    while (pos < str.length) {
        unsigned char lead = (unsigned char)str.chars[pos];
        if (lead < 0x80) {
            codepoint = lead;
            pos += 1;
        } else if (lead < 0xE0) {
            codepoint = lead & 0x1F;
            if (pos + 1 < str.length)
                codepoint = (codepoint << 6) | ((unsigned char)str.chars[pos + 1] & 0x3F);
            pos += 2;
        } else if (lead < 0xF0) {
            codepoint = lead & 0x0F;
            if (pos + 2 < str.length) {
                codepoint = (codepoint << 6) | ((unsigned char)str.chars[pos + 1] & 0x3F);
                codepoint = (codepoint << 6) | ((unsigned char)str.chars[pos + 2] & 0x3F);
            }
            pos += 3;
        } else {
            codepoint = lead & 0x07;
            if (pos + 3 < str.length) {
                codepoint = (codepoint << 6) | ((unsigned char)str.chars[pos + 1] & 0x3F);
                codepoint = (codepoint << 6) | ((unsigned char)str.chars[pos + 2] & 0x3F);
                codepoint = (codepoint << 6) | ((unsigned char)str.chars[pos + 3] & 0x3F);
            }
            pos += 4;
        }

        const GlyphInfo* g = fontAtlas->GetGlyph(codepoint);
        if (!g) continue;

        float gx = baselineX + g->bearingX;
        float gy = baselineY - g->bearingY;
        float gw = g->width;
        float gh = g->height;

        if (gw > 0 && gh > 0) {
            float q[] = {
                    gx,     gy,     g->u0, g->v0,
                    gx+gw,  gy,     g->u1, g->v0,
                    gx+gw,  gy+gh,  g->u1, g->v1,
                    gx,     gy,     g->u0, g->v0,
                    gx+gw,  gy+gh,  g->u1, g->v1,
                    gx,     gy+gh,  g->u0, g->v1,
            };
            verts.insert(verts.end(), q, q + 24);
        }

        baselineX += g->advanceX;
    }

    if (verts.empty()) return;

    glUseProgram(textProgram);
    glBindVertexArray(textVAO);

    ApplyClipUniforms(true);

    if (tuViewport >= 0)
        glUniform2f(tuViewport, (float)width, (float)height);
    if (tuColor >= 0)
        glUniform4f(tuColor, c.r / 255.0f, c.g / 255.0f,
                    c.b / 255.0f, c.a / 255.0f);
    if (tuFontAtlas >= 0)
        glUniform1i(tuFontAtlas, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fontAtlas->GetTexture());

    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, verts.size() * sizeof(float), verts.data());

    glDrawArrays(GL_TRIANGLES, 0, (int)(verts.size() / 4));
}