#!/bin/bash

source ./assert.sh

PROGRAM_EXEC="$1"

assert "Usage" "" 0 $PROGRAM_EXEC help

exit 0
