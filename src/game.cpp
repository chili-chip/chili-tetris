#include "game.hpp"

#include <cstring>
#include <cstdlib>
#include <ctime>

using namespace blit;

constexpr int GRID_X = 3;
constexpr int GRID_Y = 3;
constexpr int DOWN_BUTTON_MULTIPLIER = 10;

// Global game instance
static Tetris game;

void init() {
    set_screen_mode(ScreenMode::hires);
    // seed RNG
    srand(static_cast<unsigned int>(time(nullptr)));
}

void render(uint32_t time) {
    game.render();
}

void update(uint32_t time) {
    game.update(time);
}

// --- Tetris implementation -------------------------------------------------

Tetris::Tetris() {
    // initialize board to empty
    for (int y = 0; y < grid_height; ++y)
        for (int x = 0; x < grid_width; ++x)
            board[y][x] = TetrominoType::COUNT;

    // initialize pieces
    current_tetrimino = Tetrimino::random_tetrimino(Point(grid_width / 2, 0));
    next_tetrimino = Tetrimino::random_tetrimino(Point(grid_width + 2, 2));

    last_time = 0;
    drop_acc = 0;
}

bool Tetris::check_collision(const Tetrimino &t) const {
    auto blks = t.blocks();
    for (const auto &p : blks) {
        int x = p.x;
        int y = p.y;
        if (x < 0 || x >= grid_width || y < 0 || y >= grid_height)
            return true;
        if (board[y][x] != TetrominoType::COUNT)
            return true;
    }
    return false;
}

void Tetris::lock_piece() {
    auto blks = current_tetrimino.blocks();
    for (const auto &p : blks) {
        if (p.y >= 0 && p.y < grid_height && p.x >= 0 && p.x < grid_width)
            board[p.y][p.x] = current_tetrimino.get_type();
    }
    clear_lines();
    spawn_next();
}

void Tetris::clear_lines() {
    int cleared = 0;
    for (int y = grid_height - 1; y >= 0; --y) {
        bool full = true;
        for (int x = 0; x < grid_width; ++x) {
            if (board[y][x] == TetrominoType::COUNT) { full = false; break; }
        }
        if (full) {
            // shift everything above down
            for (int yy = y; yy > 0; --yy)
                board[yy] = board[yy - 1];
            // empty top row
            for (int x = 0; x < grid_width; ++x)
                board[0][x] = TetrominoType::COUNT;
            ++cleared;
            ++y; // re-check same row index after shift
        }
    }

    if (cleared > 0) {
        lines_cleared += cleared;
        // simple scoring: 100 points per line
        score += cleared * 100;
    }
}

void Tetris::spawn_next() {
    current_tetrimino = next_tetrimino;
    // center the piece at the top
    Point spawn_pos(grid_width / 2, 0);
    current_tetrimino.set_position(spawn_pos);
    current_tetrimino = Tetrimino(current_tetrimino.get_type(), spawn_pos, current_tetrimino.get_rotation());
    next_tetrimino = Tetrimino::random_tetrimino(Point(grid_width + 2, 2));

    if (check_collision(current_tetrimino)) {
        game_over = true;
    }
}

void Tetris::update(uint32_t time) {
    if (last_time == 0) last_time = time;
    uint32_t dt = time - last_time;
    last_time = time;

    if (game_over) {
        if (pressed(Button::MENU)) {
            // restart game
        }
        return;
    }

    // input handling (simple repeat)
    uint32_t now = time;
    bool moved = false;
    if (now - last_input_time >= input_repeat_ms) {
        if (pressed(Button::DPAD_LEFT)) {
            Tetrimino test = current_tetrimino;
            test.move_by(-1, 0);
            if (!check_collision(test)) current_tetrimino = test;
            moved = true;
        } else if (pressed(Button::DPAD_RIGHT)) {
            Tetrimino test = current_tetrimino;
            test.move_by(1, 0);
            if (!check_collision(test)) current_tetrimino = test;
            moved = true;
        } else if (pressed(Button::A)) {
            Tetrimino test = current_tetrimino;
            test.rotate_cw();
            if (!check_collision(test)) current_tetrimino = test;
            moved = true;
        } else if (pressed(Button::B)) {
            Tetrimino test = current_tetrimino;
            test.rotate_ccw();
            if (!check_collision(test)) current_tetrimino = test;
            moved = true;
        }

        if (moved) last_input_time = now;
    }

    // gravity
    drop_acc += dt;
    if (pressed(Button::DPAD_DOWN)) {
        // soft drop multiplier
        drop_acc += dt * DOWN_BUTTON_MULTIPLIER;
    }

    if (drop_acc >= drop_interval_ms) {
        drop_acc = 0;
        Tetrimino test = current_tetrimino;
        test.move_by(0, 1);
        if (!check_collision(test)) {
            current_tetrimino = test;
        } else {
            // lock
            lock_piece();
        }
    }
}

void Tetris::render() {
    // layout
    const int gx = GRID_X;
    const int gy = GRID_Y;
    const int width_px = grid_width * cell_size;
    const int height_px = grid_height * cell_size;

    // clear
    screen.pen = Pen(0, 0, 0); // background
    screen.clear();

    // draw background for playfield
    screen.pen = grid_line_color;
    screen.rectangle(Rect(gx-3, gy-3, width_px+5, height_px+5));

    // draw cells
    for (int y = 0; y < grid_height; ++y) {
        for (int x = 0; x < grid_width; ++x) {
            auto cell = board[y][x];
            if (cell != TetrominoType::COUNT) {
                screen.pen = get_tetromino_color(cell);
            } else {
                screen.pen = empty_cell_color;
            }
            screen.rectangle(Rect(gx + x * cell_size, gy + y * cell_size, cell_size - 1, cell_size - 1));
        }
    }

    // draw current piece
    screen.pen = current_tetrimino.color();
    for (auto &p : current_tetrimino.blocks()) {
        int x = p.x, y = p.y;
        if (x >= 0 && x < grid_width && y >= 0 && y < grid_height)
            screen.rectangle(Rect(gx + x * cell_size, gy + y * cell_size, cell_size - 1, cell_size - 1));
    }

    // draw next preview
    screen.pen = Pen(255, 255, 255);
    screen.text("Next:", minimal_font, Point(gx + width_px + 8, gy + 16));
    // draw preview blocks relative to an origin
    Point preview_origin(gx + width_px + 16, gy + 24);
    auto rel = get_tetromino(next_tetrimino.get_type(), 0);
    screen.pen = next_tetrimino.color();
    for (auto &pp : rel) {
        int px = preview_origin.x + (pp.x + 1) * cell_size;
        int py = preview_origin.y + (pp.y + 1) * cell_size;
        screen.rectangle(Rect(px, py, cell_size - 1, cell_size - 1));
    }

    // draw score & level
    screen.pen = ui_text_color;
    screen.text(std::string("Score: ") + std::to_string(score), minimal_font, Point(gx + width_px + 8, gy));

    if (game_over) {
        screen.pen = Pen(255, 0, 0);
        screen.text("GAME OVER", minimal_font, Point(gx + width_px + 8, gy + height_px - 8));
    }
}