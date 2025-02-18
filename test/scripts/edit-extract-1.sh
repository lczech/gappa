#!/bin/bash

${GAPPA} edit extract \
    --jplace-path "data/jplace/sample_0_0.jplace.gz" \
    --clade-list-file "data/edit-extract.csv" \
    --color-tree-file ${OUTDIR}/clades.svg \
    --samples-out-dir ${OUTDIR}

testfile "${OUTDIR}/basal.jplace"          9978   9978 || return 1
testfile "${OUTDIR}/busy.jplace"         254119 254119 || return 1
testfile "${OUTDIR}/clades.svg"          254689 254689 || return 1
testfile "${OUTDIR}/empty.jplace"          9978   9978 || return 1
testfile "${OUTDIR}/uncertain.jplace"     22647  22647 || return 1
