#!/bin/sh

source ./assert.sh

PROGRAM_EXEC="$1"

rm -f ref out
touch ref out
assert_error "reference file is empty" $PROGRAM_EXEC ref out

dd if=/dev/zero of=ref bs=513 count=1 1>/dev/null 2>&1
assert_error "reference file has size that cannot contain valid diff data" \
             $PROGRAM_EXEC -s 512 ref out

rm -f ref out
dd if=/dev/zero of=out bs=512 count=2 1>/dev/null 2>&1
# Create a two-sector backup file
dd if=/dev/zero of=ref bs=$(( 512 + 8 )) count=2 1>/dev/null 2>&1
# The first offset will be 2
printf '\x02' | dd of=ref bs=1 count=1 seek=0 conv=notrunc  1>/dev/null 2>&1
# The second offset will be 1
printf '\x01' | dd of=ref bs=1 count=1 seek=520 conv=notrunc  1>/dev/null 2>&1
assert_error "a sector offset points behind the previous offset" \
             $PROGRAM_EXEC -s 512 ref out

rm -f ref out
dd if=/dev/zero of=out bs=512 count=1 1>/dev/null 2>&1
# Create a one-sector backup file
dd if=/dev/zero of=ref bs=$(( 512 + 8 )) count=2 1>/dev/null 2>&1
# The first offset will be 1
printf '\x01' | dd of=ref bs=1 count=1 seek=0 conv=notrunc  1>/dev/null 2>&1
assert_error "a sector offset points past the end of the output file" \
             $PROGRAM_EXEC -s 512 ref out

rm -f ref out

exit 0
