#!/bin/bash

${GAPPA} edit split \
    --jplace-path "data/jplace/" \
    --split-file "data/edit-split.csv" \
    --compress \
    --out-dir ${OUTDIR}

testfile "${OUTDIR}/sample_a.jplace.gz" 5301 || return 1
testfile "${OUTDIR}/sample_b.jplace.gz" 5384 || return 1
testfile "${OUTDIR}/sample_c.jplace.gz" 5287 || return 1
