CC=gcc
CFLAGS=-Wall -Werror -Wextra -pedantic -std=c99 -O2 -fvisibility=hidden -g -I..

.PHONY: all clean

all: chain fchain temp ftemp

chain: chain.o pipes.o redirect.o ../pipes.h
	$(CC) $(CFLAGS) chain.o pipes.o redirect.o -o $@

chain.o: chain.c ../pipes.h
	$(CC) $(CFLAGS) -c $< -o $@


fchain: fchain.o fpipes.o redirect.o ../fpipes.h
	$(CC) $(CFLAGS) fchain.o fpipes.o redirect.o -o $@

fchain.o: fchain.c ../fpipes.h
	$(CC) $(CFLAGS) -c $< -o $@


temp: temp.o pipes.o redirect.o ../pipes.h
	$(CC) $(CFLAGS) temp.o pipes.o redirect.o -o $@

temp.o: temp.c ../pipes.h
	$(CC) $(CFLAGS) -c $< -o $@


ftemp: ftemp.o fpipes.o redirect.o ../pipes.h
	$(CC) $(CFLAGS) ftemp.o fpipes.o redirect.o -o $@

ftemp.o: ftemp.c ../pipes.h
	$(CC) $(CFLAGS) -c $< -o $@


pipes.o: ../pipes.c ../pipes.h
	$(CC) $(CFLAGS) -c $< -o $@

fpipes.o: ../fpipes.c ../fpipes.h
	$(CC) $(CFLAGS) -c $< -o $@

redirect.o: ../redirect.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm chain chain.o fchain fchain.o temp temp.o ftemp ftemp.o \
	   pipes.o fpipes.o redirect.o
