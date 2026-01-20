
CC = clang
CFLAGS = -std=c17 -g -Wall
SDL2_FLAGS = $(shell sdl2-config --cflags --libs)

SRC = main.c
TARGET = chip-8

all:
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(SDL2_FLAGS)
clean:
	rm -f $(TARGET)