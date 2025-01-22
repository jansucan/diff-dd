#!/bin/bash

source ./assert.sh

PROGRAM_EXEC="$1"

assert "Usage" "missing argument for option '-B'" 1 $PROGRAM_EXEC create -B

assert "Usage" "missing argument for option '-B'" 1 $PROGRAM_EXEC restore -B

exit 0
