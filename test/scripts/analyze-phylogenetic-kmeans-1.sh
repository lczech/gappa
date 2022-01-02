#!/bin/bash

${GAPPA} analyze phylogenetic-kmeans \
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

testfile  "${OUTDIR}/pkmeans_k_1_assignments.csv"     645    651 ||  return  1
testfile  "${OUTDIR}/pkmeans_k_1_centroid_0.svg"          256250 ||  return  1
testfile  "${OUTDIR}/pkmeans_k_2_assignments.csv"            662 ||  return  1
testfile  "${OUTDIR}/pkmeans_k_2_centroid_0.svg"          256643 ||  return  1
testfile  "${OUTDIR}/pkmeans_k_2_centroid_1.svg"          257038 ||  return  1
testfile  "${OUTDIR}/pkmeans_k_3_assignments.csv"            681 ||  return  1
testfile  "${OUTDIR}/pkmeans_k_3_centroid_0.svg"          257028 ||  return  1
testfile  "${OUTDIR}/pkmeans_k_3_centroid_1.svg"          256637 ||  return  1
testfile  "${OUTDIR}/pkmeans_k_3_centroid_2.svg"          256643 ||  return  1
testfile  "${OUTDIR}/pkmeans_k_4_assignments.csv"            681 ||  return  1
testfile  "${OUTDIR}/pkmeans_k_4_centroid_0.svg"          256627 ||  return  1
testfile  "${OUTDIR}/pkmeans_k_4_centroid_1.svg"          257028 ||  return  1
testfile  "${OUTDIR}/pkmeans_k_4_centroid_2.svg"          256635 ||  return  1
testfile  "${OUTDIR}/pkmeans_k_4_centroid_3.svg"          256640 ||  return  1
testfile  "${OUTDIR}/pkmeans_k_5_assignments.csv"            679 ||  return  1
testfile  "${OUTDIR}/pkmeans_k_5_centroid_0.svg"          256637 ||  return  1
testfile  "${OUTDIR}/pkmeans_k_5_centroid_1.svg"          257040 ||  return  1
testfile  "${OUTDIR}/pkmeans_k_5_centroid_2.svg"          257026 ||  return  1
testfile  "${OUTDIR}/pkmeans_k_5_centroid_3.svg"          257035 ||  return  1
testfile  "${OUTDIR}/pkmeans_k_5_centroid_4.svg"          256644 ||  return  1
testfile  "${OUTDIR}/pkmeans_overview.csv"               111 114 ||  return  1
