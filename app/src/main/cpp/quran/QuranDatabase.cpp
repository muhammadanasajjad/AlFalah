#include "QuranDatabase.hpp"
#include "../third_party/picojson/picojson.h"
#include <android/log.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>

#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "QURAN_DB", __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  "QURAN_DB", __VA_ARGS__)

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

void QuranDatabase::BuildPageMap(AAssetManager* mgr)
{
    mWordToPage.clear();
    mWordToLine.clear();
    mSurahToPage.clear();
    mPageLayouts.clear();

    std::string csv = ReadAsset(mgr, "quran/surahsQPC/qpc-v2-15-lines.csv");
    if (csv.empty()) {
        LOGE("failed to read page CSV");
        return;
    }

    auto parseLine = [](const std::string& line) -> std::vector<std::string> {
        std::vector<std::string> fields;
        std::string field;
        bool inQuotes = false;
        for (char c : line) {
            if (c == '"') {
                inQuotes = !inQuotes;
            } else if (c == ',' && !inQuotes) {
                fields.push_back(field);
                field.clear();
            } else {
                field += c;
            }
        }
        fields.push_back(field);
        return fields;
    };

    size_t pos = 0;
    // Skip header line
    pos = csv.find('\n');
    if (pos == std::string::npos) return;
    ++pos;

    int32_t currentSurah = 0;

    while (pos < csv.size()) {
        size_t end = csv.find('\n', pos);
        std::string line = csv.substr(pos, end - pos);
        if (end == std::string::npos) break;
        pos = end + 1;

        if (line.empty()) continue;

        std::vector<std::string> fields = parseLine(line);
        if (fields.size() < 6) continue;

        // Columns: page_number, line_number, line_type, is_centered, first_word_id, last_word_id, surah_number
        int32_t pageNum = std::stoi(fields[0]);
        int32_t lineNum = std::stoi(fields[1]);
        const std::string& lineType = fields[2];
        bool isCentered = (!fields[3].empty() && fields[3] != "0");

        PageLine pl;
        pl.pageNumber = pageNum;
        pl.lineNumber = lineNum;
        pl.lineType = lineType;
        pl.isCentered = isCentered;

        if (lineType == "surah_name" && fields.size() >= 7) {
            int32_t surahNum = std::stoi(fields[6]);
            mSurahToPage[surahNum] = pageNum;
            currentSurah = surahNum;
            pl.surahNumber = surahNum;
            mPageLayouts[pageNum].lines.push_back(std::move(pl));
            continue;
        }

        if (lineType == "basmallah") {
            pl.surahNumber = currentSurah;
            if (fields.size() >= 6) {
                if (!fields[4].empty()) pl.firstWordId = std::stoi(fields[4]);
                if (!fields[5].empty()) pl.lastWordId = std::stoi(fields[5]);
            }
            mPageLayouts[pageNum].lines.push_back(std::move(pl));
            continue;
        }

        if (lineType != "ayah") continue;

        pl.surahNumber = currentSurah;

        const std::string& firstStr = fields[4];
        const std::string& lastStr = fields[5];
        if (firstStr.empty() || lastStr.empty()) {
            mPageLayouts[pageNum].lines.push_back(std::move(pl));
            continue;
        }

        int32_t firstId = std::stoi(firstStr);
        int32_t lastId = std::stoi(lastStr);
        pl.firstWordId = firstId;
        pl.lastWordId = lastId;

        for (int32_t wid = firstId; wid <= lastId; ++wid) {
            mWordToPage[wid] = pageNum;
            mWordToLine[wid] = lineNum;
        }

        mPageLayouts[pageNum].lines.push_back(std::move(pl));
    }

    LOGI("built page map: %zu word->page entries, %zu word->line entries, "
         "%zu surah->page entries, %zu page layouts",
         mWordToPage.size(), mWordToLine.size(),
         mSurahToPage.size(), mPageLayouts.size());
}

void QuranDatabase::Load(AAssetManager* mgr)
{
    mSurahs.clear();
    mSurahs.reserve(114);

    BuildPageMap(mgr);
    LoadSurahMetadata(mgr);

    for (int32_t i = 1; i <= 114; ++i) {
        LoadSurah(mgr, i);
    }
}

void QuranDatabase::LoadSurahMetadata(AAssetManager* mgr)
{
    mSurahInfo.clear();
    mSurahInfo.reserve(114);

    std::string csv = ReadAsset(mgr, "quran/quran-metadata-surah-name.csv");
    if (csv.empty()) {
        LOGE("failed to read surah metadata CSV");
        return;
    }

    size_t pos = 0;
    // Skip header line
    pos = csv.find('\n');
    if (pos == std::string::npos) return;
    ++pos;

    while (pos < csv.size() && mSurahInfo.size() < 114) {
        size_t end = csv.find('\n', pos);
        std::string line = csv.substr(pos, end - pos);
        if (end == std::string::npos) break;
        pos = end + 1;

        if (line.empty()) continue;

        SurahInfo info;
        size_t start = 0;
        int fieldIdx = 0;
        for (size_t i = 0; i <= line.size() && fieldIdx <= 6; ++i) {
            if (i == line.size() || line[i] == ',') {
                std::string val = line.substr(start, i - start);
                if (val.size() >= 2 && val.front() == '"' && val.back() == '"') {
                    val = val.substr(1, val.size() - 2);
                }
                switch (fieldIdx) {
                    case 1: info.nameSimple = val; break;
                    case 2: info.nameArabic = val; break;
                    case 4: info.revelationPlace = val; break;
                    case 5: info.verseCount = std::stoi(val); break;
                }
                start = i + 1;
                ++fieldIdx;
            }
        }

        mSurahInfo.push_back(std::move(info));
    }

    for (int32_t i = 0; i < 114; ++i) {
        int32_t surahNum = i + 1;
        auto it = mSurahToPage.find(surahNum);
        if (it != mSurahToPage.end()) {
            mSurahInfo[i].startPage = it->second;
        }
    }

    for (int32_t i = 0; i < 114; ++i) {
        int32_t surahNum = i + 1;
        auto it = mSurahToPage.find(surahNum + 1);
        if (it != mSurahToPage.end()) {
            mSurahInfo[i].endPage = it->second - 1;
        } else {
            mSurahInfo[i].endPage = 604;
        }
    }

    LOGI("loaded %zu surah metadata entries", mSurahInfo.size());
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
        auto pgIt = mWordToPage.find(wordId);
        if (pgIt != mWordToPage.end()) {
            w.pageNumber = pgIt->second;
        }
        auto lnIt = mWordToLine.find(wordId);
        if (lnIt != mWordToLine.end()) {
            w.lineNumber = lnIt->second;
        }

        mWordTextById[wordId] = text;
        mWordToAyah[wordId] = ayahNum;

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

const SurahInfo& QuranDatabase::GetSurahInfo(int32_t surahNumber) const
{
    return mSurahInfo[surahNumber - 1];
}

int32_t QuranDatabase::GetSurahStartPage(int32_t surahNumber) const
{
    if (surahNumber < 1 || surahNumber > static_cast<int32_t>(mSurahInfo.size())) {
        return 0;
    }
    return mSurahInfo[surahNumber - 1].startPage;
}

const PageLayout& QuranDatabase::GetPageLayout(int32_t pageNumber) const
{
    static const PageLayout kEmpty{};
    auto it = mPageLayouts.find(pageNumber);
    if (it != mPageLayouts.end()) {
        return it->second;
    }
    return kEmpty;
}

int32_t QuranDatabase::GetWordLine(int32_t wordId) const
{
    auto it = mWordToLine.find(wordId);
    if (it != mWordToLine.end()) {
        return it->second;
    }
    return 0;
}

const std::string* QuranDatabase::GetWordText(int32_t wordId) const
{
    auto it = mWordTextById.find(wordId);
    if (it != mWordTextById.end()) {
        return &it->second;
    }
    return nullptr;
}

bool QuranDatabase::HasPageLayout(int32_t pageNumber) const
{
    return mPageLayouts.find(pageNumber) != mPageLayouts.end();
}

int32_t QuranDatabase::GetWordAyah(int32_t wordId) const
{
    auto it = mWordToAyah.find(wordId);
    if (it != mWordToAyah.end()) {
        return it->second;
    }
    return 0;
}
