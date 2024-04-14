#!/bin/bash

source ./assert.sh

PROGRAM_EXEC="$1"

rm -f input ref out
touch ref out
assert "" "cannot get size of input file" 1 $PROGRAM_EXEC backup input ref out

rm -f input ref out
touch input out
assert "" "cannot get size of reference file" 1 $PROGRAM_EXEC backup input ref out

rm -f input ref out
rmdir outdir 2>/dev/null
touch input ref
mkdir outdir
chmod -w outdir
assert "" "cannot open output file" 1 $PROGRAM_EXEC backup input ref outdir/out

rm -f input ref out
rmdir outdir

exit 0
