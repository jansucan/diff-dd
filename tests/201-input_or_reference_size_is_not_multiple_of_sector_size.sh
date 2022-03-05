#!/bin/sh

source ./assert.sh

PROGRAM_EXEC="$1"

rm -f input ref
dd if=/dev/zero of=input bs=513 count=1 1>/dev/null 2>&1
dd if=/dev/zero of=ref bs=513 count=1 1>/dev/null 2>&1

assert_error "size of input file and reference file is not multiple of [0-9]" \
             $PROGRAM_EXEC -s 512 input ref out

rm -f input ref out

exit 0
