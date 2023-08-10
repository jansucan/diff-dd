#!/bin/bash

source ./assert.sh

PROGRAM_EXEC="$1"

assert "Usage" "missing arguments" 1 $PROGRAM_EXEC backup
assert "Usage" "missing arguments" 1 $PROGRAM_EXEC restore

exit 0
