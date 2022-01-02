#!/bin/bash

${GAPPA} edit filter \
    --jplace-path "data/jplace/sample_0_0.jplace.gz" \
    --keep-names ".*9.*" \
    --file-suffix "-a" \
    --out-dir ${OUTDIR}

testfile "${OUTDIR}/filter-a.jplace" 78427 || return 1
