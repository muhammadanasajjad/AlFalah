import json
from collections import defaultdict
from pathlib import Path

filename = "qpc-v2.json"

with open(filename, "r", encoding="utf-8") as file:
    data = json.load(file)

surahs = defaultdict(dict)

for key, value in data.items():
    surah, ayah, word = key.split(":")
    surahs[int(surah)][key] = value

output_dir = Path("surahsQPC")
output_dir.mkdir(exist_ok=True)

for surah_num, surah_data in surahs.items():
    output_file = output_dir / f"surah_{surah_num:03d}.json"

    with open(output_file, "w", encoding="utf-8") as f:
        json.dump(
            surah_data,
            f,
            ensure_ascii=False,
            indent=2
        )

print(f"Saved {len(surahs)} surah files.")
