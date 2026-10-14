# Chili Tetris

[![Build](https://github.com/chili-chip/chili-tetris/actions/workflows/build.yml/badge.svg)](https://github.com/chili-chip/chili-tetris/actions/workflows/build.yml)

A falling-block puzzler with a chili twist, made for the ChiliChip **VGC Zero**
(RP2350, 128x128 screen) with the [chili-chip fork of the 32blit SDK](https://github.com/chili-chip/32blit-sdk).
It also runs on desktop (SDL) and in the browser (Emscripten).

## Features

- Title menu with start level (1-15), music, sound and ghost-piece toggles
- Top-5 high score table with 3-letter names, saved with your settings
- Modern rules: SRS rotation with wall kicks, 7-bag randomiser, hold, ghost piece,
  hard drop, lock delay, T-spins, back-to-back and combo bonuses
- Levels every 10 lines with the guideline speed curve
- Bevelled block graphics, line-clear and Tetris effects, level-up banner, pause menu
- Sound effects and a three-voice chiptune soundtrack that speeds up as you level

## Controls

| VGC Zero      | Keyboard        | Action        |
|---------------|-----------------|---------------|
| D-pad left/right | Arrows / A D | Move          |
| D-pad down    | Down / S        | Soft drop     |
| D-pad up      | Up / W          | Hard drop     |
| A             | Z               | Rotate right  |
| B             | X               | Rotate left   |
| X / Y         | C / V           | Hold piece    |
| MENU          | Esc / 2         | Pause         |

In menus: up/down to choose, A to confirm, B to go back, left/right to change a setting.

## Scoring

| Clear      | Points (x level) |
|------------|------------------|
| Single     | 100              |
| Double     | 300              |
| Triple     | 500              |
| Tetris     | 800              |
| T-spin 0/1/2/3 | 400 / 800 / 1200 / 1600 |

Back-to-back Tetrises and T-spin clears score 1.5x, consecutive clears add a
combo bonus of 50 x combo x level, soft drop scores 1 per row and hard drop 2 per row.

## Music

- **Title:** "Chili Habanera", an original theme written for this game.
- **In game:** Korobeiniki, a 19th-century Russian folk song (public domain),
  in a new melody/bass/drums arrangement written for this game.

All music and sound effects are synthesised in code (`src/audio.cpp`) and are
covered by this repository's licence.

## Code layout

| File | What it does |
|------|--------------|
| `src/game.cpp` | Scenes (title, play, pause, game over, high scores, help, credits) and rendering |
| `src/tetris.cpp` | Game rules: board, movement, rotation, lock delay, scoring, levels |
| `src/tetriminos.cpp` | Piece shapes, SRS kick tables, colours, 7-bag randomiser |
| `src/gfx.cpp` | Block tiles, panels, outlined text, chunky 3x5 font, chili mascot |
| `src/audio.cpp` | Music sequencer and sound effects |
| `src/save.cpp` | Settings and high score persistence |

## Building

Desktop:
```
mkdir build
cd build
cmake -D32BLIT_DIR=/path/to/32blit-sdk/ ..
make
./chili-tetris.elf
```

VGC Zero:
```
mkdir build.vgc
cd build.vgc
cmake .. \
  -D32BLIT_DIR=../../32blit-sdk \
  -DPICO_SDK_PATH=../../pico-sdk \
  -DPICO_BOARD=chilichip_vgc \
  -DPICO_PLATFORM=rp2350-arm-s \
  -DCMAKE_TOOLCHAIN_FILE=../../32blit-sdk/pico2.toolchain
make
```
To build an installable `.blit` for the VGC Zero loader instead of a standalone `.uf2`, add
`-DBLIT_EXECUTABLE_PICO_BLIT=1` to the vgc cmake command (or use the `vgc-blit` preset), then install it over USB with
`32blit install game.blit`. The console needs the loader flashed first; see the
[loader instructions](https://github.com/chili-chip/32blit-sdk/blob/master/docs/vgc.md#loader-and-blit-games-os-preview).

[detailed instructions for vgc](https://github.com/chili-chip/32blit-sdk/blob/master/docs/vgc.md)

### Flashing over SWD

You need Raspberry Pi's [openocd fork](https://github.com/raspberrypi/openocd)
(for RP2350 support) and a Pico Debugger or Picoprobe on the SWD pins.

```bash
cd build.vgc
openocd -f interface/cmsis-dap.cfg -f target/rp2350.cfg -c "adapter speed 5000; program "chili-tetris.elf" verify reset exit"
```
