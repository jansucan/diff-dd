CC=gcc
CFLAGS=-Wall
EXEC_NAME=diff-dd

SOURCES=*.c
HEADERS=*.h

all: $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -o $(EXEC_NAME) $(SOURCES)

.PHONY: clean

clean:
	rm -f *.o *~ $(EXEC_NAME)
