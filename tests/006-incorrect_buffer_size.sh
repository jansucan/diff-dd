#!/bin/bash

source ./assert.sh

PROGRAM_EXEC="$1"

assert "Usage" "incorrect sector size" 1 $PROGRAM_EXEC backup -S abc123 -i in -b base -o out
assert "Usage" "sector size cannot be 0" 1 $PROGRAM_EXEC backup -S 0 -i in -b base -o out
assert "Usage" "sector size cannot larger than buffer size" 1 $PROGRAM_EXEC backup -S 2 -B 1 -i in -b base -o out

assert "Usage" "incorrect sector size" 1 $PROGRAM_EXEC restore -S abc123 -d diff -o out
assert "Usage" "sector size cannot be 0" 1 $PROGRAM_EXEC restore -S 0 -d diff -o out
assert "Usage" "sector size cannot larger than buffer size" 1 $PROGRAM_EXEC restore -S 2 -B 1 -d diff -o out

exit 0
