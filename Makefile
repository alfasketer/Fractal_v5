src = fractal.c TinyPngOut.c TinyPngOut.h
src2 = unbalanced.c TinyPngOut.c TinyPngOut.h

CC = gcc
CFLAGS = -std=c99 -pedantic -Wall
LDFLAGS = -lm -pthread

.PHONY: all
all: fractal unbalanced

fractal:
	$(CC) -o fractal.out $(src) $(LDFLAGS)

unbalanced:
	$(CC) -o unbalanced.out $(src2) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f fractal.out unbalanced.out
