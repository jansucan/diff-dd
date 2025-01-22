#!/bin/bash

source ./assert.sh

PROGRAM_EXEC="$1"

function files_are_the_same()
{
    [ -z "$(diff "$1" "$2")" ]
}

rm -f input backedup_input base out

# Create a four-sector base file (the original file)
dd if=/dev/zero of=base bs=512 count=4 1>/dev/null 2>&1

# Change the orignal file to make it an input file for differential backup
# There will be four different sectors in the input file
cp base input

# The first sector will have the 0th byte chaged
printf '\xFF' | dd of=input bs=1 count=1 seek=0 conv=notrunc  1>/dev/null 2>&1

# The second sector will have no changes

# The third sector will have the last byte changed
printf '\xFF' | dd of=input bs=1 count=1 seek=$(( (512 * 3) - 1 )) conv=notrunc  1>/dev/null 2>&1

# The fourth sector will have the middle byte changed
printf '\xFF' | dd of=input bs=1 count=1 seek=$(( (512 * 4) - (512 / 2) )) conv=notrunc  1>/dev/null 2>&1

assert "" "" 0 $PROGRAM_EXEC create -i input -b base -o out

if ! files_are_the_same out 400-expected_backup_output.bin; then
    echo "assert: Backup output file differs from the expected one"
    exit 1
fi

# Modify the file to backup (the input file used in the backup phase)
cp input backedup_input
cp base input
if ! files_are_the_same base input; then
    echo "assert: The input file must be the same as the base one before restoring it"
    exit 1
fi

assert "" "" 0 $PROGRAM_EXEC restore -d out -o input

if ! files_are_the_same input backedup_input; then
    echo "assert: Cannot restore the backup"
    exit 1
fi

rm -f input backedup_input base out

exit 0
