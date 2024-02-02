include config.mk

all:
	$(MAKE) -C src all

.PHONY: test clean

test: all
	$(MAKE) -C tests

clean:
	$(MAKE) -C src clean

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f src/diff-dd ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/diff-dd

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/diff-dd
