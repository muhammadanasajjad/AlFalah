#include "engine/FontManager.hpp"
#include "engine/renderer/FontAtlas.hpp"
#include "engine/renderer/ClayRenderer.hpp"
#include <android/log.h>

#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "FONT_MGR", __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  "FONT_MGR", __VA_ARGS__)

void FontManager::Init(AAssetManager* mgr)
{
    Destroy();
    mMgr = mgr;
}

std::string FontManager::MakeCompositeKey(const char* key, float size)
{
    return std::string(key) + "@" + std::to_string(size);
}

uint16_t FontManager::LoadSystem(const char* name, float size, bool rtl)
{
    std::string ck = MakeCompositeKey(name, size);
    {
        auto it = mKeyToFontId.find(ck);
        if (it != mKeyToFontId.end()) return it->second;
    }

    static const char* kFontPaths[] = {
        "/system/fonts/NotoSans-Regular.ttf",
        "/system/fonts/Roboto-Regular.ttf",
        "/system/fonts/RobotoCondensed-Regular.ttf",
        "/system/fonts/Roboto-Light.ttf",
        "/system/fonts/DroidSans.ttf",
    };

    for (const char* fp : kFontPaths) {
        FontAtlas* atlas = new FontAtlas();
        if (atlas->LoadFromFile(fp, size)) {
            LOGI("loaded system font '%s' from %s", name, fp);
            return AddEntry(ck, name, size, rtl, atlas);
        }
        delete atlas;
    }

    // Fallback: try bundled font in APK assets
    std::string assetPath = "fonts/" + std::string(name) + "-Regular.ttf";
    return LoadAsset(assetPath.c_str(), size, rtl);
}

uint16_t FontManager::LoadAsset(const char* key, float size, bool rtl)
{
    std::string ck = MakeCompositeKey(key, size);
    {
        auto it = mKeyToFontId.find(ck);
        if (it != mKeyToFontId.end()) return it->second;
    }

    if (!mMgr) {
        LOGE("LoadAsset: no asset manager (key=%s)", key);
        return kInvalidFontId;
    }

    FontAtlas* atlas = new FontAtlas();
    if (!atlas->Load(mMgr, key, size)) {
        delete atlas;
        LOGE("LoadAsset failed: %s", key);
        return kInvalidFontId;
    }

    LOGI("loaded asset font '%s' rtl=%d", key, (int)rtl);
    return AddEntry(ck, key, size, rtl, atlas);
}

uint16_t FontManager::GetOrCreateFontId(const char* key, float size, bool rtl)
{
    std::string ck = MakeCompositeKey(key, size);
    {
        auto it = mKeyToFontId.find(ck);
        if (it != mKeyToFontId.end()) return it->second;
    }

    return AddEntry(ck, key, size, rtl, nullptr);
}

uint16_t FontManager::GetFontId(const char* key, float size) const
{
    std::string ck = MakeCompositeKey(key, size);
    auto it = mKeyToFontId.find(ck);
    if (it != mKeyToFontId.end()) return it->second;
    return kInvalidFontId;
}

FontAtlas* FontManager::GetAtlas(uint16_t fontId)
{
    if (fontId >= mFonts.size()) return nullptr;
    return mFonts[fontId].atlas;
}

bool FontManager::IsRtl(uint16_t fontId) const
{
    if (fontId >= mFonts.size()) return false;
    return mFonts[fontId].isRtl;
}

bool FontManager::EnsureFontLoaded(uint16_t fontId)
{
    if (fontId >= mFonts.size()) return false;
    FontEntry& entry = mFonts[fontId];
    if (entry.atlas) return true;

    if (!mMgr) {
        LOGE("EnsureFontLoaded: no asset manager (fontId=%u)", fontId);
        return false;
    }

    FontAtlas* atlas = new FontAtlas();
    if (!atlas->Load(mMgr, entry.path.c_str(), entry.size)) {
        delete atlas;
        LOGE("EnsureFontLoaded failed: %s", entry.path.c_str());
        return false;
    }

    entry.atlas = atlas;
    LOGI("lazy loaded font '%s' (fontId=%u)", entry.path.c_str(), fontId);
    return true;
}

void FontManager::Register(uint16_t fontId, ClayRenderer& renderer) const
{
    if (fontId >= mFonts.size()) return;
    const FontEntry& entry = mFonts[fontId];
    renderer.SetFontAtlas(fontId, entry.atlas, entry.isRtl);
}

void FontManager::RegisterAll(ClayRenderer& renderer) const
{
    for (uint16_t i = 0; i < (uint16_t)mFonts.size(); ++i) {
        Register(i, renderer);
    }
}

void FontManager::Destroy()
{
    for (auto& entry : mFonts) {
        delete entry.atlas;
        entry.atlas = nullptr;
    }
    mFonts.clear();
    mKeyToFontId.clear();
}

uint16_t FontManager::AddEntry(const std::string& compositeKey, const std::string& path,
                                float size, bool rtl, FontAtlas* atlas)
{
    uint16_t id = (uint16_t)mFonts.size();
    mFonts.push_back({compositeKey, path, atlas, rtl, size});
    mKeyToFontId[compositeKey] = id;
    return id;
}
