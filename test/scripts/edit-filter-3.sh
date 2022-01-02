#!/bin/bash

${GAPPA} edit filter \
    --jplace-path "data/jplace/sample_0_0.jplace.gz" \
    --max-n-placements 1 \
    --file-suffix "-a" \
    --out-dir ${OUTDIR}

testfile "${OUTDIR}/filtered-a.jplace" 153624 || return 1

${GAPPA} edit filter \
    --jplace-path "data/jplace/sample_0_0.jplace.gz" \
    --max-n-placements 1 \
    --normalize-after \
    --file-suffix "-b" \
    --out-dir ${OUTDIR}

testfile "${OUTDIR}/filtered-b.jplace" 148026 || return 1
