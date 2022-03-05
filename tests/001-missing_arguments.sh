#!/bin/sh

source ./assert.sh

PROGRAM_EXEC="$1"

assert_error "missing arguments" $PROGRAM_EXEC
assert_error "missing arguments" $PROGRAM_EXEC arg

exit 0
