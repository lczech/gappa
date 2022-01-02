#!/bin/bash

${GAPPA} edit filter \
    --jplace-path "data/jplace/sample_0_0.jplace.gz" \
    --min-mass-threshold 0.3 \
    --file-suffix "-a" \
    --out-dir ${OUTDIR}

testfile "${OUTDIR}/filtered-a.jplace" 148257 || return 1

${GAPPA} edit filter \
    --jplace-path "data/jplace/sample_0_0.jplace.gz" \
    --min-mass-threshold 0.3 \
    --normalize-after \
    --file-suffix "-b" \
    --out-dir ${OUTDIR}

testfile "${OUTDIR}/filtered-b.jplace" 146893 || return 1

${GAPPA} edit filter \
    --jplace-path "data/jplace/sample_0_0.jplace.gz" \
    --min-mass-threshold 0.3 \
    --no-remove-empty \
    --file-suffix "-c" \
    --out-dir ${OUTDIR}

testfile "${OUTDIR}/filtered-c.jplace" 166572 || return 1
