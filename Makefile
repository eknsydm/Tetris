CC = gcc

CFLAGS = -Wall -lSDL2 -lSDL2_ttf

SRCDIR = ./src/
SOURCES = $(SRCDIR)*.c*
INCLUDES = 

all: build run clean


build:
	$(CC) $(SOURCES) $(CFLAGS) -o Tetris.o

run:
	SDL_VIDEODRIVER=wayland ./Tetris.o

clean:
	rm Tetris.o
