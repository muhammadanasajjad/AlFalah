#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <android/asset_manager.h>

class FontAtlas;
class ClayRenderer;

class FontManager {
public:
    static constexpr uint16_t kInvalidFontId = UINT16_MAX;

    void Init(AAssetManager* mgr);

    uint16_t LoadSystem(const char* name, float size, bool rtl = false);
    uint16_t LoadAsset(const char* key, float size, bool rtl = false);
    uint16_t GetOrCreateFontId(const char* key, float size, bool rtl = false);

    uint16_t GetFontId(const char* key, float size) const;
    FontAtlas* GetAtlas(uint16_t fontId);
    bool IsRtl(uint16_t fontId) const;
    bool EnsureFontLoaded(uint16_t fontId);

    void Register(uint16_t fontId, ClayRenderer& renderer) const;
    void RegisterAll(ClayRenderer& renderer) const;

    void Destroy();

private:
    struct FontEntry {
        std::string compositeKey;
        std::string path;
        FontAtlas* atlas = nullptr;
        bool isRtl = false;
        float size = 16.0f;
    };

    static std::string MakeCompositeKey(const char* key, float size);
    uint16_t AddEntry(const std::string& compositeKey, const std::string& path,
                      float size, bool rtl, FontAtlas* atlas);

    AAssetManager* mMgr = nullptr;
    std::vector<FontEntry> mFonts;
    std::unordered_map<std::string, uint16_t> mKeyToFontId;
};
