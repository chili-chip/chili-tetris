#pragma once

#include <cstdint>

// Music and sound effects, synthesised with the 32blit audio channels.
//
// Channel layout:
//   0 - music melody     1 - music bass     2 - music drums
//   3 - main effects     4 - light effects (moves, rotations, menu ticks)
namespace audio {

enum class Song : uint8_t {
    None,
    Title, // original theme written for chili-tetris
    Game,  // Korobeiniki (traditional, public domain), arranged for this game
};

enum class Sfx : uint8_t {
    MenuMove,
    MenuSelect,
    MenuBack,
    Move,
    Rotate,
    Lock,
    HardDrop,
    Hold,
    Line,
    Tetris,
    TSpin,
    LevelUp,
    Countdown,
    Go,
    Pause,
    GameOver,
    HighScore,
};

void init();
void update(uint32_t dt_ms);

void play_song(Song song);
void stop_song();
Song current_song();
// 1.0 = written tempo. The in-game music speeds up with the level.
void set_tempo(float multiplier);

void play(Sfx sfx);

void set_music_enabled(bool enabled);
void set_sfx_enabled(bool enabled);

} // namespace audio
