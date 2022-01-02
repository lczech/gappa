#!/bin/bash

# Clean up and prep
mkdir -p data/jplace
rm -r data/jplace/*

# Metadata table prep
echo -e "sample\tscore1\tscore2" > data/meta.csv

for s in 0 1 2 ; do
    echo "Subtree $s"

    for i in `seq 0 9`; do
        # Make random placements
        ../bin/gappa simulate random-placements \
        --reference-tree data/random-tree.newick \
        --pquery-count 1000 \
        --subtree ${s} \
        --out-dir data/jplace \
        --compress \
        > /dev/null
        mv data/jplace/random-placements.jplace.gz data/jplace/sample_${s}_${i}.jplace.gz

        # Create random metadata
        num1=$(( $RANDOM % 20 + 50 * $s - 60 ))
        num2=$(( $RANDOM % 20 + 10 * $s - 20 ))
        echo -e "sample_${s}_${i}\t${num1}\t${num2}" >> data/meta.csv
    done
done
