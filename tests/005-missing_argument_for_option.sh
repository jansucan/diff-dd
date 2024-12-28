#!/bin/bash

source ./assert.sh

PROGRAM_EXEC="$1"

assert "Usage" "missing argument for option '-B'" 1 $PROGRAM_EXEC backup -B
assert "Usage" "missing argument for option '-S'" 1 $PROGRAM_EXEC backup -S

assert "Usage" "missing argument for option '-B'" 1 $PROGRAM_EXEC restore -B
assert "Usage" "missing argument for option '-S'" 1 $PROGRAM_EXEC restore -S

exit 0
