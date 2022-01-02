#!/bin/bash

# Test that the openmp thread setting works.
export OMP_NUM_THREADS=4

${GAPPA} examine info \
    --jplace-path "data/jplace/sample_0_0.jplace.gz"

GAPPA_THREADS=`egrep -- "--threads + 4" ${LOGDIR}/${TESTNAME}.log`
[[ -n ${GAPPA_THREADS} ]] || return 1
