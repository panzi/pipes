CC=gcc
CFLAGS=-Wall -Werror -Wextra -pedantic -std=c99 -O2 -fvisibility=hidden -g
PREFIX=/usr/local
LIBDIR=$(PREFIX)/lib
INCDIR=$(PREFIX)/include

.PHONY: lib all examples clean install uninstall

lib: libpipes.so

all: lib examples

examples:
	$(MAKE) -C examples

libpipes.so: pipes.o fpipes.o redirect.o
	$(CC) $(CFLAGS) -shared -o $@ pipes.o fpipes.o redirect.o -Wl,-soname,libpipes.so.1

pipes.o: pipes.c pipes.h
	$(CC) $(CFLAGS) -c $< -o $@ -fPIC

fpipes.o: fpipes.c fpipes.h
	$(CC) $(CFLAGS) -c $< -o $@ -fPIC

redirect.o: redirect.c
	$(CC) $(CFLAGS) -c $< -o $@ -fPIC

clean:
	rm libpipes.so pipes.o fpipes.o redirect.o
	$(MAKE) -C examples clean

install: lib
	install -s libpipes.so "$(LIBDIR)"
	ln -s libpipes.so "$(LIBDIR)/libpipes.so.1"
	ln -s libpipes.so.1 "$(LIBDIR)/libpipes.so.1.0.0"
	install pipes.h fpipes.h export.h "$(INCDIR)/pipes"

uninstall:
	rm -rv "$(LIBDIR)/libpipes.so.1.0.0" "$(LIBDIR)/libpipes.so.1" \
	       "$(LIBDIR)/libpipes.so" "$(INCDIR)/pipes"
