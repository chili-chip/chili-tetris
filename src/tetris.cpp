#include "tetris.hpp"

#include <cmath>

namespace {

constexpr int DAS_MS = 160;
constexpr int ARR_MS = 45;
constexpr uint32_t LOCK_DELAY_MS = 500;
constexpr int MAX_LOCK_RESETS = 15;
constexpr int MAX_LEVEL = 20;

constexpr TetrominoType EMPTY = TetrominoType::COUNT;

} // namespace

void Tetris::start(int start_level) {
    for (auto &row : board_)
        row.fill(EMPTY);

    bag_.reset();
    for (auto &q : queue_)
        q = bag_.next();

    hold_ = EMPTY;
    has_hold_ = false;
    score_ = 0;
    start_level_ = start_level < 1 ? 1 : start_level;
    level_ = start_level_;
    lines_ = 0;
    combo_ = -1;
    b2b_ = false;
    das_dir_ = 0;
    events_ = 0;
    last_clear_ = ClearInfo();
    trail_ = Trail();
    clearing_count_ = 0;
    phase_ = Phase::Falling;
    spawn_next();
}

float Tetris::gravity_ms() const {
    // Guideline gravity curve: seconds per row = (0.8 - (level - 1) * 0.007) ^ (level - 1)
    const float l = static_cast<float>(level_ - 1);
    return powf(0.8f - l * 0.007f, l) * 1000.0f;
}

bool Tetris::collides(const Tetrimino &t) const {
    for (const auto &p : t.blocks()) {
        if (p.x < 0 || p.x >= WIDTH || p.y < 0 || p.y >= HEIGHT)
            return true;
        if (board_[p.y][p.x] != EMPTY)
            return true;
    }
    return false;
}

bool Tetris::on_ground() const {
    Tetrimino t = piece_;
    t.move_by(0, 1);
    return collides(t);
}

void Tetris::moved_on_ground() {
    // Moving or rotating while resting restarts the lock delay, a limited number of times.
    if (on_ground() && lock_resets_ < MAX_LOCK_RESETS) {
        lock_timer_ = 0;
        ++lock_resets_;
    }
}

bool Tetris::try_move(int dx, int dy) {
    Tetrimino t = piece_;
    t.move_by(dx, dy);
    if (collides(t))
        return false;
    piece_ = t;
    last_was_rotation_ = false;
    if (piece_.get_position().y > lowest_y_) {
        lowest_y_ = piece_.get_position().y;
        lock_resets_ = 0;
        lock_timer_ = 0;
    }
    if (dx != 0)
        moved_on_ground();
    return true;
}

bool Tetris::try_rotate(int dir) {
    if (piece_.get_type() == TetrominoType::O)
        return false; // issue #2: the O piece never rotates

    const int from = piece_.get_rotation();
    const int to = (from + dir + 4) % 4;
    const auto &kicks = get_kicks(piece_.get_type(), from, to);
    for (int k = 0; k < 5; ++k) {
        Tetrimino t = piece_;
        t.set_rotation(to);
        t.move_by(kicks[k].x, kicks[k].y);
        if (!collides(t)) {
            piece_ = t;
            last_was_rotation_ = true;
            last_kick_ = k;
            if (piece_.get_position().y > lowest_y_) {
                lowest_y_ = piece_.get_position().y;
                lock_resets_ = 0;
                lock_timer_ = 0;
            }
            moved_on_ground();
            return true;
        }
    }
    return false;
}

void Tetris::spawn(TetrominoType type) {
    // Issue #1: pieces appear in the hidden rows just above the visible field,
    // then drop one row straight away if there is room (guideline behaviour).
    piece_ = Tetrimino(type, Point(3, HIDDEN - 2), 0);
    if (collides(piece_)) {
        phase_ = Phase::Over;
        events_ |= EV_GAME_OVER;
        return;
    }
    Tetrimino t = piece_;
    t.move_by(0, 1);
    if (!collides(t))
        piece_ = t;

    lowest_y_ = piece_.get_position().y;
    lock_timer_ = 0;
    lock_resets_ = 0;
    gravity_acc_ = 0;
    last_was_rotation_ = false;
}

void Tetris::spawn_next() {
    TetrominoType type = queue_[0];
    for (int i = 0; i < QUEUE - 1; ++i)
        queue_[i] = queue_[i + 1];
    queue_[QUEUE - 1] = bag_.next();
    can_hold_ = true;
    spawn(type);
}

void Tetris::do_hold() {
    if (!can_hold_)
        return;
    TetrominoType current = piece_.get_type();
    if (has_hold_) {
        TetrominoType held = hold_;
        hold_ = current;
        spawn(held);
    } else {
        hold_ = current;
        has_hold_ = true;
        spawn_next();
    }
    can_hold_ = false;
    events_ |= EV_HOLD;
}

Tetrimino Tetris::ghost() const {
    Tetrimino t = piece_;
    while (true) {
        Tetrimino n = t;
        n.move_by(0, 1);
        if (collides(n))
            return t;
        t = n;
    }
}

float Tetris::lock_progress() const {
    if (phase_ != Phase::Falling || !on_ground())
        return 0.0f;
    return static_cast<float>(lock_timer_) / LOCK_DELAY_MS;
}

bool Tetris::is_tspin() const {
    // 3-corner rule: T piece, last action a rotation, 3 of the 4 diagonal
    // corners around its centre occupied (walls count).
    if (piece_.get_type() != TetrominoType::T || !last_was_rotation_)
        return false;
    const Point c(piece_.get_position().x + 1, piece_.get_position().y + 1);
    int filled = 0;
    for (int dy = -1; dy <= 1; dy += 2)
        for (int dx = -1; dx <= 1; dx += 2) {
            int x = c.x + dx, y = c.y + dy;
            if (x < 0 || x >= WIDTH || y >= HEIGHT || (y >= 0 && board_[y][x] != EMPTY))
                ++filled;
        }
    return filled >= 3;
}

void Tetris::lock() {
    const bool tspin = is_tspin();

    bool any_visible = false;
    last_locked_ = piece_.blocks();
    for (const auto &p : piece_.blocks()) {
        board_[p.y][p.x] = piece_.get_type();
        if (p.y >= HIDDEN)
            any_visible = true;
    }
    events_ |= EV_LOCK;

    // Find full rows.
    clearing_count_ = 0;
    for (int y = 0; y < HEIGHT; ++y) {
        bool full = true;
        for (int x = 0; x < WIDTH; ++x)
            if (board_[y][x] == EMPTY) {
                full = false;
                break;
            }
        if (full && clearing_count_ < 4)
            clearing_rows_[clearing_count_++] = y;
    }

    const int n = clearing_count_;
    if (n == 0 && !any_visible) {
        // Lock out: the whole piece came to rest above the visible field.
        phase_ = Phase::Over;
        events_ |= EV_GAME_OVER;
        return;
    }

    static const uint32_t LINE_POINTS[5] = {0, 100, 300, 500, 800};
    static const uint32_t TSPIN_POINTS[4] = {400, 800, 1200, 1600};
    uint32_t points = tspin ? TSPIN_POINTS[n < 4 ? n : 3] : LINE_POINTS[n];
    points *= level_;

    ClearInfo info;
    info.lines = n;
    info.tspin = tspin;

    if (n > 0) {
        const bool difficult = n == 4 || tspin;
        if (difficult && b2b_) {
            points = points * 3 / 2;
            info.back_to_back = true;
        }
        b2b_ = difficult;
        ++combo_;
        if (combo_ > 0)
            points += 50 * combo_ * level_;
        info.combo = combo_;
        events_ |= EV_LINES;
    } else {
        combo_ = -1;
    }

    if (tspin)
        events_ |= EV_TSPIN;

    score_ += points;
    info.points = points;
    if (n > 0 || tspin)
        last_clear_ = info;

    if (n > 0) {
        phase_ = Phase::Clearing;
        clear_timer_ = CLEAR_MS;
    } else {
        spawn_next();
    }
}

void Tetris::finish_clear() {
    // Remove cleared rows (they are sorted top to bottom).
    for (int i = 0; i < clearing_count_; ++i) {
        for (int y = clearing_rows_[i]; y > 0; --y)
            board_[y] = board_[y - 1];
        board_[0].fill(EMPTY);
    }

    const int old_level = level_;
    lines_ += clearing_count_;
    int lvl = 1 + lines_ / 10;
    if (lvl < start_level_)
        lvl = start_level_;
    level_ = lvl > MAX_LEVEL ? MAX_LEVEL : lvl;
    if (level_ > old_level)
        events_ |= EV_LEVEL_UP;

    clearing_count_ = 0;
    phase_ = Phase::Falling;
    spawn_next();
}

void Tetris::update(uint32_t dt, const Input &in) {
    if (phase_ == Phase::Over)
        return;

    if (phase_ == Phase::Clearing) {
        clear_timer_ -= dt;
        if (clear_timer_ <= 0)
            finish_clear();
        // Remember the direction held so DAS carries into the next piece.
        das_dir_ = in.left ? -1 : (in.right ? 1 : 0);
        return;
    }

    if (in.hold) {
        do_hold();
        if (phase_ == Phase::Over)
            return;
    }

    if (in.rotate_cw && try_rotate(1))
        events_ |= EV_ROTATE;
    if (in.rotate_ccw && try_rotate(-1))
        events_ |= EV_ROTATE;

    // Horizontal movement with delayed auto shift.
    if (in.left_p || in.right_p) {
        das_dir_ = in.left_p ? -1 : 1;
        das_timer_ = DAS_MS;
        if (try_move(das_dir_, 0))
            events_ |= EV_MOVE;
    } else {
        if ((das_dir_ == -1 && !in.left) || (das_dir_ == 1 && !in.right)) {
            // Released: fall back to the other direction if it is still held.
            das_dir_ = in.left ? -1 : (in.right ? 1 : 0);
            das_timer_ = DAS_MS;
        }
        if (das_dir_ != 0) {
            das_timer_ -= dt;
            while (das_timer_ <= 0) {
                das_timer_ += ARR_MS;
                if (try_move(das_dir_, 0))
                    events_ |= EV_MOVE;
                else
                    break;
            }
            if (das_timer_ < 0)
                das_timer_ = 0;
        }
    }

    if (in.hard_drop) {
        Tetrimino g = ghost();
        int dist = g.get_position().y - piece_.get_position().y;
        trail_.to = g.blocks();
        trail_.distance = dist;
        score_ += 2 * dist;
        piece_ = g;
        last_was_rotation_ = last_was_rotation_ && dist == 0;
        events_ |= EV_HARD_DROP;
        lock();
        return;
    }

    // Gravity, with soft drop.
    const float g = gravity_ms();
    float interval = g;
    if (in.down) {
        float soft = g / 20.0f;
        if (soft < 25.0f)
            soft = 25.0f;
        if (soft < interval)
            interval = soft;
    }
    gravity_acc_ += dt;
    while (gravity_acc_ >= interval) {
        gravity_acc_ -= interval;
        if (!try_move(0, 1)) {
            gravity_acc_ = 0;
            break;
        }
        if (in.down)
            score_ += 1;
    }

    // Lock delay.
    if (on_ground()) {
        lock_timer_ += dt;
        if (lock_timer_ >= LOCK_DELAY_MS)
            lock();
    }
}
