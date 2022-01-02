#!/bin/bash

# Test that setting the thread count works.

${GAPPA} examine info \
    --jplace-path "data/jplace/sample_0_0.jplace.gz" \
    --threads 6

GAPPA_THREADS=`egrep -- "--threads + 6" ${LOGDIR}/${TESTNAME}.log`
[[ -n ${GAPPA_THREADS} ]] || return 1
