#!/bin/sh

source ./assert.sh

PROGRAM_EXEC="$1"

assert "Usage" "missing argument for option '-b'" 1 $PROGRAM_EXEC backup -b
assert "Usage" "missing argument for option '-s'" 1 $PROGRAM_EXEC backup -s

assert "Usage" "missing argument for option '-b'" 1 $PROGRAM_EXEC restore -b
assert "Usage" "missing argument for option '-s'" 1 $PROGRAM_EXEC restore -s

exit 0
