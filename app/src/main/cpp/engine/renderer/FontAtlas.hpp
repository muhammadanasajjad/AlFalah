#pragma once
#include <GLES3/gl3.h>
#include <android/asset_manager.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <hb.h>
#include <hb-ft.h>
#include <unordered_map>
#include <string>
#include <vector>

struct GlyphInfo {
    float advanceX;
    float bearingX;
    float bearingY;
    float width;
    float height;
    float u0, v0, u1, v1;
};

class FontAtlas {
public:
    ~FontAtlas();
    bool Load(AAssetManager* mgr, const char* path, float size);
    bool LoadFromFile(const char* filePath, float size);
    void Destroy();
    void SetSize(float size);

    const GlyphInfo* GetGlyph(uint32_t codepoint);
    float GetAdvance(uint32_t codepoint);
    GLuint GetTexture() const { return texture; }
    hb_font_t* GetHbFont() const { return hbFont; }
    float GetScale() const { return scale; }
    float GetAscender() const { return ascender; }
    float GetDescender() const { return descender; }
    float GetLineHeight() const { return lineHeight; }

private:
    bool Init(const std::vector<char>& fontData, const char* label);
    GlyphInfo* RenderGlyph(uint32_t codepoint);

    FT_Library ft = nullptr;
    FT_Face face = nullptr;
    hb_font_t* hbFont = nullptr;

    float fontSize = 0;
    float scale = 1.0f;
    float ascender = 0;
    float descender = 0;
    float lineHeight = 0;

    GLuint texture = 0;
    int atlasW = 2048;
    int atlasH = 2048;
    int cursorX = 0;
    int cursorY = 0;
    int rowH = 0;
    uint8_t* atlasData = nullptr;

    std::unordered_map<uint32_t, GlyphInfo> glyphs;
    std::vector<char> fontData;
};
