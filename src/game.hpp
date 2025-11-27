#pragma once

#include "32blit.hpp"
#include <array>
#include <cstdint>
#include <ctime>

// Tetrimino public API is provided in `src/tetriminos.hpp`.
#include "tetriminos.hpp"

using namespace blit;

// --- Definitions for readability ---
#define NOTE_B4 494
#define NOTE_C5 523
#define NOTE_D5 587
#define NOTE_E5 659
#define NOTE_F5 698
#define NOTE_G5 784
#define NOTE_A5 880
#define NOTE_A4 440
#define REST 0

// Tempo: 150 BPM (approximate)
// E = Eighth note, Q = Quarter note, DQ = Dotted Quarter
#define E 200
#define Q 400
#define DQ 600

class Tetris
{
public:
    Tetris();

    // Update game state; `time` is the 32blit frame time in ms passed to update().
    void update(uint32_t time);
    void render();

private:
    // grid constants
    static constexpr int GRID_WIDTH = 10;   // logical width
    static constexpr int BOARD_HEIGHT = 40; // logical height (full internal board)
    static constexpr int VISIBLE_ROWS = 20; // fully visible rows
    // We also show half of row index VISIBLE_ROWS (the 21st row 1-based)
    static constexpr int VISIBLE_OFFSET = 19; // first fully visible row index (we show lower half of this row)

    int cell_size = 6;                      // Size of each cell in pixels 6x6c
    Pen empty_cell_color = Pen(0, 0, 0);    // Color for empty cells (black)
    Pen grid_line_color = Pen(50, 50, 50);  // Color for grid lines (dark gray)
    Pen ui_text_color = Pen(255, 255, 255); // Color for UI text (white)

    // game state
    bool game_over = false;
    Tetrimino current_tetrimino{Tetrimino::random_tetrimino(Point(GRID_WIDTH / 2, 0))};
    Tetrimino next_tetrimino{Tetrimino::random_tetrimino(Point(GRID_WIDTH + 2, 2))};
    std::array<std::array<TetrominoType, GRID_WIDTH>, BOARD_HEIGHT> board; // TetrominoType::COUNT == empty

    // scoring and progression
    int score = 0;

    // timing
    uint32_t last_time = 0;
    uint32_t drop_acc = 0;           // milliseconds accumulator for gravity
    uint32_t drop_interval_ms = 800; // base gravity

    // simple input repeat handling
    uint32_t last_input_time = 0;
    uint32_t input_repeat_ms = 120;

    // helpers
    bool check_collision(const Tetrimino &t) const;
    bool check_game_over() const;
    void lock_piece();
    void clear_lines();
    void spawn_next();

    // drawing helpers
    void draw_cell(int x, int y, Pen pen, int gx, int gy) const;      // full cell
    void draw_half_cell(int x, int y, Pen pen, int gx, int gy) const; // half (top) cell for partial row
    void draw_board(int gx, int gy) const;
    void draw_current_piece(int gx, int gy) const;
    void draw_next(int gx, int gy) const;
    void draw_ui(int gx, int gy) const;

    // simple tune player

    struct Note
    {
        uint16_t freq;
        uint16_t dur_ms;
    };

    static constexpr Note melody[] = {
        // --- PART A ---
        {NOTE_E5, Q},
        {NOTE_B4, E},
        {NOTE_C5, E},
        {NOTE_D5, Q},
        {NOTE_C5, E},
        {NOTE_B4, E},
        {NOTE_A4, Q},
        {NOTE_A4, E},
        {NOTE_C5, E},
        {NOTE_E5, Q},
        {NOTE_D5, E},
        {NOTE_C5, E},
        {NOTE_B4, DQ},
        {NOTE_C5, E},
        {NOTE_D5, Q},
        {NOTE_E5, Q},
        {NOTE_C5, Q},
        {NOTE_A4, Q},
        {NOTE_A4, Q},
        {REST, E}, // Brief pause

        // --- PART B ---
        {NOTE_D5, DQ},
        {NOTE_F5, E},
        {NOTE_A5, Q},
        {NOTE_G5, E},
        {NOTE_F5, E},
        {NOTE_E5, DQ},
        {NOTE_C5, E},
        {NOTE_E5, Q},
        {NOTE_D5, E},
        {NOTE_C5, E},
        {NOTE_B4, Q},
        {NOTE_B4, E},
        {NOTE_C5, E},
        {NOTE_D5, Q},
        {NOTE_E5, Q},
        {NOTE_C5, Q},
        {NOTE_A4, Q},
        {NOTE_A4, Q},
        {REST, Q} // End of loop pause
    };

    size_t melody_index = 0;
    uint32_t melody_timer = 0;
    bool melody_playing = true;
    void update_music(uint32_t dt);
};