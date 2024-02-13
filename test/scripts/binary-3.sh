#!/bin/bash

# Test that the openmp thread setting works.
export OMP_NUM_THREADS=2

${GAPPA} examine info \
    --jplace-path "data/jplace/sample_0_0.jplace.gz"

GAPPA_THREADS=`egrep -- "--threads + 2" ${LOGDIR}/${TESTNAME}.log`

# On macos, we currently do not use OpenMP, so this fails.
# Hence, for now, we do not run this test on macos...
if [[ $OSTYPE == 'darwin'* ]]; then
    return 0
else
    [[ -n ${GAPPA_THREADS} ]] || return 1
fi
