#!/bin/bash

source ./assert.sh

PROGRAM_EXEC="$1"

assert "Usage" "missing input file" 1 $PROGRAM_EXEC create
assert "Usage" "missing diff file" 1 $PROGRAM_EXEC restore

exit 0
