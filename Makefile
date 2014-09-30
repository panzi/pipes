CC=gcc
CFLAGS=-Wall -Werror -Wextra -pedantic -std=c99 -O2

.PHONY: all clean

all: demo

demo: demo.o pipes.o pipes.h
	$(CC) $(CFLAGS) demo.o pipes.o -o $@

%.o: %.c pipes.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm demo demo.o pipes.o
