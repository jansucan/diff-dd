#!/bin/sh

source ./assert.sh

PROGRAM_EXEC="$1"

assert_error "too many arguments" $PROGRAM_EXEC arg1 arg2 arg3 arg4

exit 0
