src = fractal.c TinyPngOut.c TinyPngOut.h

CC = gcc
CFLAGS = -std=c99 -pedantic -Wall
LDFLAGS = -lm -pthread

.PHONY: all
all: fractal

fractal:
	$(CC) -o fractal.out $(src) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f fractal.out
