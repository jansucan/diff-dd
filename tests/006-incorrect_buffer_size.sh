#!/bin/bash

source ./assert.sh

PROGRAM_EXEC="$1"

assert "Usage" "incorrect sector size" 1 $PROGRAM_EXEC backup -s abc123 in base out
assert "Usage" "sector size cannot be 0" 1 $PROGRAM_EXEC backup -s 0 in base out
assert "Usage" "sector size cannot larger than buffer size" 1 $PROGRAM_EXEC backup -s 2 -b 1 in base out

assert "Usage" "incorrect sector size" 1 $PROGRAM_EXEC restore -s abc123 diff out
assert "Usage" "sector size cannot be 0" 1 $PROGRAM_EXEC restore -s 0 diff out
assert "Usage" "sector size cannot larger than buffer size" 1 $PROGRAM_EXEC restore -s 2 -b 1 diff out

exit 0
