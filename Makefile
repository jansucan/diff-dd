PROGRAM_NAME=diff-dd
PROGRAM_VERSION=1.0
# Program name is used in the tests to locate the executable file
export PROGRAM_NAME

CC=gcc
CFLAGS=-Wall

SOURCES=*.c
HEADERS=*.h

all: $(PROGRAM_NAME)

$(PROGRAM_NAME): $(SOURCES) $(HEADERS) program_info.h
	$(CC) $(CFLAGS) -o $(PROGRAM_NAME) $(SOURCES)

program_info.h:
	echo "#define PROGRAM_NAME_STR \"$(PROGRAM_NAME)\"" >program_info.h
	echo "#define PROGRAM_VERSION_STR \"$(PROGRAM_VERSION)\"" >>program_info.h

.PHONY: test clean

test: all
	$(MAKE) -C tests

clean:
	rm -f *.o *~ $(PROGRAM_NAME) program_info.h
