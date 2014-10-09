MAN2HTML=man2html
MANDIR=$(PREFIX)/share/man

.PHONY: all html clean install uninstall

all: html

html: pipes.h.html

pipes.h.html: pipes.h.3
	$(MAN2HTML) ./pipes.h.3 | tail -n +3 > $@

clean:
	rm -r pipes.h.html

install:
	install pipes.h.3 "$(MANDIR)/man3"

uninstall:
	rm -v "$(MANDIR)/man3/pipes.h.3"
