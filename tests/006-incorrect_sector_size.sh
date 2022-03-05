#!/bin/sh

source ./assert.sh

PROGRAM_EXEC="$1"

assert_error "incorrect buffer size" $PROGRAM_EXEC -b abc123 ref out
assert_error "buffer size cannot be 0" $PROGRAM_EXEC -b 0 ref out
assert_error "buffer size is not multiple of sector size" $PROGRAM_EXEC -b 3 -s 2 ref out

exit 0
