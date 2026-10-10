#pragma once

#include "tetriminos.hpp"

#include <array>
#include <cstdint>

// Things that happened during one update, for sounds and effects.
enum TetrisEvent : uint32_t {
    EV_MOVE = 1 << 0,
    EV_ROTATE = 1 << 1,
    EV_LOCK = 1 << 2,
    EV_HARD_DROP = 1 << 3,
    EV_HOLD = 1 << 4,
    EV_LINES = 1 << 5,
    EV_TSPIN = 1 << 6,
    EV_LEVEL_UP = 1 << 7,
    EV_GAME_OVER = 1 << 8,
};

// What the last lock scored, for the on-screen callout.
struct ClearInfo {
    int lines = 0;
    bool tspin = false;
    bool back_to_back = false;
    int combo = 0;
    uint32_t points = 0;
};

// Game rules only: board, active piece, scoring. No drawing or audio.
class Tetris {
public:
    static constexpr int WIDTH = 10;
    static constexpr int HEIGHT = 40;  // full internal board
    static constexpr int VISIBLE = 20; // rows HIDDEN..HEIGHT-1 are on screen
    static constexpr int HIDDEN = HEIGHT - VISIBLE;
    static constexpr int QUEUE = 5;
    static constexpr uint32_t CLEAR_MS = 360;

    enum class Phase { Falling, Clearing, Over };

    struct Input {
        bool left, right, down;          // held
        bool left_p, right_p;            // pressed this tick
        bool hard_drop, rotate_cw, rotate_ccw, hold; // pressed this tick
    };

    void start(int start_level);
    void update(uint32_t dt, const Input &in);

    // Board access; TetrominoType::COUNT means empty.
    TetrominoType cell(int x, int y) const { return board_[y][x]; }

    Phase phase() const { return phase_; }
    const Tetrimino &piece() const { return piece_; }
    Tetrimino ghost() const;
    bool has_hold() const { return has_hold_; }
    TetrominoType hold_piece() const { return hold_; }
    bool can_hold() const { return can_hold_; }
    TetrominoType next(int i) const { return queue_[i]; }

    uint32_t score() const { return score_; }
    int level() const { return level_; }
    int lines() const { return lines_; }

    // Rows being cleared (valid while phase() == Clearing) and progress 0..1.
    int clearing_count() const { return clearing_count_; }
    int clearing_row(int i) const { return clearing_rows_[i]; }
    float clearing_progress() const { return 1.0f - static_cast<float>(clear_timer_) / CLEAR_MS; }

    // Lock-delay progress 0..1 while resting on the stack (for a flash cue).
    float lock_progress() const;

    uint32_t take_events() {
        uint32_t e = events_;
        events_ = 0;
        return e;
    }
    const ClearInfo &last_clear() const { return last_clear_; }

    // Hard-drop trail: columns and the rows the piece fell through.
    struct Trail {
        std::array<Point, 4> to;
        int distance = 0;
    };
    const Trail &last_trail() const { return trail_; }

    // Cells of the most recently locked piece (for the lock flash).
    const std::array<Point, 4> &last_locked() const { return last_locked_; }

private:
    std::array<std::array<TetrominoType, WIDTH>, HEIGHT> board_{};
    Tetrimino piece_;
    std::array<TetrominoType, QUEUE> queue_{};
    PieceBag bag_;
    TetrominoType hold_ = TetrominoType::COUNT;
    bool has_hold_ = false;
    bool can_hold_ = true;

    Phase phase_ = Phase::Over;
    uint32_t score_ = 0;
    int level_ = 1;
    int start_level_ = 1;
    int lines_ = 0;
    int combo_ = -1;
    bool b2b_ = false;

    // timing
    float gravity_acc_ = 0;
    int das_dir_ = 0;
    int das_timer_ = 0;
    uint32_t lock_timer_ = 0;
    int lock_resets_ = 0;
    int lowest_y_ = 0;
    bool last_was_rotation_ = false;
    int last_kick_ = 0;

    int clearing_rows_[4] = {};
    int clearing_count_ = 0;
    int32_t clear_timer_ = 0;

    uint32_t events_ = 0;
    ClearInfo last_clear_;
    Trail trail_;
    std::array<Point, 4> last_locked_{};

    bool collides(const Tetrimino &t) const;
    bool on_ground() const;
    bool try_move(int dx, int dy);
    bool try_rotate(int dir);
    void moved_on_ground();
    void spawn(TetrominoType type);
    void spawn_next();
    void do_hold();
    void lock();
    bool is_tspin() const;
    void finish_clear();
    float gravity_ms() const;
};
