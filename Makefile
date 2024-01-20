include config.mk

all:
	$(MAKE) -C src all

.PHONY: test clean

test: all
	$(MAKE) -C tests

clean:
	$(MAKE) -C src clean
