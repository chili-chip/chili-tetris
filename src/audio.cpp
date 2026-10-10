#include "audio.hpp"

#include "32blit.hpp"

#include <cmath>
#include <string>
#include <vector>

using namespace blit;

namespace audio {

namespace {

// ---------------------------------------------------------------------------
// Music
// ---------------------------------------------------------------------------

// Drum "notes" share the note field with pitches; real pitches start at 12.
enum : uint8_t { REST = 0, KICK = 1, SNARE = 2, HAT = 3 };

struct Event {
    uint8_t note; // MIDI note number, REST, or a drum id
    uint8_t len;  // length in sixteenth notes
};

using Track = std::vector<Event>;

struct SongData {
    Track melody, bass, drums;
    uint16_t sixteenth_ms; // tempo
};

// Parses "e5:4 g#4:2 r:4 k:2 s:2 h:2". Notes are letter + optional '#' +
// octave; r is a rest, k/s/h are kick, snare and hi-hat.
Track parse(const char *text) {
    Track out;
    const char *p = text;
    while (*p) {
        while (*p == ' ' || *p == '\n' || *p == '|')
            ++p;
        if (!*p)
            break;

        uint8_t note = REST;
        char c = *p++;
        switch (c) {
        case 'r': note = REST; break;
        case 'k': note = KICK; break;
        case 's': note = SNARE; break;
        case 'h': note = HAT; break;
        default: {
            static const int semis[] = {9, 11, 0, 2, 4, 5, 7}; // a b c d e f g
            int semi = semis[c - 'a'];
            if (*p == '#') {
                ++semi;
                ++p;
            }
            int octave = *p++ - '0';
            note = static_cast<uint8_t>(12 * (octave + 1) + semi);
        }
        }

        int len = 0;
        if (*p == ':') {
            ++p;
            while (*p >= '0' && *p <= '9')
                len = len * 10 + (*p++ - '0');
        }
        out.push_back({note, static_cast<uint8_t>(len ? len : 1)});
    }
    return out;
}

void append(Track &dst, const Track &src) { dst.insert(dst.end(), src.begin(), src.end()); }

void repeat(Track &dst, const Track &bar, int times) {
    for (int i = 0; i < times; ++i)
        append(dst, bar);
}

uint8_t midi(const char *name) { return parse(name)[0].note; }

// Octave-jumping eighth-note bass, one bar per root.
Track octave_bass(std::initializer_list<const char *> roots) {
    Track out;
    for (auto r : roots) {
        uint8_t n = midi(r);
        for (int i = 0; i < 4; ++i) {
            out.push_back({n, 2});
            out.push_back({static_cast<uint8_t>(n + 12), 2});
        }
    }
    return out;
}

// Habanera bass (dotted eighth, sixteenth, eighth, eighth), one bar per root.
Track habanera_bass(std::initializer_list<const char *> roots) {
    Track out;
    for (auto r : roots) {
        uint8_t n = midi(r);
        for (int i = 0; i < 2; ++i) {
            out.push_back({n, 3});
            out.push_back({n, 1});
            out.push_back({static_cast<uint8_t>(n + 7), 2});
            out.push_back({static_cast<uint8_t>(n + 12), 2});
        }
    }
    return out;
}

SongData make_game_song() {
    // Korobeiniki: 19th-century Russian folk song, public domain.
    // This arrangement (melody + octave bass + drums) was written for chili-tetris.
    const Track a = parse(
        "e5:4 b4:2 c5:2 d5:4 c5:2 b4:2 | a4:4 a4:2 c5:2 e5:4 d5:2 c5:2 |"
        "b4:6 c5:2 d5:4 e5:4 | c5:4 a4:4 a4:4 r:4 |"
        "r:2 d5:4 f5:2 a5:4 g5:2 f5:2 | e5:6 c5:2 e5:4 d5:2 c5:2 |"
        "b4:4 b4:2 c5:2 d5:4 e5:4 | c5:4 a4:4 a4:4 r:4");
    const Track b = parse(
        "e5:8 c5:8 | d5:8 b4:8 | c5:8 a4:8 | g#4:8 b4:8 |"
        "e5:8 c5:8 | d5:8 b4:8 | c5:4 e5:4 a5:8 | g#5:16");

    SongData s;
    append(s.melody, a);
    append(s.melody, a);
    append(s.melody, b);

    const Track bass_a = octave_bass({"e3", "a2", "e3", "a2", "d3", "a2", "e3", "a2"});
    const Track bass_b = octave_bass({"a2", "e3", "a2", "e3", "a2", "e3", "a2", "e3"});
    append(s.bass, bass_a);
    append(s.bass, bass_a);
    append(s.bass, bass_b);

    repeat(s.drums, parse("k:2 h:2 s:2 h:2 k:2 h:2 s:2 h:2"), 23);
    append(s.drums, parse("k:2 h:2 s:2 h:2 s:1 s:1 s:2 s:2 s:2"));

    s.sixteenth_ms = 100; // 150 bpm
    return s;
}

SongData make_title_song() {
    // "Chili Habanera" - original theme composed for chili-tetris (D minor).
    SongData s;
    s.melody = parse(
        "d5:3 f5:1 a5:2 a5:2 g5:2 f5:2 e5:2 f5:2 | d5:6 a4:2 d5:2 f5:2 e5:4 |"
        "g5:3 a5:1 a#5:2 a5:2 g5:2 f5:2 e5:2 d5:2 | e5:4 c#5:4 a4:4 r:4 |"
        "d5:3 f5:1 a5:2 d6:2 c6:2 a5:2 f5:2 a5:2 | a#5:4 a5:2 g5:2 f5:4 d5:4 |"
        "e5:2 f5:2 g5:2 a5:2 g5:2 f5:2 e5:2 c#5:2 | d5:8 r:4 a4:2 c#5:2");
    s.bass = habanera_bass({"d3", "d3", "g2", "a2", "d3", "a#2", "a2", "d3"});
    repeat(s.drums, parse("k:3 h:1 s:2 h:2 k:3 h:1 s:2 h:2"), 8);
    s.sixteenth_ms = 115; // ~130 bpm
    return s;
}

struct Voice {
    const Track *track = nullptr;
    size_t index = 0;
    float remaining = 0; // ms left on the current event
    float gate = 0;      // ms left before the note is released
};

std::vector<SongData> songs; // indexed by Song - 1
Song active_song = Song::None;
Voice voices[3];
float tempo = 1.0f;
bool music_on = true;
bool sfx_on = true;

uint16_t note_freq(uint8_t note) {
    return static_cast<uint16_t>(440.0f * powf(2.0f, (note - 69) / 12.0f));
}

void start_event(int ch, const Event &e) {
    AudioChannel &c = channels[ch];
    if (e.note == REST) {
        c.trigger_release();
        return;
    }
    if (ch == 2) {
        // drums
        switch (e.note) {
        case KICK:
            c.waveforms = Waveform::SQUARE;
            c.frequency = 70;
            c.attack_ms = 1;
            c.decay_ms = 60;
            c.sustain = 0;
            c.release_ms = 10;
            break;
        case SNARE:
            c.waveforms = Waveform::NOISE;
            c.frequency = 2000;
            c.attack_ms = 1;
            c.decay_ms = 70;
            c.sustain = 0;
            c.release_ms = 10;
            break;
        default:
            c.waveforms = Waveform::NOISE;
            c.frequency = 8000;
            c.attack_ms = 1;
            c.decay_ms = 18;
            c.sustain = 0;
            c.release_ms = 5;
            break;
        }
        c.trigger_attack();
        return;
    }
    c.frequency = note_freq(e.note);
    c.trigger_attack();
}

void configure_music_channels() {
    channels[0].waveforms = Waveform::SQUARE;
    channels[0].pulse_width = 0x5000;
    channels[0].attack_ms = 4;
    channels[0].decay_ms = 120;
    channels[0].sustain = 0xa000;
    channels[0].release_ms = 40;

    channels[1].waveforms = Waveform::TRIANGLE | Waveform::SQUARE;
    channels[1].pulse_width = 0x7fff;
    channels[1].attack_ms = 2;
    channels[1].decay_ms = 80;
    channels[1].sustain = 0x8000;
    channels[1].release_ms = 20;

    channels[2].waveforms = Waveform::NOISE;
}

void apply_music_volume() {
    channels[0].volume = music_on ? 0x4800 : 0;
    channels[1].volume = music_on ? 0x3800 : 0;
    channels[2].volume = music_on ? 0x2400 : 0;
}

// ---------------------------------------------------------------------------
// Sound effects
// ---------------------------------------------------------------------------

struct Segment {
    uint16_t from_hz, to_hz, ms;
    uint8_t wave;
};

struct SfxPlayer {
    int channel;
    const Segment *segs = nullptr;
    int count = 0;
    int index = 0;
    float elapsed = 0;
};

constexpr uint8_t SQ = Waveform::SQUARE;
constexpr uint8_t TR = Waveform::TRIANGLE;
constexpr uint8_t SW = Waveform::SAW;
constexpr uint8_t NZ = Waveform::NOISE;

const Segment SFX_MENU_MOVE[] = {{988, 988, 25, SQ}};
const Segment SFX_MENU_SELECT[] = {{659, 659, 45, SQ}, {988, 988, 45, SQ}, {1319, 1319, 70, SQ}};
const Segment SFX_MENU_BACK[] = {{659, 440, 80, SQ}};
const Segment SFX_MOVE[] = {{330, 330, 12, SQ}};
const Segment SFX_ROTATE[] = {{523, 784, 35, TR | SQ}};
const Segment SFX_LOCK[] = {{180, 90, 45, SQ}};
const Segment SFX_HARD_DROP[] = {{900, 120, 60, SQ}, {400, 400, 60, NZ}};
const Segment SFX_HOLD[] = {{392, 784, 50, TR | SQ}, {784, 784, 30, TR | SQ}};
const Segment SFX_LINE[] = {{523, 523, 50, SQ}, {659, 659, 50, SQ}, {784, 784, 50, SQ}, {1047, 1047, 90, SQ}};
const Segment SFX_TETRIS[] = {{523, 523, 60, SQ}, {659, 659, 60, SQ}, {784, 784, 60, SQ}, {1047, 1047, 60, SQ},
                              {784, 784, 60, SQ}, {1047, 1047, 60, SQ}, {1319, 1319, 120, SQ}, {1568, 1568, 260, SQ}};
const Segment SFX_TSPIN[] = {{300, 1200, 120, SW}, {1200, 1200, 80, SQ}};
const Segment SFX_LEVEL_UP[] = {{440, 440, 60, SQ}, {554, 554, 60, SQ}, {659, 659, 60, SQ},
                                {880, 880, 60, SQ}, {1109, 1109, 60, SQ}, {1319, 1319, 160, SQ}};
const Segment SFX_COUNTDOWN[] = {{660, 660, 110, SQ}};
const Segment SFX_GO[] = {{1320, 1320, 260, SQ}};
const Segment SFX_PAUSE[] = {{784, 784, 50, SQ}, {523, 523, 70, SQ}};
const Segment SFX_GAME_OVER[] = {{392, 392, 220, SW}, {370, 370, 220, SW}, {349, 349, 220, SW},
                                 {330, 165, 700, SW}};
const Segment SFX_HIGH_SCORE[] = {{523, 523, 90, SQ}, {523, 523, 90, SQ}, {523, 523, 90, SQ}, {698, 698, 270, SQ},
                                  {880, 880, 90, SQ}, {784, 784, 90, SQ}, {1047, 1047, 450, SQ}};

#define SFX(arr) arr, static_cast<int>(sizeof(arr) / sizeof(arr[0]))

SfxPlayer main_fx{3};
SfxPlayer light_fx{4};

void start_segment(SfxPlayer &p) {
    AudioChannel &c = channels[p.channel];
    const Segment &s = p.segs[p.index];
    c.waveforms = s.wave;
    c.frequency = s.from_hz;
    c.pulse_width = 0x7fff;
    c.attack_ms = 1;
    c.decay_ms = 1;
    c.sustain = 0xffff;
    c.release_ms = 15;
    c.volume = sfx_on ? (p.channel == 3 ? 0x6000 : 0x3800) : 0;
    c.trigger_attack();
}

void update_sfx(SfxPlayer &p, float dt) {
    if (!p.segs)
        return;
    p.elapsed += dt;
    while (p.segs && p.elapsed >= p.segs[p.index].ms) {
        p.elapsed -= p.segs[p.index].ms;
        if (++p.index >= p.count) {
            channels[p.channel].trigger_release();
            p.segs = nullptr;
            return;
        }
        start_segment(p);
    }
    const Segment &s = p.segs[p.index];
    if (s.from_hz != s.to_hz) {
        float t = p.elapsed / s.ms;
        channels[p.channel].frequency = static_cast<uint16_t>(s.from_hz + (s.to_hz - s.from_hz) * t);
    }
}

void start_sfx(SfxPlayer &p, const Segment *segs, int count) {
    p.segs = segs;
    p.count = count;
    p.index = 0;
    p.elapsed = 0;
    start_segment(p);
}

} // namespace

void init() {
    songs.clear();
    songs.push_back(make_title_song());
    songs.push_back(make_game_song());
    configure_music_channels();
    apply_music_volume();
}

void play_song(Song song) {
    if (song == active_song)
        return;
    stop_song();
    active_song = song;
    if (song == Song::None)
        return;
    const SongData &s = songs[static_cast<int>(song) - 1];
    voices[0].track = &s.melody;
    voices[1].track = &s.bass;
    voices[2].track = &s.drums;
    for (auto &v : voices) {
        v.index = 0;
        v.remaining = 0;
        v.gate = 0;
    }
    configure_music_channels();
    apply_music_volume();
}

void stop_song() {
    active_song = Song::None;
    for (int i = 0; i < 3; ++i) {
        voices[i].track = nullptr;
        channels[i].trigger_release();
    }
}

Song current_song() { return active_song; }

void set_tempo(float multiplier) { tempo = multiplier < 0.25f ? 0.25f : multiplier; }

void update(uint32_t dt_ms) {
    const float dt = static_cast<float>(dt_ms);

    if (active_song != Song::None) {
        const SongData &s = songs[static_cast<int>(active_song) - 1];
        const float step = s.sixteenth_ms / tempo;
        for (int ch = 0; ch < 3; ++ch) {
            Voice &v = voices[ch];
            if (!v.track || v.track->empty())
                continue;
            v.remaining -= dt;
            if (v.gate > 0) {
                v.gate -= dt;
                if (v.gate <= 0 && ch != 2)
                    channels[ch].trigger_release();
            }
            while (v.remaining <= 0) {
                const Event &e = (*v.track)[v.index];
                start_event(ch, e);
                float len = e.len * step;
                v.remaining += len;
                v.gate = len * 0.85f;
                v.index = (v.index + 1) % v.track->size();
            }
        }
    }

    update_sfx(main_fx, dt);
    update_sfx(light_fx, dt);
}

void play(Sfx sfx) {
    if (!sfx_on)
        return;
    switch (sfx) {
    case Sfx::MenuMove: start_sfx(light_fx, SFX(SFX_MENU_MOVE)); break;
    case Sfx::MenuSelect: start_sfx(main_fx, SFX(SFX_MENU_SELECT)); break;
    case Sfx::MenuBack: start_sfx(main_fx, SFX(SFX_MENU_BACK)); break;
    case Sfx::Move: start_sfx(light_fx, SFX(SFX_MOVE)); break;
    case Sfx::Rotate: start_sfx(light_fx, SFX(SFX_ROTATE)); break;
    case Sfx::Lock: start_sfx(main_fx, SFX(SFX_LOCK)); break;
    case Sfx::HardDrop: start_sfx(main_fx, SFX(SFX_HARD_DROP)); break;
    case Sfx::Hold: start_sfx(light_fx, SFX(SFX_HOLD)); break;
    case Sfx::Line: start_sfx(main_fx, SFX(SFX_LINE)); break;
    case Sfx::Tetris: start_sfx(main_fx, SFX(SFX_TETRIS)); break;
    case Sfx::TSpin: start_sfx(main_fx, SFX(SFX_TSPIN)); break;
    case Sfx::LevelUp: start_sfx(light_fx, SFX(SFX_LEVEL_UP)); break;
    case Sfx::Countdown: start_sfx(main_fx, SFX(SFX_COUNTDOWN)); break;
    case Sfx::Go: start_sfx(main_fx, SFX(SFX_GO)); break;
    case Sfx::Pause: start_sfx(main_fx, SFX(SFX_PAUSE)); break;
    case Sfx::GameOver: start_sfx(main_fx, SFX(SFX_GAME_OVER)); break;
    case Sfx::HighScore: start_sfx(main_fx, SFX(SFX_HIGH_SCORE)); break;
    }
}

void set_music_enabled(bool enabled) {
    music_on = enabled;
    apply_music_volume();
}

void set_sfx_enabled(bool enabled) {
    sfx_on = enabled;
    if (!enabled) {
        main_fx.segs = nullptr;
        light_fx.segs = nullptr;
        channels[3].trigger_release();
        channels[4].trigger_release();
    }
}

} // namespace audio
