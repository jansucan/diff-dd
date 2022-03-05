#!/bin/sh

source ./assert.sh

PROGRAM_EXEC="$1"

assert_error "missing argument for option '-b'" $PROGRAM_EXEC -b
assert_error "missing argument for option '-s'" $PROGRAM_EXEC -s

exit 0
