#!/bin/bash

${GAPPA} analyze correlation \
    --jplace-path "data/jplace" \
    --mass-norm "relative" \
    --metadata-table-file "data/meta.csv" \
    --metadata-separator-char tab \
    --write-newick-tree \
    --write-nexus-tree \
    --write-phyloxml-tree \
    --write-svg-tree \
    --svg-tree-stroke-width 8.0 \
    --svg-tree-ladderize \
    --out-dir ${OUTDIR}

testfile  "${OUTDIR}/score1_imbalances_pearson.newick"    6810     ||  return  1
testfile  "${OUTDIR}/score1_imbalances_pearson.nexus"     20365    ||  return  1
testfile  "${OUTDIR}/score1_imbalances_pearson.phyloxml"  704758   ||  return  1
testfile  "${OUTDIR}/score1_imbalances_pearson.svg"       256771   ||  return  1
testfile  "${OUTDIR}/score1_imbalances_spearman.newick"   6810     ||  return  1
testfile  "${OUTDIR}/score1_imbalances_spearman.nexus"    20365    ||  return  1
testfile  "${OUTDIR}/score1_imbalances_spearman.phyloxml" 704758   ||  return  1
testfile  "${OUTDIR}/score1_imbalances_spearman.svg"      256769   ||  return  1
testfile  "${OUTDIR}/score1_masses_pearson.newick"        6810     ||  return  1
testfile  "${OUTDIR}/score1_masses_pearson.nexus"         20365    ||  return  1
testfile  "${OUTDIR}/score1_masses_pearson.phyloxml"      704501   ||  return  1
testfile  "${OUTDIR}/score1_masses_pearson.svg"           256771   ||  return  1
testfile  "${OUTDIR}/score1_masses_spearman.newick"       6810     ||  return  1
testfile  "${OUTDIR}/score1_masses_spearman.nexus"        20365    ||  return  1
testfile  "${OUTDIR}/score1_masses_spearman.phyloxml"     704496   ||  return  1
testfile  "${OUTDIR}/score1_masses_spearman.svg"          256771   ||  return  1
testfile  "${OUTDIR}/score2_imbalances_pearson.newick"    6810     ||  return  1
testfile  "${OUTDIR}/score2_imbalances_pearson.nexus"     20365    ||  return  1
testfile  "${OUTDIR}/score2_imbalances_pearson.phyloxml"  704765   ||  return  1
testfile  "${OUTDIR}/score2_imbalances_pearson.svg"       256769   ||  return  1
testfile  "${OUTDIR}/score2_imbalances_spearman.newick"   6810     ||  return  1
testfile  "${OUTDIR}/score2_imbalances_spearman.nexus"    20365    ||  return  1
testfile  "${OUTDIR}/score2_imbalances_spearman.phyloxml" 704767   ||  return  1
testfile  "${OUTDIR}/score2_imbalances_spearman.svg"      256771   ||  return  1
testfile  "${OUTDIR}/score2_masses_pearson.newick"        6810     ||  return  1
testfile  "${OUTDIR}/score2_masses_pearson.nexus"         20365    ||  return  1
testfile  "${OUTDIR}/score2_masses_pearson.phyloxml"      704609   ||  return  1
testfile  "${OUTDIR}/score2_masses_pearson.svg"           256769   ||  return  1
testfile  "${OUTDIR}/score2_masses_spearman.newick"       6810     ||  return  1
testfile  "${OUTDIR}/score2_masses_spearman.nexus"        20365    ||  return  1
testfile  "${OUTDIR}/score2_masses_spearman.phyloxml"     704509   ||  return  1
testfile  "${OUTDIR}/score2_masses_spearman.svg"          256771   ||  return  1
