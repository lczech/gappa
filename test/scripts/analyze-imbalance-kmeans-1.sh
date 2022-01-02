#!/bin/bash

${GAPPA} analyze imbalance-kmeans \
    --jplace-path "data/jplace" \
    --k "1-5" \
    --write-overview-file \
    --write-svg-tree \
    --svg-tree-stroke-width 8.0 \
    --svg-tree-ladderize \
    --out-dir ${OUTDIR}

    # --write-newick-tree \
    # --write-nexus-tree \
    # --write-phyloxml-tree \

testfile  "${OUTDIR}/ikmeans_k_1_assignments.csv"           645 ||  return  1
testfile  "${OUTDIR}/ikmeans_k_1_centroid_0.svg"  256628 257013 ||  return  1
testfile  "${OUTDIR}/ikmeans_k_2_assignments.csv"    662    665 ||  return  1
testfile  "${OUTDIR}/ikmeans_k_2_centroid_0.svg"  256628 257013 ||  return  1
testfile  "${OUTDIR}/ikmeans_k_2_centroid_1.svg"  256628 257013 ||  return  1
testfile  "${OUTDIR}/ikmeans_k_3_assignments.csv"           692 ||  return  1
testfile  "${OUTDIR}/ikmeans_k_3_centroid_0.svg"  256628 257013 ||  return  1
testfile  "${OUTDIR}/ikmeans_k_3_centroid_1.svg"         256630 ||  return  1
testfile  "${OUTDIR}/ikmeans_k_3_centroid_2.svg"  256628 257013 ||  return  1
testfile  "${OUTDIR}/ikmeans_k_4_assignments.csv"    693    696 ||  return  1
testfile  "${OUTDIR}/ikmeans_k_4_centroid_0.svg"         256626 ||  return  1
testfile  "${OUTDIR}/ikmeans_k_4_centroid_1.svg"  256628 257013 ||  return  1
testfile  "${OUTDIR}/ikmeans_k_4_centroid_2.svg"         256622 ||  return  1
testfile  "${OUTDIR}/ikmeans_k_4_centroid_3.svg"         256613 ||  return  1
testfile  "${OUTDIR}/ikmeans_k_5_assignments.csv"           685 ||  return  1
testfile  "${OUTDIR}/ikmeans_k_5_centroid_0.svg"  256628 257013 ||  return  1
testfile  "${OUTDIR}/ikmeans_k_5_centroid_1.svg"         257794 ||  return  1
testfile  "${OUTDIR}/ikmeans_k_5_centroid_2.svg"         256626 ||  return  1
testfile  "${OUTDIR}/ikmeans_k_5_centroid_3.svg"         256613 ||  return  1
testfile  "${OUTDIR}/ikmeans_k_5_centroid_4.svg"         256236 ||  return  1
testfile  "${OUTDIR}/ikmeans_overview.csv"                  117 ||  return  1
