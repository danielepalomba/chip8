#ifndef CHIP_8
#define CHIP_8

#include <stdint.h>

extern int enable_logging;

#define CHIP8_LOG(...)                                                         \
  do {                                                                         \
    if (enable_logging)                                                        \
      printf(__VA_ARGS__);                                                     \
  } while (0)

typedef struct {
  uint8_t memory[4096]; // 4KB RAM
  uint8_t V[16];        // 16 Registers
  uint16_t I;           // Index Register
  uint16_t pc;          // Program Counter

  uint8_t gfx[64 * 32]; // Display
  uint8_t draw_flag;

  uint8_t delay_timer; // Timer
  uint8_t sound_timer; // Sound timer

  uint16_t stack[16]; // Stack
  uint16_t sp;        // Stack Pointer

  uint8_t keypad[16]; // Key status
} Chip8;

Chip8 *init_chip8(void);
int load_rom(char *filename, Chip8 *chip);
void emulate(Chip8 *chip);

#endif
