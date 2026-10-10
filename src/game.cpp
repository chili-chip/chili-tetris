#include "game.hpp"

#include "audio.hpp"
#include "gfx.hpp"
#include "save.hpp"
#include "tetris.hpp"

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>

using namespace blit;

namespace {

// --- Layout (128x128) --------------------------------------------------------

constexpr int CELL = 6;
constexpr int FX = 4; // playfield origin
constexpr int FY = 4;
constexpr int FW = Tetris::WIDTH * CELL;
constexpr int FH = Tetris::VISIBLE * CELL;
constexpr int PANEL_X = 68;
constexpr int PANEL_W = 58;

const char *VERSION = "v1.0.0";

// --- State -------------------------------------------------------------------

enum class Scene { Title, Playing, HighScores, Help, Credits };
enum class PlayState { Countdown, Running, Paused, Dying, NameEntry, Results };

enum TitleItem { T_PLAY, T_LEVEL, T_MUSIC, T_SOUND, T_GHOST, T_SCORES, T_HELP, T_CREDITS, T_COUNT };
enum PauseItem { P_RESUME, P_RESTART, P_MUSIC, P_SOUND, P_QUIT, P_COUNT };

Tetris game;
Scene scene = Scene::Title;
PlayState play_state = PlayState::Countdown;

uint32_t last_time = 0;
uint32_t anim = 0;        // ms since the current scene started
uint32_t state_timer = 0; // ms since the current play state started
int title_sel = 0;
int pause_sel = 0;

// effects
uint32_t shake_timer = 0;
int shake_strength = 0;
uint32_t trail_timer = 0;
uint32_t lock_flash_timer = 0;
uint32_t callout_timer = 0;
std::string callout_big, callout_small;
Pen callout_color;
uint32_t level_banner_timer = 0;

// game over / high score
int new_rank = -1;
int highlight_rank = -1;
char entry_name[4] = "AAA";
int entry_cursor = 0;

struct Particle {
    float x, y, vx, vy;
    int32_t life, max_life;
    Pen color;
};
std::vector<Particle> particles;

struct DecoPiece {
    TetrominoType type;
    float x, y, speed;
    int rot;
};
DecoPiece deco[7];

float frand(float a, float b) { return a + (b - a) * (rand() % 1000) / 1000.0f; }

bool pressed_btn(uint32_t b) { return buttons.pressed & b; }
bool held_btn(uint32_t b) { return buttons.state & b; }

void apply_settings() {
    audio::set_music_enabled(save_data().music);
    audio::set_sfx_enabled(save_data().sfx);
}

// --- Effects -----------------------------------------------------------------

void spawn_particles(float x, float y, Pen color, int count, float power) {
    for (int i = 0; i < count && particles.size() < 220; ++i) {
        Particle p;
        p.x = x;
        p.y = y;
        p.vx = frand(-0.07f, 0.07f) * power;
        p.vy = frand(-0.12f, 0.01f) * power;
        p.max_life = p.life = static_cast<int32_t>(frand(450, 900));
        p.color = color;
        particles.push_back(p);
    }
}

void update_particles(uint32_t dt) {
    for (auto &p : particles) {
        p.vy += 0.00035f * dt;
        p.x += p.vx * dt;
        p.y += p.vy * dt;
        p.life -= dt;
    }
    for (size_t i = 0; i < particles.size();) {
        if (particles[i].life <= 0 || particles[i].y > 130) {
            particles[i] = particles.back();
            particles.pop_back();
        } else {
            ++i;
        }
    }
}

void draw_particles() {
    for (auto &p : particles) {
        Pen c = p.color;
        c.a = static_cast<uint8_t>(255 * p.life / p.max_life);
        screen.pen = c;
        int s = p.life > p.max_life / 2 ? 2 : 1;
        screen.rectangle(Rect(static_cast<int>(p.x), static_cast<int>(p.y), s, s));
    }
}

void shake(uint32_t ms, int strength) {
    if (strength >= shake_strength || shake_timer == 0) {
        shake_timer = ms;
        shake_strength = strength;
    }
}

void show_callout(const std::string &big, const std::string &small, Pen color) {
    callout_big = big;
    callout_small = small;
    callout_color = color;
    callout_timer = 1300;
}

void reset_deco(DecoPiece &d, bool anywhere) {
    d.type = static_cast<TetrominoType>(rand() % 7);
    d.x = frand(-8, 120);
    d.y = anywhere ? frand(-20, 128) : frand(-40, -20);
    d.speed = frand(0.008f, 0.02f);
    d.rot = rand() % 4;
}

// --- Scene changes -------------------------------------------------------------

void go_title() {
    scene = Scene::Title;
    anim = 0;
    audio::set_tempo(1.0f);
    audio::play_song(audio::Song::Title);
}

void start_game() {
    game.start(save_data().start_level);
    scene = Scene::Playing;
    play_state = PlayState::Countdown;
    state_timer = 0;
    particles.clear();
    callout_timer = level_banner_timer = trail_timer = lock_flash_timer = shake_timer = 0;
    new_rank = highlight_rank = -1;
    audio::stop_song();
    audio::play(audio::Sfx::Countdown);
}

void set_play_state(PlayState s) {
    play_state = s;
    state_timer = 0;
}

float game_tempo() {
    float t = 1.0f + (game.level() - 1) * 0.035f;
    return t > 1.6f ? 1.6f : t;
}

// --- Update ------------------------------------------------------------------------

void update_title() {
    const int sel_before = title_sel;
    if (pressed_btn(Button::DPAD_UP))
        title_sel = (title_sel + T_COUNT - 1) % T_COUNT;
    if (pressed_btn(Button::DPAD_DOWN))
        title_sel = (title_sel + 1) % T_COUNT;
    if (title_sel != sel_before)
        audio::play(audio::Sfx::MenuMove);

    auto &sd = save_data();
    const bool left = pressed_btn(Button::DPAD_LEFT);
    const bool right = pressed_btn(Button::DPAD_RIGHT);
    const bool a = pressed_btn(Button::A);
    bool changed = false;

    switch (title_sel) {
    case T_PLAY:
        if (a) {
            audio::play(audio::Sfx::MenuSelect);
            start_game();
        }
        break;
    case T_LEVEL:
        if (left && sd.start_level > 1) {
            --sd.start_level;
            changed = true;
        }
        if ((right || a) && sd.start_level < 15) {
            ++sd.start_level;
            changed = true;
        } else if (a) {
            sd.start_level = 1;
            changed = true;
        }
        break;
    case T_MUSIC:
        if (left || right || a) {
            sd.music = !sd.music;
            changed = true;
        }
        break;
    case T_SOUND:
        if (left || right || a) {
            sd.sfx = !sd.sfx;
            changed = true;
        }
        break;
    case T_GHOST:
        if (left || right || a) {
            sd.ghost = !sd.ghost;
            changed = true;
        }
        break;
    case T_SCORES:
    case T_HELP:
    case T_CREDITS:
        if (a) {
            audio::play(audio::Sfx::MenuSelect);
            highlight_rank = -1;
            scene = title_sel == T_SCORES ? Scene::HighScores : (title_sel == T_HELP ? Scene::Help : Scene::Credits);
            anim = 0;
        }
        break;
    }

    if (changed) {
        apply_settings();
        store_save();
        audio::play(audio::Sfx::MenuMove);
    }

    for (auto &d : deco) {
        d.y += d.speed * 10;
        if (d.y > 130)
            reset_deco(d, false);
    }
}

void update_info_screen() {
    if (pressed_btn(Button::A) || pressed_btn(Button::B) || pressed_btn(Button::MENU)) {
        audio::play(audio::Sfx::MenuBack);
        go_title();
    }
}

void handle_events(uint32_t ev) {
    using audio::Sfx;
    if (ev & EV_HOLD)
        audio::play(Sfx::Hold);
    if (ev & EV_ROTATE)
        audio::play(Sfx::Rotate);
    if (ev & EV_MOVE)
        audio::play(Sfx::Move);

    if (ev & EV_HARD_DROP) {
        trail_timer = 160;
        shake(90, 1);
    }

    if (ev & EV_LINES) {
        const ClearInfo &c = game.last_clear();
        static const char *names[] = {"", "SINGLE", "DOUBLE", "TRIPLE", "TETRIS!"};
        std::string big = c.tspin ? "T-SPIN" : names[c.lines];
        std::string small;
        if (c.tspin)
            small = names[c.lines];
        if (c.back_to_back)
            small = small.empty() ? "BACK-TO-BACK" : "B2B " + small;
        if (c.combo > 0)
            small += (small.empty() ? "" : " ") + std::string("COMBO ") + std::to_string(c.combo);
        if (small.empty())
            small = "+" + std::to_string(c.points);

        Pen color = c.lines == 4 ? Pen(40, 210, 230) : (c.tspin ? Pen(190, 100, 255) : gfx::LABEL);
        if (c.lines >= 2 || c.tspin)
            show_callout(big, small, color);

        if (c.tspin)
            audio::play(Sfx::TSpin);
        else
            audio::play(c.lines == 4 ? Sfx::Tetris : Sfx::Line);

        // Burst of particles from every cleared cell.
        for (int i = 0; i < game.clearing_count(); ++i) {
            int row = game.clearing_row(i) - Tetris::HIDDEN;
            for (int x = 0; x < Tetris::WIDTH; ++x) {
                Pen col = get_tetromino_color(game.cell(x, game.clearing_row(i)));
                spawn_particles(FX + x * CELL + CELL / 2, FY + row * CELL + CELL / 2, col, c.lines == 4 ? 3 : 2,
                                c.lines == 4 ? 1.6f : 1.0f);
            }
        }
        if (c.lines == 4)
            shake(350, 3);
        else if (c.lines >= 2)
            shake(150, 1);
    } else if (ev & EV_TSPIN) {
        show_callout("T-SPIN", "+" + std::to_string(game.last_clear().points), Pen(190, 100, 255));
        audio::play(Sfx::TSpin);
    } else if (ev & EV_HARD_DROP) {
        audio::play(Sfx::HardDrop);
    } else if (ev & EV_LOCK) {
        audio::play(Sfx::Lock);
    }

    if (ev & EV_LOCK)
        lock_flash_timer = 120;

    if (ev & EV_LEVEL_UP) {
        level_banner_timer = 1500;
        audio::play(Sfx::LevelUp);
        audio::set_tempo(game_tempo());
    }
}

void update_running(uint32_t dt) {
    if (pressed_btn(Button::MENU)) {
        set_play_state(PlayState::Paused);
        pause_sel = 0;
        audio::stop_song();
        audio::play(audio::Sfx::Pause);
        return;
    }

    Tetris::Input in{};
    in.left = held_btn(Button::DPAD_LEFT);
    in.right = held_btn(Button::DPAD_RIGHT);
    in.down = held_btn(Button::DPAD_DOWN);
    in.left_p = pressed_btn(Button::DPAD_LEFT);
    in.right_p = pressed_btn(Button::DPAD_RIGHT);
    in.hard_drop = pressed_btn(Button::DPAD_UP);
    in.rotate_cw = pressed_btn(Button::A);
    in.rotate_ccw = pressed_btn(Button::B);
    in.hold = pressed_btn(Button::X) || pressed_btn(Button::Y);

    game.update(dt, in);
    uint32_t ev = game.take_events();
    handle_events(ev);

    if (ev & EV_GAME_OVER) {
        set_play_state(PlayState::Dying);
        audio::stop_song();
        audio::play(audio::Sfx::GameOver);
        shake(300, 2);
    }
}

void update_paused() {
    const int before = pause_sel;
    if (pressed_btn(Button::DPAD_UP))
        pause_sel = (pause_sel + P_COUNT - 1) % P_COUNT;
    if (pressed_btn(Button::DPAD_DOWN))
        pause_sel = (pause_sel + 1) % P_COUNT;
    if (before != pause_sel)
        audio::play(audio::Sfx::MenuMove);

    auto &sd = save_data();
    const bool toggle = pressed_btn(Button::A) || pressed_btn(Button::DPAD_LEFT) || pressed_btn(Button::DPAD_RIGHT);

    if (pressed_btn(Button::MENU) || pressed_btn(Button::B) || (pause_sel == P_RESUME && pressed_btn(Button::A))) {
        audio::play(audio::Sfx::MenuSelect);
        set_play_state(PlayState::Running);
        audio::set_tempo(game_tempo());
        audio::play_song(audio::Song::Game);
        return;
    }
    if (pause_sel == P_RESTART && pressed_btn(Button::A)) {
        start_game();
        return;
    }
    if (pause_sel == P_QUIT && pressed_btn(Button::A)) {
        audio::play(audio::Sfx::MenuBack);
        go_title();
        return;
    }
    if (pause_sel == P_MUSIC && toggle) {
        sd.music = !sd.music;
        apply_settings();
        store_save();
        audio::play(audio::Sfx::MenuMove);
    }
    if (pause_sel == P_SOUND && toggle) {
        sd.sfx = !sd.sfx;
        apply_settings();
        store_save();
        audio::play(audio::Sfx::MenuMove);
    }
}

void finish_name_entry() {
    HighScore hs{};
    std::memcpy(hs.name, entry_name, 4);
    hs.score = game.score();
    hs.lines = static_cast<uint16_t>(game.lines());
    hs.level = static_cast<uint8_t>(game.level());
    insert_high_score(new_rank, hs);
    std::memcpy(save_data().last_name, entry_name, 4);
    store_save();
    highlight_rank = new_rank;
    audio::play(audio::Sfx::MenuSelect);
    set_play_state(PlayState::Results);
}

void update_name_entry() {
    static const char CHARS[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    const int n = static_cast<int>(sizeof(CHARS) - 1);
    char &c = entry_name[entry_cursor];
    int idx = static_cast<int>(strchr(CHARS, c) ? strchr(CHARS, c) - CHARS : 0);
    if (pressed_btn(Button::DPAD_UP)) {
        c = CHARS[(idx + 1) % n];
        audio::play(audio::Sfx::MenuMove);
    }
    if (pressed_btn(Button::DPAD_DOWN)) {
        c = CHARS[(idx + n - 1) % n];
        audio::play(audio::Sfx::MenuMove);
    }
    if (pressed_btn(Button::DPAD_LEFT) && entry_cursor > 0) {
        --entry_cursor;
        audio::play(audio::Sfx::MenuMove);
    }
    if (pressed_btn(Button::DPAD_RIGHT) && entry_cursor < 2) {
        ++entry_cursor;
        audio::play(audio::Sfx::MenuMove);
    }
    if (pressed_btn(Button::B) && entry_cursor > 0) {
        --entry_cursor;
        audio::play(audio::Sfx::MenuBack);
    }
    if (pressed_btn(Button::A) || pressed_btn(Button::MENU)) {
        if (entry_cursor < 2 && !pressed_btn(Button::MENU)) {
            ++entry_cursor;
            audio::play(audio::Sfx::MenuMove);
        } else {
            finish_name_entry();
        }
    }
}

void update_playing(uint32_t dt) {
    state_timer += dt;
    switch (play_state) {
    case PlayState::Countdown: {
        uint32_t step_before = (state_timer - dt) / 600;
        uint32_t step = state_timer / 600;
        if (step != step_before) {
            if (step < 3) {
                audio::play(audio::Sfx::Countdown);
            } else {
                audio::play(audio::Sfx::Go);
                show_callout("GO!", "", gfx::LABEL);
                callout_timer = 600;
                set_play_state(PlayState::Running);
                audio::set_tempo(game_tempo());
                audio::play_song(audio::Song::Game);
            }
        }
        break;
    }
    case PlayState::Running:
        update_running(dt);
        break;
    case PlayState::Paused:
        update_paused();
        break;
    case PlayState::Dying:
        if (state_timer > 1400) {
            new_rank = high_score_rank(game.score());
            if (new_rank >= 0) {
                std::memcpy(entry_name, save_data().last_name, 4);
                entry_cursor = 0;
                audio::play(audio::Sfx::HighScore);
                set_play_state(PlayState::NameEntry);
            } else {
                set_play_state(PlayState::Results);
            }
        }
        break;
    case PlayState::NameEntry:
        update_name_entry();
        break;
    case PlayState::Results:
        if (state_timer > 300) {
            if (pressed_btn(Button::A)) {
                audio::play(audio::Sfx::MenuSelect);
                start_game();
            } else if (pressed_btn(Button::B) || pressed_btn(Button::MENU)) {
                audio::play(audio::Sfx::MenuBack);
                go_title();
            } else if (pressed_btn(Button::Y) || pressed_btn(Button::X)) {
                audio::play(audio::Sfx::MenuSelect);
                scene = Scene::HighScores;
                anim = 0;
                audio::play_song(audio::Song::Title);
            }
        }
        break;
    }
}

// --- Rendering -------------------------------------------------------------------

void draw_background() {
    screen.pen = gfx::BG;
    screen.clear();
    // slow diagonal drift of faint dots
    screen.pen = Pen(30, 22, 52);
    int off = (anim / 120) % 8;
    for (int y = -8; y < 128; y += 8)
        for (int x = -8; x < 128; x += 8)
            screen.pixel(Point(x + off + ((y / 8) & 1) * 4, y + off));
}

void draw_menu_row(int y, bool selected, const char *label, const std::string &value) {
    if (selected) {
        screen.pen = Pen(140, 25, 35, 200);
        screen.rectangle(Rect(14, y - 1, 100, 9));
        screen.pen = gfx::CHILI_RED;
        screen.h_span(Point(14, y + 7), 100);
        if ((anim / 300) % 2 == 0) {
            screen.pen = gfx::LABEL;
            screen.text(">", minimal_font, Point(16, y));
        }
    }
    screen.pen = selected ? gfx::TEXT : gfx::TEXT_DIM;
    screen.text(label, minimal_font, Point(23, y));
    if (!value.empty()) {
        screen.pen = selected ? gfx::LABEL : gfx::TEXT_DIM;
        screen.text(value, minimal_font, Point(110, y), true, TextAlign::top_right);
    }
}

void draw_logo(int y) {
    // "CHILI" with the pepper mascot
    const int cw = gfx::big_text_width("CHILI", 2);
    const int x0 = (128 - (cw + 14)) / 2;
    gfx::chili(x0, y);
    gfx::big_text("CHILI", Point(x0 + 14, y), 2, gfx::CHILI_RED);

    // "TETRIS" in falling blocks
    static const TetrominoType colors[] = {TetrominoType::Z, TetrominoType::L, TetrominoType::O,
                                           TetrominoType::S, TetrominoType::I, TetrominoType::T};
    const char *word = "TETRIS";
    const int scale = 4;
    const int w = gfx::big_text_width(word, scale);
    int x = (128 - w) / 2;
    for (int i = 0; i < 6; ++i) {
        int lt = static_cast<int>(anim) - i * 90;
        if (lt > 0) {
            float t = lt >= 350 ? 1.0f : lt / 350.0f;
            int yoff = static_cast<int>(-50 * (1 - t) * (1 - t));
            char s[2] = {word[i], 0};
            gfx::big_text(s, Point(x, y + 13 + yoff), scale, get_tetromino_color(colors[i]), true);
        }
        x += 4 * scale;
    }
}

void draw_title() {
    draw_background();

    for (auto &d : deco) {
        auto cells = get_tetromino(d.type, d.rot);
        Pen c = get_tetromino_color(d.type);
        c.a = 45;
        for (auto &p : cells)
            gfx::block(static_cast<int>(d.x) + p.x * 5, static_cast<int>(d.y) + p.y * 5, 5, c);
    }

    draw_logo(3);

    // keep the menu readable over the falling pieces
    screen.pen = Pen(10, 6, 22, 190);
    screen.rectangle(Rect(12, 41, 104, 74));

    const auto &sd = save_data();
    const char *labels[T_COUNT] = {"PLAY",        "START LEVEL", "MUSIC",       "SOUND",
                                   "GHOST PIECE", "HIGH SCORES", "HOW TO PLAY", "CREDITS"};
    for (int i = 0; i < T_COUNT; ++i) {
        std::string value;
        switch (i) {
        case T_LEVEL: value = std::to_string(sd.start_level); break;
        case T_MUSIC: value = sd.music ? "ON" : "OFF"; break;
        case T_SOUND: value = sd.sfx ? "ON" : "OFF"; break;
        case T_GHOST: value = sd.ghost ? "ON" : "OFF"; break;
        }
        if (i == T_LEVEL && title_sel == T_LEVEL)
            value = "< " + value + " >";
        draw_menu_row(43 + i * 9, title_sel == i, labels[i], value);
    }

    screen.pen = gfx::TEXT_DIM;
    screen.text(std::string("HI ") + std::to_string(sd.scores[0].score), minimal_font, Point(3, 119));
    screen.text(VERSION, minimal_font, Point(125, 119), true, TextAlign::top_right);
}

void draw_board(int ox, int oy) {
    // frame
    screen.pen = gfx::FRAME;
    screen.rectangle(Rect(ox - 3, oy - 3, FW + 6, FH + 6));
    screen.pen = gfx::FRAME_LIGHT;
    screen.h_span(Point(ox - 3, oy - 3), FW + 6);
    screen.v_span(Point(ox - 3, oy - 3), FH + 6);
    screen.pen = Pen(40, 30, 70);
    screen.rectangle(Rect(ox - 1, oy - 1, FW + 2, FH + 2));
    screen.pen = gfx::FIELD_BG;
    screen.rectangle(Rect(ox, oy, FW, FH));

    // grid dots
    screen.pen = gfx::FIELD_DOT;
    for (int y = 1; y < Tetris::VISIBLE; ++y)
        for (int x = 1; x < Tetris::WIDTH; ++x)
            screen.pixel(Point(ox + x * CELL - 1, oy + y * CELL - 1));

    // danger glow when the stack gets high
    int top = Tetris::VISIBLE;
    for (int y = Tetris::HIDDEN; y < Tetris::HEIGHT && top == Tetris::VISIBLE; ++y)
        for (int x = 0; x < Tetris::WIDTH; ++x)
            if (game.cell(x, y) != TetrominoType::COUNT) {
                top = y - Tetris::HIDDEN;
                break;
            }
    if (top < 5 && play_state == PlayState::Running) {
        int a = 40 + static_cast<int>(30 * sinf(anim / 120.0f));
        screen.pen = Pen(255, 30, 30, a);
        screen.rectangle(Rect(ox, oy, FW, 12));
    }

    // locked cells
    const bool clearing = game.phase() == Tetris::Phase::Clearing;
    const float cp = clearing ? game.clearing_progress() : 0.0f;
    for (int y = Tetris::HIDDEN; y < Tetris::HEIGHT; ++y) {
        bool is_clearing = false;
        if (clearing)
            for (int i = 0; i < game.clearing_count(); ++i)
                if (game.clearing_row(i) == y)
                    is_clearing = true;
        const int sy = oy + (y - Tetris::HIDDEN) * CELL;
        for (int x = 0; x < Tetris::WIDTH; ++x) {
            auto t = game.cell(x, y);
            if (t == TetrominoType::COUNT)
                continue;
            Pen c = get_tetromino_color(t);
            if (play_state == PlayState::Dying || play_state == PlayState::NameEntry ||
                play_state == PlayState::Results)
                c = gfx::darken(c, 120);
            if (is_clearing) {
                if (cp < 0.45f) {
                    // flash
                    c = ((static_cast<int>(cp * 20)) % 2 == 0) ? Pen(255, 255, 255) : gfx::lighten(c, 160);
                } else {
                    // collapse from the centre outwards
                    float gone = (cp - 0.45f) / 0.55f * 5.5f;
                    if (fabsf(x - 4.5f) < gone)
                        continue;
                    c = Pen(255, 255, 255);
                }
            }
            gfx::block(ox + x * CELL, sy, CELL, c);
        }
    }

    // lock flash
    if (lock_flash_timer > 0) {
        screen.pen = Pen(255, 255, 255, lock_flash_timer * 150 / 120);
        for (auto &p : game.last_locked())
            if (p.y >= Tetris::HIDDEN)
                screen.rectangle(Rect(ox + p.x * CELL, oy + (p.y - Tetris::HIDDEN) * CELL, CELL - 1, CELL - 1));
    }

    // hard drop trail
    if (trail_timer > 0) {
        const auto &tr = game.last_trail();
        for (auto &p : tr.to) {
            int y1 = p.y - tr.distance;
            for (int y = y1; y < p.y; ++y) {
                if (y < Tetris::HIDDEN)
                    continue;
                int fade = (y - y1 + 1) * 120 / (tr.distance + 1);
                screen.pen = Pen(255, 255, 255, fade * trail_timer / 160);
                screen.rectangle(Rect(ox + p.x * CELL + 1, oy + (y - Tetris::HIDDEN) * CELL, CELL - 3, CELL));
            }
        }
    }

    // ghost + active piece
    const bool show_piece = (play_state == PlayState::Running || play_state == PlayState::Countdown) &&
                            game.phase() == Tetris::Phase::Falling;
    if (show_piece) {
        if (save_data().ghost) {
            Tetrimino g = game.ghost();
            for (auto &p : g.blocks())
                if (p.y >= Tetris::HIDDEN)
                    gfx::ghost_block(ox + p.x * CELL, oy + (p.y - Tetris::HIDDEN) * CELL, CELL, g.color());
        }
        const auto &pc = game.piece();
        Pen c = pc.color();
        float lp = game.lock_progress();
        if (lp > 0)
            c = gfx::lighten(c, static_cast<int>(lp * 140 * (0.6f + 0.4f * sinf(anim / 40.0f))));
        for (auto &p : pc.blocks())
            if (p.y >= Tetris::HIDDEN)
                gfx::block(ox + p.x * CELL, oy + (p.y - Tetris::HIDDEN) * CELL, CELL, c);
    }

    // game over fill
    if (play_state == PlayState::Dying || play_state == PlayState::NameEntry || play_state == PlayState::Results) {
        int rows = std::min<int>(Tetris::VISIBLE, play_state == PlayState::Dying ? state_timer * Tetris::VISIBLE / 1000 : 99);
        for (int r = 0; r < rows; ++r) {
            int y = oy + FH - (r + 1) * CELL;
            for (int x = 0; x < Tetris::WIDTH; ++x)
                gfx::block(ox + x * CELL, y, CELL, Pen(70, 60, 90, 170));
        }
    }
}

void draw_side_panel() {
    // NEXT
    gfx::panel(Rect(PANEL_X, 1, PANEL_W, 42), "NEXT");
    gfx::piece_preview(game.next(0), Rect(PANEL_X + 2, 9, PANEL_W - 4, 16), 5);
    gfx::piece_preview(game.next(1), Rect(PANEL_X + 2, 27, PANEL_W / 2 - 2, 13), 3);
    gfx::piece_preview(game.next(2), Rect(PANEL_X + PANEL_W / 2, 27, PANEL_W / 2 - 2, 13), 3);

    // HOLD
    gfx::panel(Rect(PANEL_X, 45, PANEL_W, 25), "HOLD");
    if (game.has_hold())
        gfx::piece_preview(game.hold_piece(), Rect(PANEL_X + 2, 52, PANEL_W - 4, 16), 4, !game.can_hold());

    // stats
    gfx::panel(Rect(PANEL_X, 72, PANEL_W, 55));
    screen.pen = gfx::LABEL;
    screen.text("SCORE", minimal_font, Point(PANEL_X + 3, 74));
    screen.pen = gfx::TEXT;
    screen.text(std::to_string(game.score()), minimal_font, Point(PANEL_X + PANEL_W - 3, 82), true,
                TextAlign::top_right);

    screen.pen = gfx::LABEL;
    screen.text("LVL", minimal_font, Point(PANEL_X + 3, 92));
    screen.text("LINES", minimal_font, Point(PANEL_X + PANEL_W - 3, 92), true, TextAlign::top_right);
    screen.pen = gfx::TEXT;
    screen.text(std::to_string(game.level()), minimal_font, Point(PANEL_X + 3, 100));
    screen.text(std::to_string(game.lines()), minimal_font, Point(PANEL_X + PANEL_W - 3, 100), true,
                TextAlign::top_right);

    // progress to next level
    const int bar_w = PANEL_W - 6;
    screen.pen = Pen(50, 40, 80);
    screen.rectangle(Rect(PANEL_X + 3, 109, bar_w, 3));
    screen.pen = gfx::CHILI_RED;
    screen.rectangle(Rect(PANEL_X + 3, 109, bar_w * (game.lines() % 10) / 10, 3));

    // best score
    uint32_t best = std::max(save_data().scores[0].score, game.score());
    gfx::chili(PANEL_X + 3, 115);
    screen.pen = gfx::TEXT_DIM;
    screen.text(std::to_string(best), minimal_font, Point(PANEL_X + PANEL_W - 3, 117), true, TextAlign::top_right);
}

void draw_callouts(int ox, int oy) {
    const int cx = ox + FW / 2;
    if (callout_timer > 0) {
        int t = 1300 - static_cast<int>(callout_timer);
        int scale = callout_big.size() > 5 ? 2 : 3;
        int pop = t < 120 ? (120 - t) / 30 : 0; // small pop-in bounce
        int w = gfx::big_text_width(callout_big, scale);
        Pen c = callout_color;
        if (callout_timer < 300)
            c.a = static_cast<uint8_t>(255 * callout_timer / 300);
        gfx::big_text(callout_big, Point(cx - w / 2, oy + 34 - pop), scale, c);
        if (!callout_small.empty())
            gfx::text_outlined(callout_small, Point(cx, oy + 36 + 5 * scale), gfx::TEXT, TextAlign::top_center);
    }
    if (level_banner_timer > 0) {
        int t = 1500 - static_cast<int>(level_banner_timer);
        int slide = t < 200 ? (200 - t) / 4 : 0;
        screen.pen = Pen(0, 0, 0, 160);
        screen.rectangle(Rect(ox, oy + 70, FW, 18));
        std::string s = "LEVEL " + std::to_string(game.level());
        int w = gfx::big_text_width(s, 2);
        gfx::big_text(s, Point(cx - w / 2 + slide, oy + 74), 2, (anim / 100) % 2 ? gfx::LABEL : gfx::TEXT);
    }
}

void draw_countdown(int ox, int oy) {
    int step = state_timer / 600;
    if (step > 2)
        return;
    std::string s = std::to_string(3 - step);
    int t = state_timer % 600;
    int scale = t < 100 ? 6 : 5;
    int w = gfx::big_text_width(s, scale);
    gfx::big_text(s, Point(ox + FW / 2 - w / 2, oy + 45), scale, gfx::LABEL);
    gfx::text_outlined("GET READY", Point(ox + FW / 2, oy + 80), gfx::TEXT, TextAlign::top_center);
}

void draw_pause(int ox, int oy) {
    screen.pen = gfx::FIELD_BG;
    screen.rectangle(Rect(ox, oy, FW, FH));
    gfx::chili(ox + FW / 2 - 5, oy + 14);
    int w = gfx::big_text_width("PAUSED", 2);
    gfx::big_text("PAUSED", Point(ox + FW / 2 - w / 2, oy + 28), 2, gfx::LABEL);

    const auto &sd = save_data();
    const char *labels[P_COUNT] = {"RESUME", "RESTART", "MUSIC", "SOUND", "QUIT"};
    for (int i = 0; i < P_COUNT; ++i) {
        int y = oy + 50 + i * 11;
        std::string s = labels[i];
        if (i == P_MUSIC)
            s += sd.music ? " ON" : " OFF";
        if (i == P_SOUND)
            s += sd.sfx ? " ON" : " OFF";
        if (i == pause_sel) {
            screen.pen = Pen(140, 25, 35, 220);
            screen.rectangle(Rect(ox + 2, y - 2, FW - 4, 10));
        }
        screen.pen = i == pause_sel ? gfx::TEXT : gfx::TEXT_DIM;
        screen.text(s, minimal_font, Point(ox + FW / 2, y), true, TextAlign::top_center);
    }
}

void draw_game_over_panel() {
    const Rect r(10, 22, 108, 86);
    screen.pen = Pen(0, 0, 0, 150);
    screen.rectangle(Rect(0, 0, 128, 128));
    gfx::panel(r);
    screen.pen = gfx::CHILI_RED;
    screen.h_span(Point(r.x + 1, r.y + 1), r.w - 2);

    int w = gfx::big_text_width("GAME OVER", 2);
    gfx::big_text("GAME OVER", Point(64 - w / 2, r.y + 5), 2, gfx::CHILI_RED);

    if (play_state == PlayState::NameEntry) {
        bool blink = (anim / 250) % 2;
        gfx::text_outlined("NEW HIGH SCORE!", Point(64, r.y + 19), blink ? gfx::LABEL : gfx::TEXT, TextAlign::top_center);
        screen.pen = gfx::TEXT;
        screen.text(std::to_string(game.score()), minimal_font, Point(64, r.y + 28), true, TextAlign::top_center);
        screen.pen = gfx::TEXT_DIM;
        screen.text("ENTER YOUR NAME", minimal_font, Point(64, r.y + 38), true, TextAlign::top_center);
        for (int i = 0; i < 3; ++i) {
            int x = 64 - 22 + i * 16;
            char s[2] = {entry_name[i], 0};
            if (i == entry_cursor) {
                screen.pen = Pen(140, 25, 35);
                screen.rectangle(Rect(x - 3, r.y + 47, 15, 17));
                screen.pen = gfx::LABEL;
                screen.text("^", minimal_font, Point(x + 5, r.y + 44), true, TextAlign::top_center);
            }
            gfx::big_text(s, Point(x, r.y + 49), 3, gfx::TEXT);
        }
        screen.pen = gfx::TEXT_DIM;
        screen.text("UP/DOWN LETTER  A OK", minimal_font, Point(64, r.y + 72), true, TextAlign::top_center);
        return;
    }

    const auto &sd = save_data();
    auto row = [&](int y, const char *label, const std::string &value, Pen vc) {
        screen.pen = gfx::LABEL;
        screen.text(label, minimal_font, Point(r.x + 10, y));
        screen.pen = vc;
        screen.text(value, minimal_font, Point(r.x + r.w - 10, y), true, TextAlign::top_right);
    };
    row(r.y + 20, "SCORE", std::to_string(game.score()), gfx::TEXT);
    row(r.y + 29, "LINES", std::to_string(game.lines()), gfx::TEXT);
    row(r.y + 38, "LEVEL", std::to_string(game.level()), gfx::TEXT);
    row(r.y + 47, "BEST", std::to_string(sd.scores[0].score), gfx::TEXT_DIM);
    if (highlight_rank >= 0) {
        bool blink = (anim / 250) % 2;
        std::string s = highlight_rank == 0 ? "NEW BEST!" : "RANK #" + std::to_string(highlight_rank + 1);
        gfx::text_outlined(s, Point(64, r.y + 57), blink ? gfx::LABEL : gfx::TEXT, TextAlign::top_center);
    }
    screen.pen = gfx::TEXT;
    screen.text("A: PLAY AGAIN", minimal_font, Point(64, r.y + 66), true, TextAlign::top_center);
    screen.pen = gfx::TEXT_DIM;
    screen.text("B: MENU   Y: SCORES", minimal_font, Point(64, r.y + 76), true, TextAlign::top_center);
}

void draw_playing() {
    draw_background();

    int sx = 0, sy = 0;
    if (shake_timer > 0 && shake_strength > 0) {
        sx = rand() % (2 * shake_strength + 1) - shake_strength;
        sy = rand() % (2 * shake_strength + 1) - shake_strength;
    }
    const int ox = FX + sx, oy = FY + sy;

    draw_board(ox, oy);
    draw_side_panel();
    draw_particles();

    switch (play_state) {
    case PlayState::Countdown: draw_countdown(ox, oy); break;
    case PlayState::Running: draw_callouts(ox, oy); break;
    case PlayState::Paused: draw_pause(ox, oy); break;
    case PlayState::Dying: break;
    case PlayState::NameEntry:
    case PlayState::Results: draw_game_over_panel(); break;
    }
}

void draw_header(const char *title) {
    draw_background();
    int w = gfx::big_text_width(title, 2);
    gfx::big_text(title, Point(64 - w / 2, 5), 2, gfx::LABEL);
    screen.pen = gfx::CHILI_RED;
    screen.h_span(Point(10, 18), 108);
}

void draw_high_scores() {
    draw_header("HIGH SCORES");
    gfx::panel(Rect(6, 23, 116, 86));
    screen.pen = gfx::TEXT_DIM;
    screen.text("NAME", minimal_font, Point(22, 27));
    screen.text("SCORE", minimal_font, Point(84, 27), true, TextAlign::top_right);
    screen.text("LV", minimal_font, Point(112, 27), true, TextAlign::top_right);
    const auto &sd = save_data();
    for (int i = 0; i < HIGH_SCORE_COUNT; ++i) {
        const auto &s = sd.scores[i];
        int y = 39 + i * 14;
        bool hl = i == highlight_rank && (anim / 250) % 2 == 0;
        if (i == highlight_rank) {
            screen.pen = Pen(140, 25, 35, 200);
            screen.rectangle(Rect(8, y - 3, 112, 12));
        }
        static const TetrominoType medal[] = {TetrominoType::O, TetrominoType::I, TetrominoType::L,
                                              TetrominoType::T, TetrominoType::J};
        gfx::block(10, y - 1, 6, get_tetromino_color(medal[i]));
        screen.pen = gfx::TEXT;
        screen.text(std::to_string(i + 1), minimal_font, Point(12, y - 1));
        screen.pen = hl ? gfx::LABEL : gfx::TEXT;
        screen.text(s.name, minimal_font, Point(22, y));
        screen.text(std::to_string(s.score), minimal_font, Point(84, y), true, TextAlign::top_right);
        screen.pen = gfx::TEXT_DIM;
        screen.text(std::to_string(s.level), minimal_font, Point(112, y), true, TextAlign::top_right);
    }
    screen.pen = gfx::TEXT_DIM;
    screen.text("B BACK", minimal_font, Point(64, 116), true, TextAlign::top_center);
}

void draw_help() {
    draw_header("HOW TO PLAY");
    struct Line {
        const char *key, *what;
    };
    static const Line lines[] = {
        {"LEFT/RIGHT", "MOVE"},   {"DOWN", "SOFT DROP"},   {"UP", "HARD DROP"}, {"A", "ROTATE RIGHT"},
        {"B", "ROTATE LEFT"},     {"X / Y", "HOLD PIECE"}, {"MENU", "PAUSE"},
    };
    int y = 25;
    for (auto &l : lines) {
        screen.pen = gfx::LABEL;
        screen.text(l.key, minimal_font, Point(8, y));
        screen.pen = gfx::TEXT;
        screen.text(l.what, minimal_font, Point(120, y), true, TextAlign::top_right);
        y += 10;
    }
    screen.pen = gfx::TEXT_DIM;
    screen.text("Clear lines to score.\nEvery 10 lines: level up!", minimal_font, Point(64, 96), true,
                TextAlign::top_center);
}

void draw_credits() {
    draw_header("CREDITS");
    gfx::chili(59, 24);
    struct Line {
        const char *text;
        bool label;
    };
    static const Line lines[] = {
        {"GAME", true},      {"ChiliChip / TeriyakiGod", false},
        {"MUSIC", true},     {"Chili Habanera (original)", false},
        {"Korobeiniki (trad.)", false},
        {"MADE WITH", true}, {"32blit SDK", false},
    };
    int y = 37;
    for (auto &l : lines) {
        screen.pen = l.label ? gfx::LABEL : gfx::TEXT;
        screen.text(l.text, minimal_font, Point(64, y), true, TextAlign::top_center);
        y += l.label ? 8 : 10;
    }
    screen.pen = gfx::TEXT_DIM;
    screen.text(std::string("chilichip.eu  ") + VERSION, minimal_font, Point(64, 117), true, TextAlign::top_center);
}

} // namespace

// --- 32blit entry points ---------------------------------------------------------

void init() {
    set_screen_mode(ScreenMode::hires);
    srand(static_cast<unsigned int>(time(nullptr)) ^ now());

    load_save();
    audio::init();
    apply_settings();

    for (auto &d : deco)
        reset_deco(d, true);

    game.start(1);
    go_title();
}

void update(uint32_t time) {
    if (last_time == 0)
        last_time = time;
    uint32_t dt = time - last_time;
    last_time = time;
    if (dt > 50)
        dt = 50;

    anim += dt;
    audio::update(dt);
    update_particles(dt);

    auto tick = [dt](uint32_t &t) { t = t > dt ? t - dt : 0; };
    tick(shake_timer);
    tick(trail_timer);
    tick(lock_flash_timer);
    tick(callout_timer);
    tick(level_banner_timer);

    switch (scene) {
    case Scene::Title: update_title(); break;
    case Scene::Playing: update_playing(dt); break;
    case Scene::HighScores:
    case Scene::Help:
    case Scene::Credits: update_info_screen(); break;
    }
}

void render(uint32_t time) {
    screen.alpha = 255;
    screen.mask = nullptr;

    switch (scene) {
    case Scene::Title: draw_title(); break;
    case Scene::Playing: draw_playing(); break;
    case Scene::HighScores: draw_high_scores(); break;
    case Scene::Help: draw_help(); break;
    case Scene::Credits: draw_credits(); break;
    }
}
