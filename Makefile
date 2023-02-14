PROGRAM_NAME=diff-dd
PROGRAM_VERSION=1.0.0

export PROGRAM_NAME
export PROGRAM_VERSION

all:
	$(MAKE) -C src all

.PHONY: test clean

test: all
	$(MAKE) -C tests

clean:
	$(MAKE) -C src clean
