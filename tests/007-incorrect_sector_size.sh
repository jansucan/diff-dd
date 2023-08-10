#!/bin/bash

source ./assert.sh

PROGRAM_EXEC="$1"

assert "Usage" "incorrect buffer size" 1 $PROGRAM_EXEC backup -b abc123 in ref out
assert "Usage" "buffer size cannot be 0" 1 $PROGRAM_EXEC backup -b 0 in ref out
assert "Usage" "buffer size is not multiple of sector size" 1 $PROGRAM_EXEC backup -b 3 -s 2 in ref out

assert "Usage" "incorrect buffer size" 1 $PROGRAM_EXEC restore -b abc123 in ref out
assert "Usage" "buffer size cannot be 0" 1 $PROGRAM_EXEC restore -b 0 in ref out
assert "Usage" "buffer size is not multiple of sector size" 1 $PROGRAM_EXEC restore -b 3 -s 2 in ref out

exit 0
