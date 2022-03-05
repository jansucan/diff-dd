#!/bin/sh

source ./assert.sh

PROGRAM_EXEC="$1"

assert_error "unknown option '-x'" $PROGRAM_EXEC -x ref out

exit 0
