#!/bin/bash

${GAPPA} edit split \
    --jplace-path "data/jplace/sample_0_0.jplace.gz" \
    --split-file "data/edit-split.csv" \
    --compress \
    --out-dir ${OUTDIR}

testfile "${OUTDIR}/sample_a.jplace.gz" 5398 5398 || return 1
testfile "${OUTDIR}/sample_b.jplace.gz" 5311 5311 || return 1
testfile "${OUTDIR}/sample_c.jplace.gz" 5321 5321 || return 1
