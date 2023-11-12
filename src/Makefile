CC=g++
CFLAGS=-Wall -fmax-errors=2

SOURCES=*.cpp
HEADERS=*.h

all: $(PROGRAM_NAME)

$(PROGRAM_NAME): $(SOURCES) $(HEADERS) program_info.h
	$(CC) $(CFLAGS) -o $(PROGRAM_NAME) $(SOURCES)

program_info.h:
	echo "#define PROGRAM_NAME_STR \"$(PROGRAM_NAME)\"" >program_info.h
	echo "#define PROGRAM_VERSION_STR \"$(PROGRAM_VERSION)\"" >>program_info.h

.PHONY: clean

clean:
	rm -f *.o *~ $(PROGRAM_NAME) program_info.h
