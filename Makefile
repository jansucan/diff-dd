PROGRAM_NAME=diff-dd
PROGRAM_VERSION=1.0

CC=gcc
CFLAGS=-Wall
EXEC_NAME=diff-dd

SOURCES=*.c
HEADERS=*.h

all: $(SOURCES) $(HEADERS) program_info_header
	$(CC) $(CFLAGS) -o $(EXEC_NAME) $(SOURCES)

program_info_header:
	echo "#define PROGRAM_NAME_STR \"$(PROGRAM_NAME)\"" >program_info.h
	echo "#define PROGRAM_VERSION_STR \"$(PROGRAM_VERSION)\"" >>program_info.h

.PHONY: clean

clean:
	rm -f *.o *~ $(EXEC_NAME) program_info.h
