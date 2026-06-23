#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <android/asset_manager.h>

struct Word {
    int32_t id;
    int32_t wordNumber;
    std::string text;
    int32_t pageNumber = 0;
    int32_t lineNumber = 0;
};

struct Ayah {
    int32_t ayahNumber;
    std::vector<Word> words;
};

struct Surah {
    int32_t surahNumber;
    std::vector<Ayah> ayahs;
};

struct SurahInfo {
    std::string nameSimple;
    std::string nameArabic;
    int32_t startPage = 0;
    int32_t endPage = 0;
    int32_t verseCount = 0;
    std::string revelationPlace;
};

struct PageLine {
    int32_t pageNumber;
    int32_t lineNumber;
    std::string lineType;    // "ayah", "surah_name", "basmallah"
    bool isCentered;
    int32_t firstWordId = 0;
    int32_t lastWordId = 0;
    int32_t surahNumber = 0;
};

struct PageLayout {
    int32_t pageNumber;
    std::vector<PageLine> lines;
};

class QuranDatabase {
public:
    void Load(AAssetManager* mgr);

    int32_t SurahCount() const;
    const Surah& GetSurah(int32_t surahNumber) const;
    const Ayah* GetAyah(int32_t surahNumber, int32_t ayahNumber) const;

    const SurahInfo& GetSurahInfo(int32_t surahNumber) const;
    int32_t GetSurahStartPage(int32_t surahNumber) const;

    // Mushaf format accessors
    const PageLayout& GetPageLayout(int32_t pageNumber) const;
    int32_t GetWordLine(int32_t wordId) const;
    const std::string* GetWordText(int32_t wordId) const;
    int32_t GetWordAyah(int32_t wordId) const;
    bool HasPageLayout(int32_t pageNumber) const;

private:
    void BuildPageMap(AAssetManager* mgr);
    void LoadSurah(AAssetManager* mgr, int32_t surahNumber);
    void LoadSurahMetadata(AAssetManager* mgr);
    static std::string ReadAsset(AAssetManager* mgr, const char* path);

    std::vector<Surah> mSurahs;
    std::vector<SurahInfo> mSurahInfo;
    std::unordered_map<int32_t, int32_t> mWordToPage;
    std::unordered_map<int32_t, int32_t> mWordToLine;
    std::unordered_map<int32_t, int32_t> mSurahToPage;
    std::unordered_map<int32_t, PageLayout> mPageLayouts;
    std::unordered_map<int32_t, std::string> mWordTextById;
    std::unordered_map<int32_t, int32_t> mWordToAyah;
};
