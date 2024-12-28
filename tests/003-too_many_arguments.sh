#!/bin/bash

source ./assert.sh

PROGRAM_EXEC="$1"

assert "Usage" "too many arguments" 1 $PROGRAM_EXEC backup -i arg1 -b arg2 -o arg3 arg4
assert "Usage" "too many arguments" 1 $PROGRAM_EXEC restore -d arg1 -o arg2 arg3

exit 0
