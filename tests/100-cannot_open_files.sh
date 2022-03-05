#!/bin/sh

source ./assert.sh

PROGRAM_EXEC="$1"

rm -f input ref out
touch ref out
assert_error "cannot open input file" $PROGRAM_EXEC input ref out

rm -f input ref out
touch input out
assert_error "cannot open reference file" $PROGRAM_EXEC input ref out

rm -f input ref out
rmdir outdir 2>/dev/null
touch input ref
mkdir outdir
chmod -w outdir
assert_error "cannot open output file" $PROGRAM_EXEC input ref outdir/out

rm -f input ref out
rmdir outdir

exit 0
