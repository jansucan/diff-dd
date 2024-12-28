#!/bin/bash

source ./assert.sh

PROGRAM_EXEC="$1"

assert "Usage" "unknown option '-x'" 1 $PROGRAM_EXEC backup -x -i in -b base -o out
assert "Usage" "unknown option '-x'" 1 $PROGRAM_EXEC restore -x -d diff -o out

exit 0
