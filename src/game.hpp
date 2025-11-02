#pragma once

#include "32blit.hpp"
#include <array>
#include <cstdint>
#include <ctime>

// Tetrimino public API is provided in `src/tetriminos.hpp`.
#include "tetriminos.hpp"

using namespace blit;

class Tetris {
public:
    Tetris();

    // Update game state; `time` is the 32blit frame time in ms passed to update().
    void update(uint32_t time);
    void render();

private:
    // drawing/grid
    int cell_size = 6; // Size of each cell in pixels 6x6
    int grid_width = 10; // Width of the Tetris grid in cells
    int grid_height = 20; // Height of the Tetris grid in cells
    Pen empty_cell_color = Pen(0, 0, 0); // Color for empty cells (black)
    Pen grid_line_color = Pen(50, 50, 50); // Color for grid lines (dark gray)
    Pen ui_text_color = Pen(255, 255, 255); // Color for UI text (white)

    // game state
    bool game_over = false;
    bool game_started = false;
    Tetrimino current_tetrimino{Tetrimino::random_tetrimino(Point(4, 0))};
    Tetrimino next_tetrimino{Tetrimino::random_tetrimino(Point(12, 2))};
    std::array<std::array<TetrominoType, 10>, 20> board; // 10x20 Tetris board; TetrominoType::COUNT == empty

    // scoring and progression
    int score = 0;
    int lines_cleared = 0;

    // timing
    uint32_t last_time = 0;
    uint32_t drop_acc = 0; // milliseconds accumulator for gravity
    uint32_t drop_interval_ms = 800; // base gravity

    // simple input repeat handling
    uint32_t last_input_time = 0;
    uint32_t input_repeat_ms = 120;

    // helpers
    bool check_collision(const Tetrimino &t) const;
    void lock_piece();
    void clear_lines();
    void spawn_next();
    
    // drawing helpers
    void draw_cell(int x, int y, Pen pen, int gx, int gy) const;
    void draw_board(int gx, int gy) const;
    void draw_current_piece(int gx, int gy) const;
    void draw_next(int gx, int gy) const;
    void draw_ui(int gx, int gy) const;
};