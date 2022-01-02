#!/bin/bash

${GAPPA} edit filter \
    --jplace-path "data/jplace/sample_0_0.jplace.gz" \
    --min-accumulated-mass 0.8 \
    --file-suffix "-a" \
    --out-dir ${OUTDIR}

testfile "${OUTDIR}/filter-a.jplace" 240181 || return 1

${GAPPA} edit filter \
    --jplace-path "data/jplace/sample_0_0.jplace.gz" \
    --min-accumulated-mass 0.8 \
    --normalize-after \
    --file-suffix "-b" \
    --out-dir ${OUTDIR}

testfile "${OUTDIR}/filter-b.jplace" 240181 || return 1
