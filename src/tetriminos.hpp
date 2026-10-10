#pragma once

#include "32blit.hpp"
#include <array>
#include <cstdint>

using namespace blit;

// The seven tetrominoes. COUNT doubles as "empty cell" on the board.
enum class TetrominoType : uint8_t { I = 0, O, T, S, Z, J, L, COUNT };

constexpr int TETROMINO_KINDS = static_cast<int>(TetrominoType::COUNT);

// Cells of `type` in rotation state `rotation` (0 = spawn, 1 = R, 2 = 180, 3 = L),
// relative to the top-left of the piece's bounding box (SRS layout).
std::array<Point, 4> get_tetromino(TetrominoType type, int rotation = 0);

// SRS wall-kick offsets (x right, y down) tried in order when rotating
// from `from` to `to`. Always 5 entries; the first one is (0, 0).
const std::array<Point, 5> &get_kicks(TetrominoType type, int from, int to);

Pen get_tetromino_color(TetrominoType type);

// A tetromino placed on the board: type, rotation state and the board
// position of its bounding box.
class Tetrimino {
public:
    Tetrimino(TetrominoType type = TetrominoType::T, Point position = Point(0, 0), int rotation = 0);

    TetrominoType get_type() const { return type_; }
    int get_rotation() const { return rotation_; }
    Point get_position() const { return position_; }

    void set_position(const Point &p) { position_ = p; }
    void set_rotation(int r) { rotation_ = ((r % 4) + 4) % 4; }
    void move_by(int dx, int dy) { position_ = Point(position_.x + dx, position_.y + dy); }

    // Block coordinates in board space.
    std::array<Point, 4> blocks() const;

    Pen color() const { return get_tetromino_color(type_); }

private:
    TetrominoType type_;
    int rotation_;
    Point position_;
};

// 7-bag randomiser: every run of seven pieces contains each piece once.
class PieceBag {
public:
    void reset();
    TetrominoType next();

private:
    std::array<TetrominoType, 7> bag_{};
    int index_ = 7;
};
