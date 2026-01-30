#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include "chip8.h"

#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32

#define SCALE 15

int main(int argc, char **argv) {
      
    if (argc != 2) {
        printf("USAGE: ./chip8 <path_to_rom>\n");
        return 1;
    }

    Chip8 *chip = init_chip8();
    if (!load_rom(argv[1], chip)) {
        fprintf(stderr, "Could not load the ROM.\n");
        return 1;
    }

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "Chip-8 Emulator",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH * SCALE, SCREEN_HEIGHT * SCALE,
        SDL_WINDOW_SHOWN
    );

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
    
    SDL_Texture *texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA8888, 
        SDL_TEXTUREACCESS_STREAMING,
        SCREEN_WIDTH, SCREEN_HEIGHT
    );

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
        }

        for (int i = 0; i < 10; i++) {
            emulate(chip);
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
