# chip8

A CHIP-8 emulator written in C, using SDL2 for graphics and input.

<p align="center">
  <img src="assets/chip8-dragon.png" alt="chip8 dragon" width="320">
</p>

## Build

Requires `gcc`, `make` and SDL2 (`sdl2-config` must be on your `PATH`).

```sh
make
```

## Usage

```sh
./chip8 <path_to_rom> [--no-log]
```

Some test ROMs are provided in the `roms/` directory:

```sh
./chip8 roms/pong.ch8
```

Pass `--no-log` to disable the per-instruction logging.

## Controls

The original CHIP-8 hex keypad is mapped to the left side of the keyboard:

```
CHIP-8        Keyboard
1 2 3 C       1 2 3 4
4 5 6 D   ->  Q W E R
7 8 9 E       A S D F
A 0 B F       Z X C V
```

Press `Esc` to quit.

## Reference

[CHIP-8 Design Specification](https://www.cs.columbia.edu/~sedwards/classes/2016/4840-spring/designs/Chip8.pdf)
