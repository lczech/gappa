#!/bin/bash

${GAPPA} analyze placement-factorization \
    --jplace-path "data/jplace" \
    --metadata-table-file "data/meta.csv" \
    --metadata-separator-char tab \
    --metadata-select-columns "score1"$'\t'"score2" \
    --write-svg-tree \
    --svg-tree-stroke-width 8.0 \
    --svg-tree-ladderize \
    --out-dir ${OUTDIR}

testfile  "${OUTDIR}/factor_balances.csv"        1777 || return 1
testfile  "${OUTDIR}/factor_edges_1.svg"       254171 || return 1
testfile  "${OUTDIR}/factor_edges_2.svg"       254171 || return 1
testfile  "${OUTDIR}/factor_edges_3.svg"       254171 || return 1
testfile  "${OUTDIR}/factor_edges_4.svg"       254171 || return 1
testfile  "${OUTDIR}/factor_edges_5.svg"       254171 || return 1
testfile  "${OUTDIR}/factors_tree.svg"         254171 || return 1
testfile  "${OUTDIR}/factor_taxa.txt"            2268 || return 1
testfile  "${OUTDIR}/objective_values_1.svg"   275691 || return 1
testfile  "${OUTDIR}/objective_values_2.svg"   276080 || return 1
testfile  "${OUTDIR}/objective_values_3.svg"   275304 || return 1
testfile  "${OUTDIR}/objective_values_4.svg"   275700 || return 1
testfile  "${OUTDIR}/objective_values_5.svg"   275702 || return 1
