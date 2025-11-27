#include "game.hpp"

#include <cstring>
#include <cstdlib>
#include <ctime>

using namespace blit;

constexpr int GRID_X = 3;
constexpr int GRID_Y = -3;
constexpr int DOWN_BUTTON_MULTIPLIER = 10;

// Global game instance
static Tetris game;

bool up_released = true;

void init()
{
    set_screen_mode(ScreenMode::hires);
    // seed RNG
    srand(static_cast<unsigned int>(time(nullptr)));

    channels[0].waveforms = Waveform::SQUARE;
    channels[0].attack_ms = 2;
    channels[0].decay_ms = 200;
    channels[0].sustain = 2;
    channels[0].release_ms = 2;

    channels[0].volume = 1000; // High volume
}

void render(uint32_t time)
{
    game.render();
}

void update(uint32_t time)
{
    game.update(time);
}

// --- Tetris implementation -------------------------------------------------

Tetris::Tetris()
{
    game_over = false;
    score = 0;
    // initialize board to empty
    for (int y = 0; y < BOARD_HEIGHT; ++y)
        for (int x = 0; x < GRID_WIDTH; ++x)
            board[y][x] = TetrominoType::COUNT;

    // spawn piece starting at row 21 (index 21)
    current_tetrimino = Tetrimino::random_tetrimino(Point(GRID_WIDTH / 2, VISIBLE_OFFSET));
    next_tetrimino = Tetrimino::random_tetrimino(Point(GRID_WIDTH + 2, 2));

    last_time = 0;
    drop_acc = 0;

    melody_index = 0;
    melody_timer = 0;
    melody_playing = true;
}

bool Tetris::check_collision(const Tetrimino &t) const
{
    auto blks = t.blocks();
    for (const auto &p : blks)
    {
        int x = p.x;
        int y = p.y;
        if (x < 0 || x >= GRID_WIDTH || y < 0 || y >= BOARD_HEIGHT)
            return true;
        if (board[y][x] != TetrominoType::COUNT)
            return true;
    }
    return false;
}

void Tetris::lock_piece()
{
    auto blks = current_tetrimino.blocks();
    for (const auto &p : blks)
    {
        if (p.y >= 0 && p.y < BOARD_HEIGHT && p.x >= 0 && p.x < GRID_WIDTH)
            board[p.y][p.x] = current_tetrimino.get_type();
    }
    clear_lines();
    spawn_next();
}

void Tetris::clear_lines()
{
    int cleared = 0;
    for (int y = BOARD_HEIGHT - 1; y >= 0; --y)
    {
        bool full = true;
        for (int x = 0; x < GRID_WIDTH; ++x)
        {
            if (board[y][x] == TetrominoType::COUNT)
            {
                full = false;
                break;
            }
        }
        if (full)
        {
            // shift everything above down
            for (int yy = y; yy > 0; --yy)
                board[yy] = board[yy - 1];
            // empty top row
            for (int x = 0; x < GRID_WIDTH; ++x)
                board[0][x] = TetrominoType::COUNT;
            ++cleared;
            ++y; // re-check same row index after shift
        }
    }

    if (cleared > 0)
    {
        // simple scoring: 100 points per line
        score += cleared * 100;
    }
}

bool Tetris::check_game_over() const
{
    // Check if any blocks occupy the spawn area (row 21 and above)
    for (int x = 0; x < GRID_WIDTH; ++x)
    {
        if (board[VISIBLE_OFFSET][x] != TetrominoType::COUNT)
            return true;
    }
    return false;
}

void Tetris::spawn_next()
{
    if (check_game_over())
    {
        game_over = true;
        return;
    }
    current_tetrimino = next_tetrimino;
    Point spawn_pos(GRID_WIDTH / 2, VISIBLE_OFFSET);
    current_tetrimino.set_position(spawn_pos);
    current_tetrimino = Tetrimino(current_tetrimino.get_type(), spawn_pos, current_tetrimino.get_rotation());
    next_tetrimino = Tetrimino::random_tetrimino(Point(GRID_WIDTH + 2, 2));
}

void Tetris::update(uint32_t time)
{
    if (last_time == 0)
        last_time = time;
    uint32_t dt = time - last_time;
    last_time = time;

    // update simple music
    update_music(dt);

    if (game_over)
    {
        if (pressed(Button::MENU))
        {
            // restart game
            *this = Tetris();
        }
        return;
    }

    // input handling (simple repeat)
    uint32_t now = time;
    bool moved = false;
    if (now - last_input_time >= input_repeat_ms)
    {
        if (pressed(Button::DPAD_LEFT))
        {
            Tetrimino test = current_tetrimino;
            test.move_by(-1, 0);
            if (!check_collision(test))
                current_tetrimino = test;
            moved = true;
        }
        else if (pressed(Button::DPAD_RIGHT))
        {
            Tetrimino test = current_tetrimino;
            test.move_by(1, 0);
            if (!check_collision(test))
                current_tetrimino = test;
            moved = true;
        }
        else if (pressed(Button::DPAD_UP) && up_released)
        {
            Tetrimino test = current_tetrimino;
            test.rotate_cw();
            if (!check_collision(test))
                current_tetrimino = test;
            moved = true;
            up_released = false;
        }

        if (moved)
            last_input_time = now;
    }

    up_released = !pressed(Button::DPAD_UP);

    // gravity
    drop_acc += dt;
    if (pressed(Button::DPAD_DOWN))
    {
        // soft drop multiplier
        drop_acc += dt * DOWN_BUTTON_MULTIPLIER;
    }

    if (drop_acc >= drop_interval_ms)
    {
        drop_acc = 0;
        Tetrimino test = current_tetrimino;
        test.move_by(0, 1);
        if (!check_collision(test))
        {
            current_tetrimino = test;
        }
        else
        {
            // lock
            lock_piece();
        }
    }
}

void Tetris::render()
{
    // layout
    const int gx = GRID_X;
    const int gy = GRID_Y;
    const int width_px = GRID_WIDTH * cell_size;
    // visible region: lower half of row 20 (index 20) + rows 21..39
    const int full_rows = BOARD_HEIGHT - (VISIBLE_OFFSET); // rows after the half row
    const int height_px = full_rows * cell_size + cell_size / 2;

    // clear
    screen.pen = Pen(0, 0, 0); // background
    screen.clear();

    // draw background for playfield
    screen.pen = grid_line_color;
    screen.rectangle(Rect(gx - 3, gy - 3, width_px + 5, height_px + 5));

    // draw board cells
    for (int r = VISIBLE_OFFSET; r < BOARD_HEIGHT; ++r)
    {
        int screen_y = gy + (cell_size / 2) + (r - (VISIBLE_OFFSET)) * cell_size;
        for (int x = 0; x < GRID_WIDTH; ++x)
        {
            auto cell = board[r][x];
            screen.pen = (cell != TetrominoType::COUNT) ? get_tetromino_color(cell) : empty_cell_color;
            screen.rectangle(Rect(gx + x * cell_size, screen_y, cell_size - 1, cell_size - 1));
        }
    }

    // draw current piece
    screen.pen = current_tetrimino.color();
    for (auto &p : current_tetrimino.blocks())
    {
        int x = p.x, y = p.y;
        if (x < 0 || x >= GRID_WIDTH)
            continue;
        int screen_y = gy + (cell_size / 2) + (y - (VISIBLE_OFFSET + 1)) * cell_size;
        screen.rectangle(Rect(gx + x * cell_size, screen_y, cell_size - 1, cell_size - 1));
    }

    // draw frame top
    screen.pen = grid_line_color;
    screen.rectangle(Rect(0, 0, width_px + 5, 3));

    // draw next preview
    screen.pen = Pen(255, 255, 255);
    screen.text("Next:", minimal_font, Point(gx + width_px + 8, gy + 16));
    // draw preview blocks relative to an origin
    Point preview_origin(gx + width_px + 16, gy + 24);
    auto rel = get_tetromino(next_tetrimino.get_type(), 0);
    screen.pen = next_tetrimino.color();
    for (auto &pp : rel)
    {
        int px = preview_origin.x + (pp.x + 1) * cell_size;
        int py = preview_origin.y + (pp.y + 1) * cell_size;
        screen.rectangle(Rect(px, py, cell_size - 1, cell_size - 1));
    }

    // draw score & level
    screen.pen = ui_text_color;
    screen.text(std::string("Score: ") + std::to_string(score), minimal_font, Point(gx + width_px + 8, 3));

    if (game_over)
    {
        screen.pen = Pen(255, 0, 0);
        screen.text("GAME OVER", minimal_font, Point(gx + width_px + 8, gy + height_px - 8));
    }
}

void Tetris::update_music(uint32_t dt)
{
    if (!melody_playing)
        return;

    // Decrease the timer by the delta time
    if (melody_timer > dt)
    {
        melody_timer -= dt;
    }
    else
    {
        melody_timer = 0;
    }

    // If timer is done, play the next note
    if (melody_timer == 0)
    {
        // 1. Stop previous note
        channels[0].trigger_release();

        // 2. Get current note data
        const auto &n = melody[melody_index];

        // 3. Play note ONLY if it is not a REST
        if (n.freq > 0)
        {
            channels[0].frequency = n.freq;
            channels[0].trigger_attack();
        }

        // 4. Set duration for this note
        melody_timer = n.dur_ms;

        // 5. Advance index, loop back to 0 if at end
        melody_index++;
        if (melody_index >= sizeof(melody) / sizeof(melody[0]))
        {
            melody_index = 0;
        }
    }
}