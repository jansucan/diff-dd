#!/bin/bash

source ./assert.sh

PROGRAM_EXEC="$1"

rm -f input base
dd if=/dev/zero of=input bs=513 count=1 1>/dev/null 2>&1
dd if=/dev/zero of=base bs=513 count=1 1>/dev/null 2>&1

assert "" "size of input file and base file is not multiple of [0-9]" \
       1 $PROGRAM_EXEC backup -S 512 -i input -b base -o out

rm -f input base out

exit 0
