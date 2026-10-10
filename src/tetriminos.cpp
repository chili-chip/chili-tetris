#include "tetriminos.hpp"

#include <cstdlib>

namespace {

// Spawn orientation of each piece inside its bounding box (y grows downwards).
// I uses a 4x4 box, O a 4x3 box (centred, never rotates), the rest 3x3.
const std::array<std::array<Point, 4>, 7> SPAWN_SHAPES = {{
    {Point(0, 1), Point(1, 1), Point(2, 1), Point(3, 1)}, // I
    {Point(1, 0), Point(2, 0), Point(1, 1), Point(2, 1)}, // O
    {Point(1, 0), Point(0, 1), Point(1, 1), Point(2, 1)}, // T
    {Point(1, 0), Point(2, 0), Point(0, 1), Point(1, 1)}, // S
    {Point(0, 0), Point(1, 0), Point(1, 1), Point(2, 1)}, // Z
    {Point(0, 0), Point(0, 1), Point(1, 1), Point(2, 1)}, // J
    {Point(2, 0), Point(0, 1), Point(1, 1), Point(2, 1)}, // L
}};

const std::array<int, 7> BOX_SIZE = {4, 0, 3, 3, 3, 3, 3};

// SRS kick tables, written as in the guideline (y up) and flipped to y down
// when looked up. Index: 0 = 0->R, 1 = R->0, 2 = R->2, 3 = 2->R,
// 4 = 2->L, 5 = L->2, 6 = L->0, 7 = 0->L.
std::array<std::array<Point, 5>, 8> make_kicks(const int table[8][5][2]) {
    std::array<std::array<Point, 5>, 8> out{};
    for (int i = 0; i < 8; ++i)
        for (int k = 0; k < 5; ++k)
            out[i][k] = Point(table[i][k][0], -table[i][k][1]);
    return out;
}

const int JLSTZ_TABLE[8][5][2] = {
    {{0, 0}, {-1, 0}, {-1, 1}, {0, -2}, {-1, -2}},
    {{0, 0}, {1, 0}, {1, -1}, {0, 2}, {1, 2}},
    {{0, 0}, {1, 0}, {1, -1}, {0, 2}, {1, 2}},
    {{0, 0}, {-1, 0}, {-1, 1}, {0, -2}, {-1, -2}},
    {{0, 0}, {1, 0}, {1, 1}, {0, -2}, {1, -2}},
    {{0, 0}, {-1, 0}, {-1, -1}, {0, 2}, {-1, 2}},
    {{0, 0}, {-1, 0}, {-1, -1}, {0, 2}, {-1, 2}},
    {{0, 0}, {1, 0}, {1, 1}, {0, -2}, {1, -2}},
};

const int I_TABLE[8][5][2] = {
    {{0, 0}, {-2, 0}, {1, 0}, {-2, -1}, {1, 2}},
    {{0, 0}, {2, 0}, {-1, 0}, {2, 1}, {-1, -2}},
    {{0, 0}, {-1, 0}, {2, 0}, {-1, 2}, {2, -1}},
    {{0, 0}, {1, 0}, {-2, 0}, {1, -2}, {-2, 1}},
    {{0, 0}, {2, 0}, {-1, 0}, {2, 1}, {-1, -2}},
    {{0, 0}, {-2, 0}, {1, 0}, {-2, -1}, {1, 2}},
    {{0, 0}, {1, 0}, {-2, 0}, {1, -2}, {-2, 1}},
    {{0, 0}, {-1, 0}, {2, 0}, {-1, 2}, {2, -1}},
};

int kick_index(int from, int to) {
    if (from == 0 && to == 1) return 0;
    if (from == 1 && to == 0) return 1;
    if (from == 1 && to == 2) return 2;
    if (from == 2 && to == 1) return 3;
    if (from == 2 && to == 3) return 4;
    if (from == 3 && to == 2) return 5;
    if (from == 3 && to == 0) return 6;
    return 7; // 0 -> 3
}

// Colours for each tetromino (I, O, T, S, Z, J, L). Chili-warm takes on the
// classic palette so the board reads well on the small OLED.
const std::array<Pen, 7> TETROMINO_COLORS = {{
    Pen(40, 210, 230),  // I - cyan
    Pen(255, 210, 40),  // O - yellow
    Pen(170, 70, 230),  // T - purple
    Pen(80, 210, 60),   // S - green
    Pen(235, 45, 45),   // Z - chili red
    Pen(50, 100, 240),  // J - blue
    Pen(255, 130, 20),  // L - orange
}};

} // namespace

std::array<Point, 4> get_tetromino(TetrominoType type, int rotation) {
    const auto idx = static_cast<size_t>(type);
    std::array<Point, 4> out = SPAWN_SHAPES[idx];
    const int n = BOX_SIZE[idx];
    if (n == 0)
        return out; // O piece does not rotate (issue #2)

    int r = ((rotation % 4) + 4) % 4;
    for (int i = 0; i < r; ++i)
        for (auto &p : out)
            p = Point(n - 1 - p.y, p.x); // 90 degrees clockwise inside the box
    return out;
}

const std::array<Point, 5> &get_kicks(TetrominoType type, int from, int to) {
    static const auto jlstz = make_kicks(JLSTZ_TABLE);
    static const auto i_kicks = make_kicks(I_TABLE);
    static const std::array<Point, 5> none = {Point(0, 0), Point(0, 0), Point(0, 0), Point(0, 0), Point(0, 0)};
    if (type == TetrominoType::O)
        return none;
    const auto &table = type == TetrominoType::I ? i_kicks : jlstz;
    return table[kick_index(from, to)];
}

Pen get_tetromino_color(TetrominoType type) {
    if (type == TetrominoType::COUNT)
        return Pen(0, 0, 0);
    return TETROMINO_COLORS[static_cast<size_t>(type)];
}

Tetrimino::Tetrimino(TetrominoType type, Point position, int rotation)
    : type_(type), rotation_(((rotation % 4) + 4) % 4), position_(position) {}

std::array<Point, 4> Tetrimino::blocks() const {
    auto rel = get_tetromino(type_, rotation_);
    for (auto &p : rel)
        p = Point(p.x + position_.x, p.y + position_.y);
    return rel;
}

void PieceBag::reset() { index_ = 7; }

TetrominoType PieceBag::next() {
    if (index_ >= 7) {
        for (int i = 0; i < 7; ++i)
            bag_[i] = static_cast<TetrominoType>(i);
        for (int i = 6; i > 0; --i) {
            int j = rand() % (i + 1);
            std::swap(bag_[i], bag_[j]);
        }
        index_ = 0;
    }
    return bag_[index_++];
}
