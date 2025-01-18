#!/bin/bash

source ./assert.sh

PROGRAM_EXEC="$1"

assert "diff-dd 3.0.0-prealpha" "" 0 $PROGRAM_EXEC version

exit 0
