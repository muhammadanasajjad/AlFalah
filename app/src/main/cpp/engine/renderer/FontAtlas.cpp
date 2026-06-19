#include "FontAtlas.hpp"
#include <android/log.h>
#include <vector>
#include <cstring>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "FONT", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "FONT", __VA_ARGS__)

FontAtlas::~FontAtlas() { Destroy(); }

bool FontAtlas::Init(const std::vector<char>& data, const char* label)
{
    this->fontData = data;
    if (FT_New_Memory_Face(ft, (const FT_Byte*)this->fontData.data(),
                           this->fontData.size(), 0, &face)) {
        LOGE("failed to create font face from %s", label);
        return false;
    }

    FT_Set_Pixel_Sizes(face, 0, (FT_UInt)fontSize);
    scale = (float)face->size->metrics.y_scale / 65536.0f;
    ascender = (float)face->size->metrics.ascender / 64.0f;
    descender = (float)face->size->metrics.descender / 64.0f;
    lineHeight = (float)face->size->metrics.height / 64.0f;

    hbFont = hb_ft_font_create(face, nullptr);

    atlasData = new uint8_t[atlasW * atlasH]();

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, atlasW, atlasH, 0,
                 GL_RED, GL_UNSIGNED_BYTE, atlasData);

    LOGI("loaded font '%s' size=%.0f ascender=%.1f descender=%.1f line=%.1f",
         label, fontSize, ascender, descender, lineHeight);
    return true;
}

bool FontAtlas::Load(AAssetManager* mgr, const char* path, float size)
{
    Destroy();
    fontSize = size;

    if (FT_Init_FreeType(&ft)) {
        LOGE("failed to init FreeType");
        return false;
    }

    AAsset* asset = AAssetManager_open(mgr, path, AASSET_MODE_BUFFER);
    if (!asset) {
        LOGE("failed to open font: %s", path);
        return false;
    }

    const void* data = AAsset_getBuffer(asset);
    off_t len = AAsset_getLength(asset);

    std::vector<char> fontData(static_cast<const char*>(data),
                               static_cast<const char*>(data) + len);
    AAsset_close(asset);

    return Init(fontData, path);
}

bool FontAtlas::LoadFromFile(const char* filePath, float size)
{
    Destroy();
    fontSize = size;

    if (FT_Init_FreeType(&ft)) {
        LOGE("failed to init FreeType");
        return false;
    }

    FILE* f = fopen(filePath, "rb");
    if (!f) {
        LOGE("failed to open font file: %s", filePath);
        return false;
    }

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);

    std::vector<char> fontData(len);
    fread(fontData.data(), 1, len, f);
    fclose(f);

    return Init(fontData, filePath);
}

void FontAtlas::Destroy()
{
    if (texture) { glDeleteTextures(1, &texture); texture = 0; }
    delete[] atlasData; atlasData = nullptr;
    if (hbFont) { hb_font_destroy(hbFont); hbFont = nullptr; }
    if (face) { FT_Done_Face(face); face = nullptr; }
    if (ft) { FT_Done_FreeType(ft); ft = nullptr; }
    glyphs.clear();
    fontData.clear();
    fontData.shrink_to_fit();
    cursorX = cursorY = rowH = 0;
}

void FontAtlas::SetSize(float size)
{
    if (!face || fontSize == size) return;
    fontSize = size;

    FT_Set_Pixel_Sizes(face, 0, (FT_UInt)fontSize);
    ascender = (float)face->size->metrics.ascender / 64.0f;
    descender = (float)face->size->metrics.descender / 64.0f;
    lineHeight = (float)face->size->metrics.height / 64.0f;

    if (hbFont) hb_font_destroy(hbFont);
    hbFont = hb_ft_font_create(face, nullptr);

    glyphs.clear();
    cursorX = cursorY = rowH = 0;

    // Wipe stale pixel data from the GL texture
    if (texture && atlasData) {
        memset(atlasData, 0, atlasW * atlasH);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, atlasW, atlasH,
                        GL_RED, GL_UNSIGNED_BYTE, atlasData);
    }
}

const GlyphInfo* FontAtlas::GetGlyph(uint32_t codepoint)
{
    auto it = glyphs.find(codepoint);
    if (it != glyphs.end()) return &it->second;
    return RenderGlyph(codepoint);
}

float FontAtlas::GetAdvance(uint32_t codepoint)
{
    auto it = glyphs.find(codepoint);
    if (it != glyphs.end()) return it->second.advanceX;

    // Get advance metrics from FreeType without rendering to the atlas
    if (FT_Load_Char(face, codepoint, FT_LOAD_NO_BITMAP)) {
        return 0.0f;
    }
    return (float)face->glyph->advance.x / 64.0f;
}

GlyphInfo* FontAtlas::RenderGlyph(uint32_t codepoint)
{
    if (FT_Load_Char(face, codepoint, FT_LOAD_RENDER)) {
        LOGE("failed to load glyph U+%04X", codepoint);
        return nullptr;
    }

    FT_Bitmap& bmp = face->glyph->bitmap;
    int gw = (int)bmp.width;
    int gh = (int)bmp.rows;

    if (cursorX + gw + 1 > atlasW) {
        cursorX = 0;
        cursorY += rowH + 1;
        rowH = 0;
    }
    if (cursorY + gh + 1 > atlasH) {
        LOGE("atlas full (%dx%d)", atlasW, atlasH);
        return nullptr;
    }

    for (int y = 0; y < gh; ++y)
        for (int x = 0; x < gw; ++x)
            atlasData[(cursorY + y) * atlasW + (cursorX + x)] =
                bmp.buffer[y * gw + x];

    glBindTexture(GL_TEXTURE_2D, texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(GL_TEXTURE_2D, 0, cursorX, cursorY, gw, gh,
                    GL_RED, GL_UNSIGNED_BYTE, bmp.buffer);

    GlyphInfo info;
    info.advanceX = (float)face->glyph->advance.x / 64.0f;
    info.bearingX = (float)face->glyph->bitmap_left;
    info.bearingY = (float)face->glyph->bitmap_top;
    info.width = (float)gw;
    info.height = (float)gh;
    info.u0 = (float)cursorX / atlasW;
    info.v0 = (float)cursorY / atlasH;
    info.u1 = (float)(cursorX + gw) / atlasW;
    info.v1 = (float)(cursorY + gh) / atlasH;

    cursorX += gw + 1;
    if (gh + 1 > rowH) rowH = gh + 1;

    glyphs[codepoint] = info;
    return &glyphs[codepoint];
}
