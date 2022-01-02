#!/bin/bash

${GAPPA} analyze krd \
    --jplace-path "data/jplace" \
    --out-dir ${OUTDIR}

testfile  "${OUTDIR}/krd_matrix.csv"      7847     ||  return  1
