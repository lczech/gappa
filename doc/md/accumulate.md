## Description

The command is useful to assess placements that are distributed across (nearby) branches of the reference tree - for example, if the reference tree contains multiple representatives for the same species. It accumulates the placement mass (likelihood weight ratio) of the placements of each pquery upwards the tree (towards the root), until the accumulated mass at a basal branch reaches the given `--threshold`:

![Accumulation of placement mass towards a basal branch.](https://github.com/lczech/gappa/blob/master/doc/png/accumulate.png?raw=true)

That is, each pquery is treated separately. Its mass is first normalized to a total of 1.0. Then, the command looks for the basal branch whose underlying clade accumulates more than the threshold mass. This can be understood as finding the clade that contains most of the placement mass. All placements of the pquery are then removed, and only one placement at the basal branch is added, with a mass of 1.0, which hence represents the accumulated original masses. The pendant length of the resulting pquery is set to the weighted average of the pendant lengths that have been accumulated in the clade, using the masses (likelihood weight ratios) as weights.

It can happen that a pquery contains placement mass across different sides of the root. If no side contains more than the given `--threshold` mass, there is no basal branch or clade that satisfies the above description. In that case, the whole pquery is removed from the output, and its name(s) are printed in order to inform about this. This can for example happen with chimeric sequences that fit in multiple places of the tree, and hence should be treated as a warning sign. Another reason can be that the root of reference tree is not chosen properly. In that case, it can help to reroot the tree first.

The output of the command is a file called `accumulated.jplace`, potentially using the `--file-prefix`.
