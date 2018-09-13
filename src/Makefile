CC=gcc
CFLAGS=-Wall -Werror -Wextra -pedantic -std=c11 -O2 -fvisibility=hidden -g
SOFLAGS=$(CFLAGS) -DPIPES_BUILDING_LIB -fPIC
PREFIX=/usr/local
LIBDIR=$(PREFIX)/lib
INCDIR=$(PREFIX)/include

.PHONY: lib all examples man clean install uninstall

lib: ../build/libpipes.so

all: lib

../build/libpipes.so: ../build/pipes.o ../build/fpipes.o ../build/redirect.o
	$(CC) $(SOFLAGS) -shared -o $@ ../build/pipes.o ../build/fpipes.o ../build/redirect.o -Wl,-soname,libpipes.so.1

../build/pipes.o: pipes.c pipes.h
	$(CC) $(SOFLAGS) -c $< -o $@

../build/fpipes.o: fpipes.c fpipes.h
	$(CC) $(SOFLAGS) -c $< -o $@

../build/redirect.o: redirect.c
	$(CC) $(SOFLAGS) -c $< -o $@

clean:
	rm ../build/libpipes.so ../build/pipes.o ../build/fpipes.o ../build/redirect.o

install: lib
	install -s ../build/libpipes.so "$(LIBDIR)"
	ln -s ../build/libpipes.so "$(LIBDIR)/libpipes.so.1"
	ln -s ../build/libpipes.so.1 "$(LIBDIR)/libpipes.so.1.0.0"
	install pipes.h fpipes.h export.h "$(INCDIR)/pipes"

uninstall:
	rm -rv "$(LIBDIR)/libpipes.so.1.0.0" "$(LIBDIR)/libpipes.so.1" \
	       "$(LIBDIR)/libpipes.so" "$(INCDIR)/pipes"
