#include "QuranDatabase.hpp"
#include "../third_party/picojson/picojson.h"
#include <android/log.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>

#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "QURAN_DB", __VA_ARGS__)

static constexpr const char* kSurahDir = "quran/surahsQPC/";

std::string QuranDatabase::ReadAsset(AAssetManager* mgr, const char* path)
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

void QuranDatabase::Load(AAssetManager* mgr)
{
    mSurahs.clear();
    mSurahs.reserve(114);

    for (int32_t i = 1; i <= 114; ++i) {
        LoadSurah(mgr, i);
    }
}

void QuranDatabase::LoadSurah(AAssetManager* mgr, int32_t surahNumber)
{
    char path[64];
    snprintf(path, sizeof(path), "%ssurah_%03d.json", kSurahDir, surahNumber);

    std::string json = ReadAsset(mgr, path);
    if (json.empty()) {
        LOGE("empty or missing surah file: %s", path);
        Surah emptySurah;
        emptySurah.surahNumber = surahNumber;
        mSurahs.push_back(std::move(emptySurah));
        return;
    }

    picojson::value root;
    std::string err = picojson::parse(root, json);
    if (!err.empty()) {
        LOGE("parse error for %s: %s", path, err.c_str());
        Surah emptySurah;
        emptySurah.surahNumber = surahNumber;
        mSurahs.push_back(std::move(emptySurah));
        return;
    }

    if (!root.is<picojson::object>()) {
        LOGE("expected object in %s", path);
        Surah emptySurah;
        emptySurah.surahNumber = surahNumber;
        mSurahs.push_back(std::move(emptySurah));
        return;
    }

    const picojson::object& obj = root.get<picojson::object>();
    Surah surah;
    surah.surahNumber = surahNumber;

    std::unordered_map<int32_t, Ayah> ayahMap;

    for (const auto& [key, val] : obj) {
        if (!val.is<picojson::object>()) continue;

        const picojson::object& entry = val.get<picojson::object>();

        auto itId = entry.find("id");
        auto itAyah = entry.find("ayah");
        auto itWord = entry.find("word");
        auto itText = entry.find("text");
        if (itId == entry.end() || itAyah == entry.end() ||
            itWord == entry.end() || itText == entry.end()) {
            continue;
        }

        int32_t ayahNum = std::stoi(itAyah->second.get<std::string>());
        int32_t wordNum = std::stoi(itWord->second.get<std::string>());
        int32_t wordId = static_cast<int32_t>(itId->second.get<double>());
        const std::string& text = itText->second.get<std::string>();

        Word w;
        w.id = wordId;
        w.wordNumber = wordNum;
        w.text = text;

        ayahMap[ayahNum].ayahNumber = ayahNum;
        ayahMap[ayahNum].words.push_back(std::move(w));
    }

    surah.ayahs.reserve(ayahMap.size());
    for (auto& [num, ayah] : ayahMap) {
        std::sort(ayah.words.begin(), ayah.words.end(),
            [](const Word& a, const Word& b) {
                return a.wordNumber < b.wordNumber;
            });
        surah.ayahs.push_back(std::move(ayah));
    }
    std::sort(surah.ayahs.begin(), surah.ayahs.end(),
        [](const Ayah& a, const Ayah& b) {
            return a.ayahNumber < b.ayahNumber;
        });

    mSurahs.push_back(std::move(surah));
}

int32_t QuranDatabase::SurahCount() const
{
    return static_cast<int32_t>(mSurahs.size());
}

const Surah& QuranDatabase::GetSurah(int32_t surahNumber) const
{
    return mSurahs[surahNumber - 1];
}

const Ayah* QuranDatabase::GetAyah(int32_t surahNumber, int32_t ayahNumber) const
{
    if (surahNumber < 1 || surahNumber > static_cast<int32_t>(mSurahs.size())) {
        return nullptr;
    }
    const Surah& s = mSurahs[surahNumber - 1];
    for (const auto& a : s.ayahs) {
        if (a.ayahNumber == ayahNumber) {
            return &a;
        }
    }
    return nullptr;
}
