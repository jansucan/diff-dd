#!/bin/bash

source ./assert.sh

PROGRAM_EXEC="$1"

rm -f input base out
touch base out
assert "" "cannot get size of input file" 1 $PROGRAM_EXEC backup input base out

rm -f input base out
touch input out
assert "" "cannot get size of base file" 1 $PROGRAM_EXEC backup input base out

rm -f input base out
rmdir outdir 2>/dev/null
touch input base
mkdir outdir
chmod -w outdir
assert "" "cannot open output file" 1 $PROGRAM_EXEC backup input base outdir/out

rm -f input base out
rmdir outdir

exit 0
