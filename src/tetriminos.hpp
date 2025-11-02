#pragma once

#include "32blit.hpp"
#include <array>
#include <cstdint>

using namespace blit;

// Public declarations for tetrimino coordinate-list representation.
// Each tetromino is represented as 4 block coordinates (x,y) relative to a local origin.

enum class TetrominoType : uint8_t { I = 0, O, T, S, Z, J, L, COUNT };

extern const std::array<std::array<Point, 4>, 7> TETROMINOS;

Point rotate_point_cw(const Point &p);
std::array<Point, 4> rotate_shape_cw(const std::array<Point, 4> &shape);
std::array<Point, 4> get_rotated_shape(const std::array<Point, 4> &shape, int rotation);
std::array<Point, 4> get_tetromino(TetrominoType type, int rotation = 0);

// Color (Pen) for each tetromino type. Order matches TetrominoType.
extern const std::array<Pen, 7> TETROMINO_COLORS;
Pen get_tetromino_color(TetrominoType type);

// Class representing a tetrimino instance on the board.
class Tetrimino {
public:
	// Construct a tetrimino of `type` at `position` with `rotation` (0..3)
	Tetrimino(TetrominoType type, Point position = Point(0, 0), int rotation = 0);

    static Tetrimino random_tetrimino(Point position = Point(0, 0));

	TetrominoType get_type() const;
	int get_rotation() const; // 0..3
	Point get_position() const;

	void set_position(const Point &p);
	void move_by(int dx, int dy);

	// Rotate 90° clockwise / counter-clockwise
	void rotate_cw();
	void rotate_ccw();

	// Return the 4 block coordinates in board space (translated by position)
	std::array<Point, 4> blocks() const;

	// Get drawing color for this tetrimino
	Pen color() const;

private:
	TetrominoType type_;
	int rotation_; // normalized to 0..3
	Point position_;
};
