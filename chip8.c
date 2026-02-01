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
          case 0xE0: // 00E0 -> CLS (Clear the display)
            memset(chip->gfx, 0, sizeof(chip->gfx));
            chip->draw_flag = 1;
            printf("CLS\n");
            break;
          case 0xEE: // 00EE -> RET (Return from subroutine)
            chip->sp--;
            chip->pc = chip->stack[chip->sp];
            printf("RET 0x%X\n", chip->pc);
            break;
          default:
            printf("Opcode unknown: 0x%X\n", opcode);
            break;
        }
        break;

    case 0x1000: // 1nnn -> JP (Jump to nnn)
      chip->pc = opcode & 0x0FFF; 
      printf("JUMP a 0x%X\n", chip->pc);
      break;
    
    case 0x2000: //2nnn -> CALL (Call subroutine at nnn)
      {
        uint16_t nnn = (opcode & 0x0FFF);
        chip->stack[chip->sp] = chip->pc;
        chip->sp++;

        chip->pc = nnn;

        printf("Call subroutine at %d\n", nnn);
      }
      break;
    
    case 0x3000: //3xkk -> SE (Skip next instruction if Vx = kk)
      {
        uint8_t x = (opcode & 0x0F00) >> 8;
        uint8_t kk = (opcode & 0x00FF);

        if(chip->V[x] == kk){
          chip->pc += 2;
          printf("SE skipping next instruction\n");
        }
      }
    break;

    case 0x4000: //4xkk -> SNE (Skip next instruction if Vx != kk)
      {
        uint8_t kk = (opcode & 0x00FF);
        uint8_t x = (opcode & 0x0F00) >> 8;
        if(chip->V[x] != kk){
          chip->pc += 2;
          printf("SNE skipping next instruction\n");
        }
      }
      break;
    
    case 0x5000: //5xy0 -> SE (Skip next instruction if Vx = Vy)
      {
        uint8_t x = (opcode & 0x0F00) >> 8;
        uint8_t y = (opcode & 0x00F0) >> 4;

        if(chip->V[x] == chip->V[y]){
          chip->pc += 2;
          printf("5xy0 Skipping next instruction (V[%d] != V[%d])\n", x,y);
        }
      }
    break;

    case 0x6000: //6xkk -> LD (Set V[x] = kk)
      {
      uint8_t x = (opcode & 0x0F00) >> 8;
      uint8_t nn = (opcode & 0x00FF);
      
      chip->V[x] = nn;
      printf("SET V[%d] = %d\n", x, nn);
      }
      break;

    case 0x7000: // 7xkk -> ADD (Set V[x] = V[x] + kk)
    {
        uint8_t x = (opcode & 0x0F00) >> 8;
        uint8_t nn = (opcode & 0x00FF);
        
        chip->V[x] += nn;
        
        printf("ADD V[%d] += %d\n", x, nn);
    }
    break;
    
    case 0x8000:
      switch(opcode & 0x000F){
        case 0: // 8xy0 -> LD (Set V[x] = V[y])
          {
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t y = (opcode & 0x00F0) >> 4;

            chip->V[x] = chip->V[y];
            printf("8xy0 Set V[%d] = v[%d]\n", x,y);
          }
        break;

        case 0x1: //8xy1 -> OR (Set V[x] = V[x] OR V[y]) 
          {
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t y = (opcode & 0x00F0) >> 4;
            
            chip->V[x] |= chip->V[y];
            printf("OR - Index: x = %d, y = %d\n", x,y);
          }
          break;

        case 0x2: //8xy2 -> AND (Set V[x] = V[x] AND V[y])
          {
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t y = (opcode & 0x00F0) >> 4;
          
            chip->V[x] &= chip->V[y];
            printf("AND - Index x = %d, y = %d\n", x,y);
          } 
          break;

        case 0x3: // 8xy3 -> XOR (Set V[x] = V[x] XOR V[y])
          {
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t y = (opcode & 0x00F0) >> 4;
            
            chip->V[x] ^= chip->V[y];
            printf("XOR - Index: x = %d, y = %d\n", x,y);
          }
          break;
        
        case 0x4: // 8xy4 -> ADD (Set V[x] = V[x] + V[y], set V[F] = carry)
          {
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t y = (opcode & 0x00F0) >> 4;

            if ((uint16_t)chip->V[x] + (uint16_t)chip->V[y] > 255) {
                chip->V[0xF] = 1;
            } else {
                chip->V[0xF] = 0;
            }

            chip->V[x] += chip->V[y];
            
            printf("8xy4: V[%d] += V[%d] (Result: %d, Carry: %d)\n", 
                    x, y, chip->V[x], chip->V[0xF]);
          }
          break;

        case 0x5: // 8xy5 -> SUB (Set Vx = Vx - Vy, set VF = NOT borrow)
          {
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t y = (opcode & 0x00F0) >> 4;
            if(chip->V[x] >= chip->V[y]){
              chip->V[0xF] = 1;
            }else{
              chip->V[0xF] = 0;
            }

            chip->V[x] -= chip->V[y];
            printf("SUB: V[%d] = V[%d] - V[%d] (VF=%d)\n", x,x,y,chip->V[0xF]);
          }
          break;

        case 0x6: //8xy6 -> SHR (Set V[x] = V[x] SHR 1)
          {
            uint8_t x = (opcode & 0x0F00) >> 8;

            chip->V[0xF] = (chip->V[x] & 0x1);
            chip->V[x] >>= 1;
            printf("V[%d] SHR 1 (VF=%d)\n", x, chip->V[0xF]);
          }
          break;
          
        case 0x7: //8xy7 -> SUBN (Set V[x] = V[y] - V[x], set V[F] = NOT borrow)
          {
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t y = (opcode & 0x00F0) >> 4;
            
            if(chip->V[y] >= chip->V[x]){
              chip->V[0xF] = 1;
            }else{
              chip->V[0xF] = 0;
            }

            chip->V[x] = chip->V[y] - chip->V[x];
            printf("SUBN: V[%d] = V[%d] - V[%d] (VF=%d)\n", x,y,x,chip->V[0xF]);
          }
        break;

        case 0xE: //8xyE -> SHL (Set V[x] = V[x] SHL 1)
          {
            uint8_t x = (opcode & 0x0F00) >> 8;
            
            chip->V[0xF] = (chip->V[x] >> 7) & 0x1;
            chip->V[x] <<= 1;

            printf("V[%d] SHL 1 (VF=%d)\n", x,chip->V[0xF]);
          }
        break;

        default:
          printf("Opcode unknown: 0x%X\n", opcode);
      }
    break;

    case 0x9000: // 9xy0 -> SNE (Skip next instruction if Vx != Vy)
      {
        uint8_t x = (opcode & 0x0F00) >> 8;
        uint8_t y = (opcode & 0x00F0) >> 4;

        if (chip->V[x] != chip->V[y]) {
          chip->pc += 2;
          printf("9xy0 Skipping next instruction (V[%d] != V[%d])\n", x, y);
        }
      }
      break;

    case 0xA000: // Annn -> LD (Set Index (I) to nnn)
      chip->I = opcode & 0x0FFF;
      printf("SET\n");
      break;

    case 0xB000: //Bnnn -> JP (Jump to location nnn + V[0])
      chip->pc = (opcode & 0x0FFF) + chip->V[0];
      printf("JP to 0x%X + V[0](0x%X) = 0x%X\n", (opcode & 0x0FFF), chip->V[0], chip->pc);
      break;

    case 0xC000: //Cxkk -> RND (Set Vx = random byte AND kk)
      {
        uint8_t x = (opcode & 0x0F00) >> 8;
        uint16_t kk = (opcode & 0x00FF); 

        chip->V[x] = (rand() % 256) & kk;
        printf("RND V[%d] = %d (Rand AND %d)\n", x, chip->V[x], kk);
      }
      break;
      

    case 0xD000: //Dxyn -> DRW (Display n-byte sprite) 
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
    
    case 0xE000:
      switch(opcode & 0x00FF){
        case 0x9E: //Ex9E -> SKP (Skip next instruction if the key with value Vx is pressed)
          {
            uint8_t x = (opcode & 0x0F00) >> 8;
            if(chip->keypad[chip->V[x]]){ //pressed = 1
              chip->pc += 2;
              printf("Ex9E keypad at %d pressed\n", chip->V[x]);
            }
          }
        break;

        case 0xA1: //ExA1 -> SKNP (Skip next instruction if the key with value Vx is not pressed)
          {
            uint8_t x = (opcode & 0x0F00) >> 8;
            if(!chip->keypad[chip->V[x]]){
              chip->pc += 2;
              printf("ExA1 keypad at %d not pressed\n", chip->V[x]);
            }
          }
        break;

        default:
          printf("Opcode unknown: 0x%X\n", opcode);
      }
    break;

    case 0xF000:
      switch(opcode & 0x00FF){
        case 0x07: //Fx07 -> LD (Set Vx = delay_timer)
          {
            uint8_t x = (opcode & 0x0F00) >> 8;
            chip->V[x] = chip->delay_timer;
            printf("Fx07 V[%d] = %d(delay_timer)\n", x, chip->delay_timer);
          }
        break;

        case 0x0A: //Fx0A -> LD Vx (Wait for a key press, store the value of the key in Vx.)
          {
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t key_pressed = 0;

            for(int i = 0; i<16; i++){
              if(chip->keypad[i]){
                chip->V[x] = i;
                key_pressed = 1;
                break;
              }
            }

            if(!key_pressed) // Iterate over the same instruction
              chip->pc -= 2;
            else
              printf("Key %d pressed, stored in V[%d]\n", chip->V[x], x);
          }
          break;

        case 0x15: //Fx15 -> LD DT (Set delay_timer = V[x])
          {
            uint8_t x = (opcode & 0x0F00) >> 8;
            chip->delay_timer = chip->V[x];
            printf("FX15 delay_timer = %d\n", chip->V[x]);
          }
        break;

        case 0x18: //Fx18 -> LD ST (Set sound_timer = V[x])
          chip->sound_timer = chip->V[((opcode & 0x0F00)>>8)];
          break;

        case 0x29: // Fx29 -> LD F (Set I to location of sprite for digit V[x])
          {
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t digit = chip->V[x];
            chip->I = digit * 5; 
            printf("Set I to font char '%X' at address %X\n", digit, chip->I);
          }
          break;

        case 0x33: // Fx33 -> LD B (Store BCD representation of V[x] in memory locations I, I+1, and I+2)
          {
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t value = chip->V[x];

            chip->memory[chip->I] = value / 100;       
            chip->memory[chip->I + 1] = (value / 10) % 10; 
            chip->memory[chip->I + 2] = value % 10;        
            printf("BCD of %d stored at I(%d)\n", value, chip->I);
          }
          break;

        case 0x55: // Fx55 -> LD [I] (Store V[0]..V[x] in memory starting at I)
          {
            uint8_t x = (opcode & 0x0F00) >> 8;
            for (int i = 0; i <= x; i++) {
                chip->memory[chip->I + i] = chip->V[i];
            }
            chip->I = chip->I + x + 1;
          }
          break;

        case 0x1E: //Fx1E -> ADD I (Set I = I + V[x])
          {
            uint8_t x = (opcode & 0x0F00) >> 8;
            printf("SET I(%d) = I(%d) + Vx(%d)\n", chip->I, chip->I, chip->V[x]);
            chip->I += chip->V[x];
          } 
        break;

        case 0x65: //Fx65 -> LD V[x] (Fills V[0]...V[x] with values from memory starting at address I)
          {
            uint8_t x = (opcode & 0x0F00) >> 8;
            for(int i = 0; i<=x; i++){
              chip->V[i] = chip->memory[chip->I + i];
            }
            chip->I = chip->I + x + 1;
          }
        break;

        default:
          printf("Opcode unknown: 0x%X\n", opcode);
      }
    break;

    default:
      printf("Opcode unknown: 0x%X\n", opcode);
  }
}

