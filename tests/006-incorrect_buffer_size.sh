#!/bin/bash

source ./assert.sh

PROGRAM_EXEC="$1"

assert "Usage" "incorrect buffer size" 1 $PROGRAM_EXEC backup -B abc123 -i in -b base -o out
assert "Usage" "buffer size cannot be 0" 1 $PROGRAM_EXEC backup -B 0 -i in -b base -o out

assert "Usage" "incorrect buffer size" 1 $PROGRAM_EXEC restore -B abc123 -d diff -o out
assert "Usage" "buffer size cannot be 0" 1 $PROGRAM_EXEC restore -B 0 -d diff -o out

exit 0
