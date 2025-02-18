#!/bin/bash

${GAPPA} examine heat-tree \
    --jplace-path "data/jplace/" \
    --write-svg-tree \
    --svg-tree-stroke-width 8.0 \
    --svg-tree-ladderize \
    --out-dir ${OUTDIR}

testfile "${OUTDIR}/tree.svg" 256702 256702 || return 1
