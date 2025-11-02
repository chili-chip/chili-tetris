#include "tetriminos.hpp"

// The seven standard tetromino shapes in a canonical orientation.
// Coordinates chosen so pieces are roughly centered around the origin for easy rotation.
const std::array<std::array<Point, 4>, 7> TETROMINOS = {{
    // I: a horizontal line of 4
    { Point(-2, 0), Point(-1, 0), Point(0, 0), Point(1, 0) },

    // O: 2x2 square
    { Point(0, 0), Point(1, 0), Point(0, 1), Point(1, 1) },

    // T: T shape with stem down
    { Point(-1, 0), Point(0, 0), Point(1, 0), Point(0, 1) },

    // S: S shape
    { Point(0, 0), Point(1, 0), Point(-1, 1), Point(0, 1) },

    // Z: Z shape
    { Point(-1, 0), Point(0, 0), Point(0, 1), Point(1, 1) },

    // J: J shape (mirror of L)
    { Point(-1, 0), Point(-1, 1), Point(0, 0), Point(1, 0) },

    // L: L shape
    { Point(-1, 0), Point(0, 0), Point(1, 0), Point(1, 1) }
}};

// Colors for each tetromino (I, O, T, S, Z, J, L)
const std::array<Pen, 7> TETROMINO_COLORS = {{
    Pen(0, 255, 255),   // I - cyan
    Pen(255, 255, 0),   // O - yellow
    Pen(160, 32, 240),  // T - purple
    Pen(0, 255, 0),     // S - green
    Pen(255, 0, 0),     // Z - red
    Pen(0, 0, 255),     // J - blue
    Pen(255, 165, 0)    // L - orange
}};

// Rotate a coordinate 90° clockwise about the origin: (x,y) -> (y, -x)
Point rotate_point_cw(const Point &p) {
    return Point(p.y, -p.x);
}

// Rotate a tetromino shape (array of 4 points) 90° clockwise
std::array<Point, 4> rotate_shape_cw(const std::array<Point, 4> &shape) {
    std::array<Point, 4> out;
    for (size_t i = 0; i < 4; ++i)
        out[i] = rotate_point_cw(shape[i]);
    return out;
}

// Return a shape rotated `rotation` times 90° clockwise (rotation may be negative or >3).
std::array<Point, 4> get_rotated_shape(const std::array<Point, 4> &shape, int rotation) {
    int r = rotation % 4;
    if (r < 0)
        r += 4;
    std::array<Point, 4> out = shape;
    for (int i = 0; i < r; ++i)
        out = rotate_shape_cw(out);
    return out;
}

// Convenience: get a tetromino by type and rotation index (0..3)
std::array<Point, 4> get_tetromino(TetrominoType type, int rotation) {
    return get_rotated_shape(TETROMINOS[static_cast<size_t>(type)], rotation);
}

Pen get_tetromino_color(TetrominoType type) {
    return TETROMINO_COLORS[static_cast<size_t>(type)];
}

// Tetrimino class implementation
Tetrimino::Tetrimino(TetrominoType type, Point position, int rotation)
    : type_(type), rotation_(rotation % 4), position_(position) {
    if (rotation_ < 0)
        rotation_ += 4;
}

Tetrimino Tetrimino::random_tetrimino(Point position) {
    TetrominoType type = static_cast<TetrominoType>(rand() % static_cast<int>(TetrominoType::COUNT));
    return Tetrimino(type, position);
}

TetrominoType Tetrimino::get_type() const { return type_; }

int Tetrimino::get_rotation() const { return rotation_; }

Point Tetrimino::get_position() const { return position_; }

void Tetrimino::set_position(const Point &p) { position_ = p; }

void Tetrimino::move_by(int dx, int dy) { position_ = Point(position_.x + dx, position_.y + dy); }

void Tetrimino::rotate_cw() { rotation_ = (rotation_ + 1) % 4; }

void Tetrimino::rotate_ccw() { rotation_ = (rotation_ + 3) % 4; }

std::array<Point, 4> Tetrimino::blocks() const {
    auto rel = get_tetromino(type_, rotation_);
    std::array<Point, 4> out;
    for (size_t i = 0; i < 4; ++i)
        out[i] = Point(rel[i].x + position_.x, rel[i].y + position_.y);
    return out;
}

Pen Tetrimino::color() const { return get_tetromino_color(type_); }
