#!/bin/bash

${GAPPA} analyze squash \
    --jplace-path "data/jplace" \
    --exponent 1.2 \
    --write-svg-tree \
    --svg-tree-stroke-width 8.0 \
    --svg-tree-ladderize \
    --out-dir ${OUTDIR}

testfile  "${OUTDIR}/cluster.newick"      968 || return 1
testfile  "${OUTDIR}/tree_0.svg"       255852 || return 1
testfile  "${OUTDIR}/tree_1.svg"       257030 || return 1
testfile  "${OUTDIR}/tree_2.svg"       257032 || return 1
testfile  "${OUTDIR}/tree_3.svg"       257037 || return 1
testfile  "${OUTDIR}/tree_4.svg"       257037 || return 1
testfile  "${OUTDIR}/tree_5.svg"       255850 || return 1
testfile  "${OUTDIR}/tree_6.svg"       257030 || return 1
testfile  "${OUTDIR}/tree_7.svg"       257037 || return 1
testfile  "${OUTDIR}/tree_8.svg"       255850 || return 1
testfile  "${OUTDIR}/tree_9.svg"       257036 || return 1
testfile  "${OUTDIR}/tree_10.svg"      255853 || return 1
testfile  "${OUTDIR}/tree_11.svg"      255850 || return 1
testfile  "${OUTDIR}/tree_12.svg"      255851 || return 1
testfile  "${OUTDIR}/tree_13.svg"      255853 || return 1
testfile  "${OUTDIR}/tree_14.svg"      255853 || return 1
testfile  "${OUTDIR}/tree_15.svg"      257433 || return 1
testfile  "${OUTDIR}/tree_16.svg"      255853 || return 1
testfile  "${OUTDIR}/tree_17.svg"      256248 || return 1
testfile  "${OUTDIR}/tree_18.svg"      255850 || return 1
testfile  "${OUTDIR}/tree_19.svg"      256248 || return 1
testfile  "${OUTDIR}/tree_20.svg"      257431 || return 1
testfile  "${OUTDIR}/tree_21.svg"      257433 || return 1
testfile  "${OUTDIR}/tree_22.svg"      257433 || return 1
testfile  "${OUTDIR}/tree_23.svg"      256250 || return 1
testfile  "${OUTDIR}/tree_24.svg"      257423 || return 1
testfile  "${OUTDIR}/tree_25.svg"      257433 || return 1
testfile  "${OUTDIR}/tree_26.svg"      257433 || return 1
testfile  "${OUTDIR}/tree_27.svg"      257433 || return 1
testfile  "${OUTDIR}/tree_28.svg"      257435 || return 1
testfile  "${OUTDIR}/tree_29.svg"      257823 || return 1
testfile  "${OUTDIR}/tree_30.svg"      257032 || return 1
testfile  "${OUTDIR}/tree_31.svg"      257037 || return 1
testfile  "${OUTDIR}/tree_32.svg"      257037 || return 1
testfile  "${OUTDIR}/tree_33.svg"      257035 || return 1
testfile  "${OUTDIR}/tree_34.svg"      256642 || return 1
testfile  "${OUTDIR}/tree_35.svg"      256642 || return 1
testfile  "${OUTDIR}/tree_36.svg"      256642 || return 1
testfile  "${OUTDIR}/tree_37.svg"      256642 || return 1
testfile  "${OUTDIR}/tree_38.svg"      256635 || return 1
testfile  "${OUTDIR}/tree_39.svg"      255853 || return 1
testfile  "${OUTDIR}/tree_40.svg"      257433 || return 1
testfile  "${OUTDIR}/tree_41.svg"      257038 || return 1
testfile  "${OUTDIR}/tree_42.svg"      257036 || return 1
testfile  "${OUTDIR}/tree_43.svg"      257033 || return 1
testfile  "${OUTDIR}/tree_44.svg"      257431 || return 1
testfile  "${OUTDIR}/tree_45.svg"      257426 || return 1
testfile  "${OUTDIR}/tree_46.svg"      257037 || return 1
testfile  "${OUTDIR}/tree_47.svg"      257028 || return 1
testfile  "${OUTDIR}/tree_48.svg"      257038 || return 1
testfile  "${OUTDIR}/tree_49.svg"      257040 || return 1
testfile  "${OUTDIR}/tree_50.svg"      257040 || return 1
testfile  "${OUTDIR}/tree_51.svg"      256643 || return 1
testfile  "${OUTDIR}/tree_52.svg"      257038 || return 1
testfile  "${OUTDIR}/tree_53.svg"      256643 || return 1
testfile  "${OUTDIR}/tree_54.svg"      256629 || return 1
testfile  "${OUTDIR}/tree_55.svg"      256643 || return 1
testfile  "${OUTDIR}/tree_56.svg"      256643 || return 1
testfile  "${OUTDIR}/tree_57.svg"      257034 || return 1
testfile  "${OUTDIR}/tree_58.svg"      256248 || return 1
