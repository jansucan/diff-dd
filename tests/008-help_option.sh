#!/bin/bash

source ./assert.sh

PROGRAM_EXEC="$1"

assert "Usage" "" 0 $PROGRAM_EXEC help
assert "Usage" "" 0 $PROGRAM_EXEC backup -h
assert "Usage" "" 0 $PROGRAM_EXEC restore -h

exit 0
