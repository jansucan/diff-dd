#!/bin/bash

source ./assert.sh

PROGRAM_EXEC="$1"

assert "Usage" "unknown option '-x'" 1 $PROGRAM_EXEC backup -x in base out
assert "Usage" "unknown option '-x'" 1 $PROGRAM_EXEC restore -x diff out

exit 0
