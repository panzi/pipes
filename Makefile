CC=gcc
CFLAGS=-Wall -Werror -Wextra -pedantic -std=c99 -O2 -fvisibility=hidden
PREFIX=/usr/local
LIBDIR=$(PREFIX)/lib
INCDIR=$(PREFIX)/include

.PHONY: lib all clean install uninstall

lib: libpipes.so

all: lib demo fdemo

libpipes.so: pipes_pic.o fpipes_pic.o redirect_pic.o
	$(CC) $(CFLAGS) -shared -o $@ pipes_pic.o fpipes_pic.o redirect_pic.o -Wl,-soname,libpipes.so.1

demo: demo.o pipes.o redirect.o pipes.h
	$(CC) $(CFLAGS) demo.o pipes.o redirect.o -o $@

fdemo: fdemo.o fpipes.o redirect.o fpipes.h
	$(CC) $(CFLAGS) fdemo.o fpipes.o redirect.o -o $@

pipes.o: pipes.c pipes.h
	$(CC) $(CFLAGS) -c $< -o $@

demo.o: demo.c pipes.h
	$(CC) $(CFLAGS) -c $< -o $@

fpipes.o: fpipes.c fpipes.h
	$(CC) $(CFLAGS) -c $< -o $@

fdemo.o: fdemo.c fpipes.h
	$(CC) $(CFLAGS) -c $< -o $@

redirect.o: redirect.c
	$(CC) $(CFLAGS) -c $< -o $@

pipes_pic.o: pipes.c pipes.h
	$(CC) $(CFLAGS) -c $< -o $@ -fPIC

fpipes_pic.o: fpipes.c fpipes.h
	$(CC) $(CFLAGS) -c $< -o $@ -fPIC

redirect_pic.o: redirect.c
	$(CC) $(CFLAGS) -c $< -o $@ -fPIC

clean:
	rm demo fdemo demo.o pipes.o fdemo.o fpipes.o redirect.o \
	   libpipes.so pipes_pic.o fpipes_pic.o redirect_pic.o

install: lib
	install libpipes.so "$(LIBDIR)"
	ln -s libpipes.so "$(LIBDIR)/libpipes.so.1"
	ln -s libpipes.so.1 "$(LIBDIR)/libpipes.so.1.0.0"
	install pipes.h fpipes.h export.h "$(INCDIR)/pipes"

uninstall:
	rm -rv "$(LIBDIR)/libpipes.so.1.0.0" "$(LIBDIR)/libpipes.so.1" \
	       "$(LIBDIR)/libpipes.so" "$(INCDIR)/pipes"
