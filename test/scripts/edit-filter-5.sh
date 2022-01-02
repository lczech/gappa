#!/bin/bash

${GAPPA} edit filter \
    --jplace-path "data/jplace/sample_0_0.jplace.gz" \
    --remove-names ".*9.*" \
    --file-suffix "-a" \
    --out-dir ${OUTDIR}

testfile "${OUTDIR}/filtered-a.jplace" 198337 || return 1
