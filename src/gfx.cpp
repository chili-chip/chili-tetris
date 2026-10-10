#include "gfx.hpp"

#include <cctype>

namespace gfx {

const Pen BG(14, 10, 28);
const Pen FIELD_BG(8, 8, 18);
const Pen FIELD_DOT(32, 30, 58);
const Pen FRAME(90, 70, 130);
const Pen FRAME_LIGHT(170, 140, 220);
const Pen PANEL_BG(22, 18, 42);
const Pen LABEL(255, 170, 40);
const Pen TEXT(250, 245, 255);
const Pen TEXT_DIM(140, 130, 170);
const Pen CHILI_RED(235, 45, 45);

Pen lighten(Pen p, int a) {
    return Pen(p.r + ((255 - p.r) * a) / 255, p.g + ((255 - p.g) * a) / 255, p.b + ((255 - p.b) * a) / 255, p.a);
}

Pen darken(Pen p, int a) {
    return Pen((p.r * (255 - a)) / 255, (p.g * (255 - a)) / 255, (p.b * (255 - a)) / 255, p.a);
}

void block(int x, int y, int size, Pen color) {
    if (size <= 2) {
        screen.pen = color;
        screen.rectangle(Rect(x, y, size, size));
        return;
    }
    screen.pen = darken(color, 90);
    screen.rectangle(Rect(x, y, size, size));
    screen.pen = color;
    screen.rectangle(Rect(x, y, size - 1, size - 1));
    screen.pen = lighten(color, 110);
    screen.h_span(Point(x, y), size - 1);
    screen.v_span(Point(x, y), size - 1);
    if (size >= 5) {
        screen.pen = lighten(color, 200);
        screen.pixel(Point(x + 1, y + 1));
    }
}

void ghost_block(int x, int y, int size, Pen color) {
    Pen c = color;
    c.a = 70;
    screen.pen = c;
    screen.rectangle(Rect(x + 1, y + 1, size - 2, size - 2));
    c.a = 200;
    screen.pen = darken(c, 40);
    screen.h_span(Point(x, y), size);
    screen.h_span(Point(x, y + size - 1), size);
    screen.v_span(Point(x, y), size);
    screen.v_span(Point(x + size - 1, y), size);
}

void piece_preview(TetrominoType type, const Rect &area, int size, bool dim) {
    if (type == TetrominoType::COUNT)
        return;
    auto cells = get_tetromino(type, 0);
    int minx = 9, maxx = -9, miny = 9, maxy = -9;
    for (auto &c : cells) {
        minx = std::min(minx, c.x);
        maxx = std::max(maxx, c.x);
        miny = std::min(miny, c.y);
        maxy = std::max(maxy, c.y);
    }
    int w = (maxx - minx + 1) * size;
    int h = (maxy - miny + 1) * size;
    int ox = area.x + (area.w - w) / 2;
    int oy = area.y + (area.h - h) / 2;
    Pen col = get_tetromino_color(type);
    if (dim)
        col = darken(Pen(90, 90, 110), 0);
    for (auto &c : cells)
        block(ox + (c.x - minx) * size, oy + (c.y - miny) * size, size, col);
}

void panel(const Rect &r, std::string_view label) {
    screen.pen = PANEL_BG;
    screen.rectangle(r);
    screen.pen = FRAME;
    screen.h_span(Point(r.x + 1, r.y), r.w - 2);
    screen.h_span(Point(r.x + 1, r.y + r.h - 1), r.w - 2);
    screen.v_span(Point(r.x, r.y + 1), r.h - 2);
    screen.v_span(Point(r.x + r.w - 1, r.y + 1), r.h - 2);
    if (!label.empty()) {
        screen.pen = LABEL;
        screen.text(label, minimal_font, Point(r.x + 3, r.y + 2));
    }
}

void text_outlined(std::string_view s, const Point &p, Pen color, TextAlign align, const Font &font) {
    screen.pen = Pen(0, 0, 0);
    for (int dy = -1; dy <= 1; ++dy)
        for (int dx = -1; dx <= 1; ++dx)
            if (dx || dy)
                screen.text(s, font, Point(p.x + dx, p.y + dy), true, align);
    screen.pen = color;
    screen.text(s, font, p, true, align);
}

namespace {

// 3x5 glyphs, rows top to bottom.
const char *glyph(char c) {
    switch (toupper(c)) {
    case 'A': return "010101111101101";
    case 'B': return "110101110101110";
    case 'C': return "011100100100011";
    case 'D': return "110101101101110";
    case 'E': return "111100110100111";
    case 'F': return "111100110100100";
    case 'G': return "011100101101011";
    case 'H': return "101101111101101";
    case 'I': return "111010010010111";
    case 'J': return "001001001101010";
    case 'K': return "101101110101101";
    case 'L': return "100100100100111";
    case 'M': return "101111111101101";
    case 'N': return "110101101101101";
    case 'O': return "010101101101010";
    case 'P': return "110101110100100";
    case 'Q': return "010101101110011";
    case 'R': return "110101110101101";
    case 'S': return "011100010001110";
    case 'T': return "111010010010010";
    case 'U': return "101101101101111";
    case 'V': return "101101101101010";
    case 'W': return "101101111111101";
    case 'X': return "101101010101101";
    case 'Y': return "101101010010010";
    case 'Z': return "111001010100111";
    case '0': return "111101101101111";
    case '1': return "010110010010111";
    case '2': return "110001010100111";
    case '3': return "110001010001110";
    case '4': return "101101111001001";
    case '5': return "111100110001110";
    case '6': return "011100111101111";
    case '7': return "111001010010010";
    case '8': return "111101111101111";
    case '9': return "111101111001110";
    case '!': return "010010010000010";
    case '-': return "000000111000000";
    case 'x': return "000101010101000";
    default: return nullptr;
    }
}

} // namespace

int big_text_width(std::string_view s, int scale) {
    if (s.empty())
        return 0;
    return static_cast<int>(s.size()) * 4 * scale - scale;
}

void big_text(std::string_view s, Point p, int scale, Pen color, bool blocks, bool shadow) {
    for (int pass = shadow ? 0 : 1; pass < 2; ++pass) {
        int x = p.x;
        for (char ch : s) {
            const char *g = glyph(ch);
            if (g) {
                for (int i = 0; i < 15; ++i) {
                    if (g[i] != '1')
                        continue;
                    int px = x + (i % 3) * scale;
                    int py = p.y + (i / 3) * scale;
                    if (pass == 0) {
                        screen.pen = Pen(0, 0, 0, 200);
                        screen.rectangle(Rect(px + 1, py + 1, scale, scale));
                    } else if (blocks) {
                        block(px, py, scale, color);
                    } else {
                        screen.pen = color;
                        screen.rectangle(Rect(px, py, scale, scale));
                    }
                }
            }
            x += 4 * scale;
        }
    }
}

void chili(int x, int y) {
    static const char *art[] = {
        "......gg..",
        ".....g....",
        "...rrrg...",
        "..rwrrrr..",
        ".rwrrrrr..",
        ".rrrrrrd..",
        "..rrrrd...",
        "...rrd....",
        "....d.....",
    };
    for (int j = 0; j < 9; ++j)
        for (int i = 0; i < 10; ++i) {
            char c = art[j][i];
            if (c == '.')
                continue;
            switch (c) {
            case 'g': screen.pen = Pen(70, 190, 60); break;
            case 'w': screen.pen = Pen(255, 160, 150); break;
            case 'd': screen.pen = Pen(150, 20, 30); break;
            default: screen.pen = CHILI_RED; break;
            }
            screen.pixel(Point(x + i, y + j));
        }
}

} // namespace gfx
