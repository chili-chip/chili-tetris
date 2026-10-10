#pragma once

#include <array>
#include <cstdint>

struct HighScore {
    char name[4];   // three letters + terminator
    uint32_t score;
    uint16_t lines;
    uint8_t level;
    uint8_t pad;
};

constexpr int HIGH_SCORE_COUNT = 5;

struct SaveData {
    uint32_t magic;
    uint16_t version;
    uint8_t music;       // bool
    uint8_t sfx;         // bool
    uint8_t start_level; // 1..15
    uint8_t ghost;       // bool
    char last_name[4];
    std::array<HighScore, HIGH_SCORE_COUNT> scores;
};

// Loads the save (or defaults when there is none / it is from another version).
SaveData &save_data();
void load_save();
void store_save();

// Position (0-based) a score would take in the table, or -1 if it doesn't qualify.
int high_score_rank(uint32_t score);
// Inserts the score at `rank`, shifting lower entries down.
void insert_high_score(int rank, const HighScore &entry);
