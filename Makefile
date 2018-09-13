.PHONY: lib all examples man clean install uninstall

lib:
	$(MAKE) -C src

man:

all: lib examples

examples:
	$(MAKE) -C examples

clean:
	$(MAKE) -C src clean
	$(MAKE) -C examples clean

install:
	$(MAKE) -C src install
	$(MAKE) -C man install

uninstall:
	$(MAKE) -C src uninstall
	$(MAKE) -C man uninstall
