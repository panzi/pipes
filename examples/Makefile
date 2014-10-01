CC=gcc
CFLAGS=-Wall -Werror -Wextra -pedantic -std=c99 -O2 -fvisibility=hidden -g -I..

.PHONY: all clean

all: demo fdemo

demo: demo.o pipes.o redirect.o ../pipes.h
	$(CC) $(CFLAGS) demo.o pipes.o redirect.o -o $@

fdemo: fdemo.o fpipes.o redirect.o ../fpipes.h
	$(CC) $(CFLAGS) fdemo.o fpipes.o redirect.o -o $@

pipes.o: ../pipes.c ../pipes.h
	$(CC) $(CFLAGS) -c $< -o $@

demo.o: demo.c ../pipes.h
	$(CC) $(CFLAGS) -c $< -o $@

fpipes.o: ../fpipes.c ../fpipes.h
	$(CC) $(CFLAGS) -c $< -o $@

fdemo.o: fdemo.c ../fpipes.h
	$(CC) $(CFLAGS) -c $< -o $@

redirect.o: ../redirect.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm demo fdemo demo.o pipes.o fdemo.o fpipes.o redirect.o
