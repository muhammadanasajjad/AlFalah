#pragma once
#include <string>
#include <vector>
#include <android/asset_manager.h>

struct Word {
    int32_t id;
    int32_t wordNumber;
    std::string text;
};

struct Ayah {
    int32_t ayahNumber;
    std::vector<Word> words;
};

struct Surah {
    int32_t surahNumber;
    std::vector<Ayah> ayahs;
};

class QuranDatabase {
public:
    void Load(AAssetManager* mgr);

    int32_t SurahCount() const;
    const Surah& GetSurah(int32_t surahNumber) const;
    const Ayah* GetAyah(int32_t surahNumber, int32_t ayahNumber) const;

private:
    void LoadSurah(AAssetManager* mgr, int32_t surahNumber);
    static std::string ReadAsset(AAssetManager* mgr, const char* path);

    std::vector<Surah> mSurahs;
};
