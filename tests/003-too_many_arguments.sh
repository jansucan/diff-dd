#!/bin/sh

source ./assert.sh

PROGRAM_EXEC="$1"

assert "Usage" "too many arguments" 1 $PROGRAM_EXEC backup arg1 arg2 arg3 arg4
assert "Usage" "too many arguments" 1 $PROGRAM_EXEC restore arg1 arg2 arg3

exit 0
