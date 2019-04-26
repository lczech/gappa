## Description

The command creates a random tree with a given number of leaf nodes (taxa).
The taxa are named with simple letter combinations, going `a, ..., z, aa, ..., az, ba, ...`,
and are randomly distributed over the tree.
The branch lengths are randomly chosen in the unit interval.

The output file is `random-tree.newick`, potentially using the `--file-prefix` if provided.
