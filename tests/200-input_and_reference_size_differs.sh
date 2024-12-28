#!/bin/bash

source ./assert.sh

PROGRAM_EXEC="$1"

rm -f input base
dd if=/dev/zero of=input bs=500 count=1 1>/dev/null 2>&1
dd if=/dev/zero of=base bs=501 count=1 1>/dev/null 2>&1

assert "" "input file and base file differ in size" 1 $PROGRAM_EXEC backup -i input -b base -o out

rm -f input base out

exit 0
