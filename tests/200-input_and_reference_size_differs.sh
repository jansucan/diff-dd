#!/bin/bash

source ./assert.sh

PROGRAM_EXEC="$1"

rm -f input ref
dd if=/dev/zero of=input bs=500 count=1 1>/dev/null 2>&1
dd if=/dev/zero of=ref bs=501 count=1 1>/dev/null 2>&1

assert "" "input file and reference file differ in size" 1 $PROGRAM_EXEC backup input ref out

rm -f input ref out

exit 0
