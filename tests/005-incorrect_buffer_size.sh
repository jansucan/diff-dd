#!/bin/sh

source ./assert.sh

PROGRAM_EXEC="$1"

assert_error "incorrect sector size" $PROGRAM_EXEC -s abc123 ref out
assert_error "sector size cannot be 0" $PROGRAM_EXEC -s 0 ref out
assert_error "sector size cannot larger than buffer size" $PROGRAM_EXEC -s 2 -b 1 ref out

exit 0
