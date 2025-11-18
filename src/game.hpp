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
    // grid constants
    static constexpr int GRID_WIDTH = 10;      // logical width
    static constexpr int BOARD_HEIGHT = 40;    // logical height (full internal board)
    static constexpr int VISIBLE_ROWS = 20;    // fully visible rows
    // We also show half of row index VISIBLE_ROWS (the 21st row 1-based)
    static constexpr int VISIBLE_OFFSET = 19;   // first fully visible row index (we show lower half of this row)

    int cell_size = 6; // Size of each cell in pixels 6x6c
    Pen empty_cell_color = Pen(0, 0, 0); // Color for empty cells (black)
    Pen grid_line_color = Pen(50, 50, 50); // Color for grid lines (dark gray)
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
    uint32_t drop_acc = 0; // milliseconds accumulator for gravity
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
    void draw_cell(int x, int y, Pen pen, int gx, int gy) const;             // full cell
    void draw_half_cell(int x, int y, Pen pen, int gx, int gy) const;        // half (top) cell for partial row
    void draw_board(int gx, int gy) const;
    void draw_current_piece(int gx, int gy) const;
    void draw_next(int gx, int gy) const;
    void draw_ui(int gx, int gy) const;
};