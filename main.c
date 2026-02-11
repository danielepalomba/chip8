#include "chip8.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32

#define SCALE 15

int main(int argc, char **argv) {

  if (argc < 2 || argc > 3) {
    printf("USAGE: ./chip8 <path_to_rom> [--no-log]\n");
    return 1;
  }

  char *rom_path = NULL;
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--no-log") == 0) {
      enable_logging = 0;
    } else {
      rom_path = argv[i];
    }
  }

  if (rom_path == NULL) {
    fprintf(stderr, "Error: ROM path not specified\n");
    printf("USAGE: ./chip8 <path_to_rom> [--no-log]\n");
    return 1;
  }

  srand(time(NULL));

  Chip8 *chip = init_chip8();
  if (!load_rom(rom_path, chip)) {
    fprintf(stderr, "Could not load the ROM.\n");
    return 1;
  }

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("SDL Error: %s\n", SDL_GetError());
    return 1;
  }

  SDL_Window *window = SDL_CreateWindow(
      "Chip-8 Emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      SCREEN_WIDTH * SCALE, SCREEN_HEIGHT * SCALE, SDL_WINDOW_SHOWN);

  SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);

  SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                           SDL_TEXTUREACCESS_STREAMING,
                                           SCREEN_WIDTH, SCREEN_HEIGHT);

  uint32_t pixels[SCREEN_WIDTH * SCREEN_HEIGHT];

  int running = 1;
  SDL_Event event;

  while (running) {

    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        running = 0;
      }
      if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_ESCAPE) {
          running = 0;
        }
      }

      /*
       * Keyboard Mapping
       *
       * 1 2 3 C     1 2 3 4
       * 4 5 6 D  -> Q W E R
       * 7 8 9 E     A S D F
       * A 0 B F     Z X C V
       *
       */

      switch (event.type) {
      case SDL_QUIT:
        break;

      case SDL_KEYDOWN:
        switch (event.key.keysym.sym) {
        case SDLK_1:
          chip->keypad[0x1] = 1;
          break;
        case SDLK_2:
          chip->keypad[0x2] = 1;
          break;
        case SDLK_3:
          chip->keypad[0x3] = 1;
          break;
        case SDLK_4:
          chip->keypad[0xC] = 1;
          break;

        case SDLK_q:
          chip->keypad[0x4] = 1;
          break;
        case SDLK_w:
          chip->keypad[0x5] = 1;
          break;
        case SDLK_e:
          chip->keypad[0x6] = 1;
          break;
        case SDLK_r:
          chip->keypad[0xD] = 1;
          break;

        case SDLK_a:
          chip->keypad[0x7] = 1;
          break;
        case SDLK_s:
          chip->keypad[0x8] = 1;
          break;
        case SDLK_d:
          chip->keypad[0x9] = 1;
          break;
        case SDLK_f:
          chip->keypad[0xE] = 1;
          break;

        case SDLK_z:
          chip->keypad[0xA] = 1;
          break;
        case SDLK_x:
          chip->keypad[0x0] = 1;
          break;
        case SDLK_c:
          chip->keypad[0xB] = 1;
          break;
        case SDLK_v:
          chip->keypad[0xF] = 1;
          break;
        }
        break;

      case SDL_KEYUP:
        switch (event.key.keysym.sym) {
        case SDLK_1:
          chip->keypad[0x1] = 0;
          break;
        case SDLK_2:
          chip->keypad[0x2] = 0;
          break;
        case SDLK_3:
          chip->keypad[0x3] = 0;
          break;
        case SDLK_4:
          chip->keypad[0xC] = 0;
          break;

        case SDLK_q:
          chip->keypad[0x4] = 0;
          break;
        case SDLK_w:
          chip->keypad[0x5] = 0;
          break;
        case SDLK_e:
          chip->keypad[0x6] = 0;
          break;
        case SDLK_r:
          chip->keypad[0xD] = 0;
          break;

        case SDLK_a:
          chip->keypad[0x7] = 0;
          break;
        case SDLK_s:
          chip->keypad[0x8] = 0;
          break;
        case SDLK_d:
          chip->keypad[0x9] = 0;
          break;
        case SDLK_f:
          chip->keypad[0xE] = 0;
          break;

        case SDLK_z:
          chip->keypad[0xA] = 0;
          break;
        case SDLK_x:
          chip->keypad[0x0] = 0;
          break;
        case SDLK_c:
          chip->keypad[0xB] = 0;
          break;
        case SDLK_v:
          chip->keypad[0xF] = 0;
          break;
        }
        break;
      }
    }

    for (int i = 0; i < 10; i++) {
      emulate(chip);
    }

    if (chip->delay_timer > 0) {
      chip->delay_timer--;
    }
    if (chip->sound_timer > 0) {
      chip->sound_timer--;
    }

    if (chip->draw_flag) {
      chip->draw_flag = 0;

      for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; ++i) {
        uint8_t pixel = chip->gfx[i];
        pixels[i] = (pixel == 1) ? 0xFFFFFFFF : 0x00000000;
      }

      SDL_UpdateTexture(texture, NULL, pixels, SCREEN_WIDTH * sizeof(uint32_t));
    }

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);

    SDL_Delay(16); // 60 FPS
  }

  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  free(chip);

  return 0;
}
