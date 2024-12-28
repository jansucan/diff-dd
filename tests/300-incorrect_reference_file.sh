#!/bin/bash

source ./assert.sh

PROGRAM_EXEC="$1"

rm -f diff out
touch diff out
assert "" "diff file is empty" 1 $PROGRAM_EXEC restore -d diff -o out

dd if=/dev/zero of=diff bs=513 count=1 1>/dev/null 2>&1
assert "" "diff file has size that cannot contain valid diff data" \
       1 $PROGRAM_EXEC restore -S 512 -d diff -o out

rm -f diff out
dd if=/dev/zero of=out bs=512 count=2 1>/dev/null 2>&1
# Create a two-sector backup file
dd if=/dev/zero of=diff bs=$(( 512 + 8 )) count=2 1>/dev/null 2>&1
# The first offset will be 2
printf '\x02' | dd of=diff bs=1 count=1 seek=0 conv=notrunc  1>/dev/null 2>&1
# The second offset will be 1
printf '\x01' | dd of=diff bs=1 count=1 seek=520 conv=notrunc  1>/dev/null 2>&1
assert "" "a sector offset points behind the previous offset" \
       1 $PROGRAM_EXEC restore -S 512 -d diff -o out

rm -f diff out
dd if=/dev/zero of=out bs=512 count=1 1>/dev/null 2>&1
# Create a one-sector backup file
dd if=/dev/zero of=diff bs=$(( 512 + 8 )) count=2 1>/dev/null 2>&1
# The first offset will be 1
printf '\x01' | dd of=diff bs=1 count=1 seek=0 conv=notrunc  1>/dev/null 2>&1
assert "" "a sector offset points past the end of the output file" \
       1 $PROGRAM_EXEC restore -S 512 -d diff -o out

rm -f diff out

exit 0
