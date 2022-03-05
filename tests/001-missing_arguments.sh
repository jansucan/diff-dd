#!/bin/sh

PROGRAM_EXEC="$1"

$PROGRAM_EXEC 1>/dev/null 2>&1

[ $? -ne 0 ] && exit 0 || exit 1
