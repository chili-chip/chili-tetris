#pragma once

#include "32blit.hpp"
#include "tetriminos.hpp"

#include <string_view>

using namespace blit;

namespace gfx {

// Palette
extern const Pen BG;
extern const Pen FIELD_BG;
extern const Pen FIELD_DOT;
extern const Pen FRAME;
extern const Pen FRAME_LIGHT;
extern const Pen PANEL_BG;
extern const Pen LABEL;
extern const Pen TEXT;
extern const Pen TEXT_DIM;
extern const Pen CHILI_RED;

Pen lighten(Pen p, int amount); // amount 0..255 towards white
Pen darken(Pen p, int amount);  // amount 0..255 towards black

// Bevelled block tile of `size` pixels.
void block(int x, int y, int size, Pen color);
// Outline used for the ghost piece.
void ghost_block(int x, int y, int size, Pen color);
// A whole tetromino in spawn orientation, centred in `area`.
void piece_preview(TetrominoType type, const Rect &area, int size, bool dim = false);

// Framed panel box with an optional label on its top edge.
void panel(const Rect &r, std::string_view label = {});

// Text with a 1px dark outline so it reads over anything.
void text_outlined(std::string_view s, const Point &p, Pen color, TextAlign align = TextAlign::top_left,
                   const Font &font = minimal_font);

// Chunky 3x5 pixel-font text where every pixel is a `scale`-sized square
// (or a bevelled block when `blocks` is set). Returns the drawn width.
int big_text_width(std::string_view s, int scale);
void big_text(std::string_view s, Point p, int scale, Pen color, bool blocks = false, bool shadow = true);

// The little chili pepper mascot (10x9 pixels).
void chili(int x, int y);

} // namespace gfx
