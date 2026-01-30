#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "chip8.h"

#define MEMORY_SIZE 4096

uint8_t fontset[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

Chip8* init_chip8(void){

  Chip8 *chip = (Chip8*)malloc(sizeof(Chip8));
  if(!chip)
    exit(EXIT_FAILURE);

  memset(chip, 0, sizeof(Chip8));

  chip->pc = 0x200;
  chip->I = 0;
  chip->draw_flag = 0;
  chip->sp = 0;
  chip->delay_timer = 0;
  chip->sound_timer = 0;

  //load fontset into the first 80 byte (0x00 -> 0x50)
  for(int i = 0; i < 80; i++){
    chip->memory[i] = fontset[i];
  }

  return chip;
}

int load_rom(char *filename, Chip8 *chip){
  FILE *fd = fopen(filename, "rb");
  if(!fd)
    return 0;

  //Get the file size
  fseek(fd, 0, SEEK_END);
  int file_size = ftell(fd);
  fseek(fd, 0, SEEK_SET);

  printf("ROM size: %d\n", file_size);

  if(file_size > (MEMORY_SIZE - 512)){
    fprintf(stderr, "ROM size is too big!");
    fclose(fd);
    return 0;
  }

  size_t result = fread(&chip->memory[0x200], 1, file_size, fd);

  if(result != (size_t)file_size){
    fprintf(stderr, "Error while reading ROM!");
    fclose(fd);
    return 0;
  }

  fclose(fd);
  return 1;
}

// Fetch - Decode - Execute
void emulate(Chip8 *chip){
  
  // Each instruction is stored in two bytes, 
  // but each memory cell is only 1 byte, so we need to merge them.
  uint16_t opcode = (chip->memory[chip->pc] << 8) | chip->memory[chip->pc + 1];

  chip->pc += 2;
  
  // Chip8 uses the first 4 bits to determine the family 
  // to which the instruction belongs. So we use a mask.
  switch(opcode & 0xF000){
      
    case 0x0000:
        switch(opcode & 0x00FF) {
          case 0xE0: // 00E0: Clear Screen
            memset(chip->gfx, 0, sizeof(chip->gfx));
            chip->draw_flag = 1;
            printf("CLS (Clear Screen)\n");
            break;
          case 0xEE: // 00EE: Return from subroutine
            chip->sp--;
            chip->pc = chip->stack[chip->sp];
            printf("RET 0x%X\n", chip->pc);
            break;
        }
        break;

    case 0x1000: // 1NNN: Jump to NNN
      chip->pc = opcode & 0x0FFF; 
      printf("JUMP a 0x%X\n", chip->pc);
      break;
    
    case 0x2000: //CALL (call subroutine at nnn)
      {
        uint16_t nnn = (opcode & 0x0FFF);
        chip->stack[chip->sp] = chip->pc;
        chip->sp++;

        chip->pc = nnn;

        printf("Call subroutine at %d\n", nnn);
      }
      break;

    case 0x4000: //SNE 4xkk (Skip next instruction if Vx != kk)
      {
        uint8_t kk = (opcode & 0x00FF);
        uint8_t x = (opcode & 0x0F00) >> 8;
        if(chip->V[x] != kk){
          chip->pc += 2;
          printf("SNE skipping next instruction\n");
        }
      }
      break;

    case 0x6000: //6XNN -> set V[X] = NN
      {
      uint8_t x = (opcode & 0x0F00) >> 8;
      uint8_t nn = (opcode & 0x00FF);
      
      chip->V[x] = nn;
      printf("SET V[%d] = %d\n", x, nn);
      }
      break;

    case 0x7000: // 7XNN: add NN to V[X]
    {
        uint8_t x = (opcode & 0x0F00) >> 8;
        uint8_t nn = (opcode & 0x00FF);
        
        chip->V[x] += nn;
        
        printf("ADD V[%d] += %d\n", x, nn);
    }
    break;

    case 0xA000: // ANNN: Set Index (I) to NNN
      chip->I = opcode & 0x0FFF;
      printf("SET\n");
      break;

    case 0xD000: //Dxyn (Display n-byte sprite) 
    {
        uint8_t vx = (opcode & 0x0F00) >> 8;
        uint8_t vy = (opcode & 0x00F0) >> 4;
        
        // Cordinates
        uint8_t x = chip->V[vx];
        uint8_t y = chip->V[vy];
        uint8_t height = opcode & 0x000F;
        uint8_t pixel;

        //printf("DEBUG DRAW: I=%X | X=%d Y=%d | H=%d\n", chip->I, x, y, height);

        chip->V[0xF] = 0;

        for (int yline = 0; yline < height; yline++) {
            
            // Reading pixel from memory
            pixel = chip->memory[chip->I + yline];

            printf("  -> Mem[%X] = 0x%X\n", chip->I + yline, pixel);

            for (int xline = 0; xline < 8; xline++) {
                if ((pixel & (0x80 >> xline)) != 0) {
                    
                    int targetX = (x + xline) % 64;
                    int targetY = (y + yline) % 32;
                    int idx = targetX + (targetY * 64);

                    if (chip->gfx[idx] == 1) {
                        chip->V[0xF] = 1;
                    }
                    chip->gfx[idx] ^= 1;
                }
            }
        }
        chip->draw_flag = 1;
    }
    break;

    case 0xF000:
      switch(opcode & 0x00FF){
        case 0x1E: //Fx1E (Set I = I + Vx)
          {
            uint8_t x = (opcode & 0x0F00) >> 8;
            chip->I += chip->V[x];
          } 
        break;
      }
    break;

    default:
      printf("Opcode unknown: 0x%X\n", opcode);
  }
}

