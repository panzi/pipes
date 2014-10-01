CC=gcc
CFLAGS=-Wall -Werror -Wextra -pedantic -std=c99 -O2 -fvisibility=hidden -g -I..

.PHONY: all clean

all: chain fchain

chain: chain.o pipes.o redirect.o ../pipes.h
	$(CC) $(CFLAGS) chain.o pipes.o redirect.o -o $@

fchain: fchain.o fpipes.o redirect.o ../fpipes.h
	$(CC) $(CFLAGS) fchain.o fpipes.o redirect.o -o $@

pipes.o: ../pipes.c ../pipes.h
	$(CC) $(CFLAGS) -c $< -o $@

chain.o: chain.c ../pipes.h
	$(CC) $(CFLAGS) -c $< -o $@

fpipes.o: ../fpipes.c ../fpipes.h
	$(CC) $(CFLAGS) -c $< -o $@

fchain.o: fchain.c ../fpipes.h
	$(CC) $(CFLAGS) -c $< -o $@

redirect.o: ../redirect.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm chain fchain chain.o pipes.o fchain.o fpipes.o redirect.o
