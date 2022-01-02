#!/bin/bash

${GAPPA} analyze edgepca \
    --jplace-path "data/jplace" \
    --write-newick-tree \
    --write-nexus-tree \
    --write-phyloxml-tree \
    --write-svg-tree \
    --svg-tree-stroke-width 8.0 \
    --svg-tree-ladderize \
    --out-dir ${OUTDIR}

testfile  "${OUTDIR}/projection.csv"      1805     ||  return  1
testfile  "${OUTDIR}/transformation.csv"  16211    ||  return  1
testfile  "${OUTDIR}/tree_0.newick"       6810     ||  return  1
testfile  "${OUTDIR}/tree_0.nexus"        20365    ||  return  1
testfile  "${OUTDIR}/tree_0.phyloxml"     704971   ||  return  1
testfile  "${OUTDIR}/tree_0.svg"          257974   ||  return  1
testfile  "${OUTDIR}/tree_1.newick"       6810     ||  return  1
testfile  "${OUTDIR}/tree_1.nexus"        20365    ||  return  1
testfile  "${OUTDIR}/tree_1.phyloxml"     704985   ||  return  1
testfile  "${OUTDIR}/tree_1.svg"          257599   ||  return  1
testfile  "${OUTDIR}/tree_2.newick"       6810     ||  return  1
testfile  "${OUTDIR}/tree_2.nexus"        20365    ||  return  1
testfile  "${OUTDIR}/tree_2.phyloxml"     704998   ||  return  1
testfile  "${OUTDIR}/tree_2.svg"          257603   ||  return  1
testfile  "${OUTDIR}/tree_3.newick"       6810     ||  return  1
testfile  "${OUTDIR}/tree_3.nexus"        20365    ||  return  1
testfile  "${OUTDIR}/tree_3.phyloxml"     704977   ||  return  1
testfile  "${OUTDIR}/tree_3.svg"          257979   ||  return  1
testfile  "${OUTDIR}/tree_4.newick"       6810     ||  return  1
testfile  "${OUTDIR}/tree_4.nexus"        20365    ||  return  1
testfile  "${OUTDIR}/tree_4.phyloxml"     705003   ||  return  1
testfile  "${OUTDIR}/tree_4.svg"          256805   ||  return  1
