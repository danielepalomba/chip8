CC = gcc

CFLAGS = -Wall -Wextra -g -std=c11

SDL_CFLAGS = $(shell sdl2-config --cflags)
SDL_LDFLAGS = $(shell sdl2-config --libs)

CFLAGS += $(SDL_CFLAGS)
LDFLAGS = $(SDL_LDFLAGS)

TARGET = chip8
SRC = main.c chip8.c
OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

run: all
	./$(TARGET) roms/PONG
