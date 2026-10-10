#include "save.hpp"

#include "32blit.hpp"

#include <cstring>

namespace {

constexpr uint32_t SAVE_MAGIC = 0x43544554; // "TETC"
constexpr uint16_t SAVE_VERSION = 1;

SaveData data;

void set_defaults() {
    std::memset(&data, 0, sizeof(data));
    data.magic = SAVE_MAGIC;
    data.version = SAVE_VERSION;
    data.music = 1;
    data.sfx = 1;
    data.start_level = 1;
    data.ghost = 1;
    std::strcpy(data.last_name, "AAA");

    // A few scores to chase on a fresh install.
    const char *names[HIGH_SCORE_COUNT] = {"CHI", "LLI", "PEP", "HOT", "JAL"};
    const uint32_t scores[HIGH_SCORE_COUNT] = {10000, 7500, 5000, 2500, 1000};
    const uint16_t lines[HIGH_SCORE_COUNT] = {60, 40, 30, 15, 8};
    const uint8_t levels[HIGH_SCORE_COUNT] = {7, 5, 4, 2, 1};
    for (int i = 0; i < HIGH_SCORE_COUNT; ++i) {
        std::strcpy(data.scores[i].name, names[i]);
        data.scores[i].score = scores[i];
        data.scores[i].lines = lines[i];
        data.scores[i].level = levels[i];
    }
}

} // namespace

SaveData &save_data() { return data; }

void load_save() {
    if (!blit::read_save(data) || data.magic != SAVE_MAGIC || data.version != SAVE_VERSION) {
        set_defaults();
        return;
    }
    // Guard against corrupted values.
    if (data.start_level < 1 || data.start_level > 15)
        data.start_level = 1;
    data.last_name[3] = 0;
    for (auto &s : data.scores)
        s.name[3] = 0;
}

void store_save() { blit::write_save(data); }

int high_score_rank(uint32_t score) {
    if (score == 0)
        return -1;
    for (int i = 0; i < HIGH_SCORE_COUNT; ++i)
        if (score > data.scores[i].score)
            return i;
    return -1;
}

void insert_high_score(int rank, const HighScore &entry) {
    if (rank < 0 || rank >= HIGH_SCORE_COUNT)
        return;
    for (int i = HIGH_SCORE_COUNT - 1; i > rank; --i)
        data.scores[i] = data.scores[i - 1];
    data.scores[rank] = entry;
}
