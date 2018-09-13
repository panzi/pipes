CC=gcc
CFLAGS=-Wall -Werror -Wextra -pedantic -std=c99 -O2 -fvisibility=hidden -g -I../src
BUILD_DIR=../build/examples

.PHONY: all clean

all: $(BUILD_DIR)/chain $(BUILD_DIR)/chain_mt $(BUILD_DIR)/fchain $(BUILD_DIR)/temp $(BUILD_DIR)/ftemp

$(BUILD_DIR)/chain: $(BUILD_DIR)/chain.o $(BUILD_DIR)/pipes.o $(BUILD_DIR)/redirect.o ../src/pipes.h
	$(CC) $(CFLAGS) $(BUILD_DIR)/chain.o $(BUILD_DIR)/pipes.o $(BUILD_DIR)/redirect.o -o $@

$(BUILD_DIR)/chain.o: chain.c ../src/pipes.h
	$(CC) $(CFLAGS) -c $< -o $@


$(BUILD_DIR)/chain_mt: $(BUILD_DIR)/chain_mt.o $(BUILD_DIR)/pipes.o $(BUILD_DIR)/redirect.o ../src/pipes.h
	$(CC) $(CFLAGS) -lpthread $(BUILD_DIR)/chain_mt.o $(BUILD_DIR)/pipes.o $(BUILD_DIR)/redirect.o -o $@

$(BUILD_DIR)/chain_mt.o: chain_mt.c ../src/pipes.h
	$(CC) $(CFLAGS) -c $< -o $@


$(BUILD_DIR)/fchain: $(BUILD_DIR)/fchain.o $(BUILD_DIR)/fpipes.o $(BUILD_DIR)/redirect.o ../src/fpipes.h
	$(CC) $(CFLAGS) $(BUILD_DIR)/fchain.o $(BUILD_DIR)/fpipes.o $(BUILD_DIR)/redirect.o -o $@

$(BUILD_DIR)/fchain.o: fchain.c ../src/fpipes.h
	$(CC) $(CFLAGS) -c $< -o $@


$(BUILD_DIR)/temp: $(BUILD_DIR)/temp.o $(BUILD_DIR)/pipes.o $(BUILD_DIR)/redirect.o ../src/pipes.h
	$(CC) $(CFLAGS) $(BUILD_DIR)/temp.o $(BUILD_DIR)/pipes.o $(BUILD_DIR)/redirect.o -o $@

$(BUILD_DIR)/temp.o: temp.c ../src/pipes.h
	$(CC) $(CFLAGS) -c $< -o $@


$(BUILD_DIR)/ftemp: $(BUILD_DIR)/ftemp.o $(BUILD_DIR)/fpipes.o $(BUILD_DIR)/redirect.o ../src/pipes.h
	$(CC) $(CFLAGS) $(BUILD_DIR)/ftemp.o $(BUILD_DIR)/fpipes.o $(BUILD_DIR)/redirect.o -o $@

$(BUILD_DIR)/ftemp.o: ftemp.c ../src/pipes.h
	$(CC) $(CFLAGS) -c $< -o $@


$(BUILD_DIR)/pipes.o: ../src/pipes.c ../src/pipes.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/fpipes.o: ../src/fpipes.c ../src/fpipes.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/redirect.o: ../src/redirect.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm $(BUILD_DIR)/chain $(BUILD_DIR)/chain.o $(BUILD_DIR)/chain_mt $(BUILD_DIR)/chain_mt.o \
	   $(BUILD_DIR)/fchain $(BUILD_DIR)/fchain.o $(BUILD_DIR)/temp \
	   $(BUILD_DIR)/temp.o $(BUILD_DIR)/ftemp $(BUILD_DIR)/ftemp.o \
	   $(BUILD_DIR)/pipes.o $(BUILD_DIR)/fpipes.o $(BUILD_DIR)/redirect.o
