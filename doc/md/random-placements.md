## Description

The command creates a set of random phylogenetic placmeents on a given reference tree.
It generates as many pqueries as given by `--pquery-count`, named `pquery_n`,
with each having a random number of placements on nearby branches of a randomly chosen edge.

The output file is named `random-placements.jplace`, potentially using the `--file-prefix` and
`--file-suffix` if provided.
