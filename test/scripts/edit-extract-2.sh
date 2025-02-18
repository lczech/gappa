#!/bin/bash

${GAPPA} edit extract \
    --jplace-path "data/jplace/sample_0_0.jplace.gz" \
    --clade-list-file "data/edit-extract.csv" \
    --exclude-clade-stems \
    --color-tree-file ${OUTDIR}/clades.svg \
    --samples-out-dir ${OUTDIR}

testfile "${OUTDIR}/basal.jplace"      12059  12059 || return 1
testfile "${OUTDIR}/busy.jplace"      249942 249942 || return 1
testfile "${OUTDIR}/clades.svg"       254689 254689 || return 1
testfile "${OUTDIR}/empty.jplace"       9978   9978 || return 1
testfile "${OUTDIR}/uncertain.jplace"  24742  24742 || return 1
