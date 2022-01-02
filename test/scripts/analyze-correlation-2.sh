#!/bin/bash

${GAPPA} analyze correlation \
    --jplace-path "data/jplace" \
    --mass-norm "absolute" \
    --point-mass \
    --metadata-table-file "data/meta.csv" \
    --metadata-separator-char tab \
    --metadata-select-columns "score1" \
    --write-svg-tree \
    --svg-tree-stroke-width 8.0 \
    --svg-tree-ladderize \
    --out-dir ${OUTDIR}

testfile "${OUTDIR}/score1_imbalances_pearson.svg"  256771 || return 1
testfile "${OUTDIR}/score1_imbalances_spearman.svg" 256771 || return 1
testfile "${OUTDIR}/score1_masses_pearson.svg"      256771 || return 1
testfile "${OUTDIR}/score1_masses_spearman.svg"     256769 || return 1
