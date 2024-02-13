## Description

Performs [Edge PCA](http://journals.plos.org/plosone/article?id=10.1371/journal.pone.0056859). The command is a re-implementation of [`guppy epca`](http://matsen.github.io/pplacer/generated_rst/guppy_epca.html), see there for more details.

## Details

Edge PCA is an analysis method for phylogenetic placement data that reveals consistent differences between samples (`jplace` files). It uses the imbalance of placements across the edges of tree, which allows to find differences in placements that may be close in the tree.

### Output Files

Similar to guppy, the command produces two tables that contain the result of the analysis. The `projection.csv` table contains the `jplace` samples projected into principal coordinate space, and the `transformation.csv` table lists the top eigenvalues (first column) and their corresponding eigenvectors (remaining columns).

Furthermore, we split the `transformation` table here, for post-processing convenience. The `eigenvalues.csv` table just contains the eigenvalues, while the `eigenvectors.csv` contains the eigenvectors across all edges that were used in the PCA computation.

The correspondence of eigenvectors to edges is a bit tricky: Only the inner edges of the tree (the ones not leading to leaf nodes) have a meaningful edge imbalance value (which is the value used for computing the PCA). Furthermore, some inner edges might have a constant imbalance value, for instance, if no sequences had any placement stored in the outer branches of an edge. In that case, we filter out these edges before computing the PCA as well, as they are not meaningful and might lead to numerical issues if retained.

Hence, for the correspondence between the (inner, non-constant imbalance) edges and the eigenvector components of the PCA, we need some extra work. The `edge_indices.newick` tree contains an annotated tree with inner nodes labeled according to the edge index. This edge index is the first column in `eigenvectors.csv`, making it possible to link the two. Note that we label the nodes in that file, and not the edges, as the Newick file format does not support the latter; see [here](https://doi.org/10.1093/molbev/msx055) for this shortcoming of the Newick file format, and resulting issues. You can for example use [Dendroscope](https://doi.org/10.1186/1471-2105-8-460) to examine the newick file, or use some programmatic way.

Furthermore, we produce separate Newick files for each PCA component, named `eigenvector_*.newick`. These are in NHX format, and annotate the components of the eigenvectors onto the edges, using `0.0` for leaf edges and those that were filtered out for the PCA. They can for example be displayed by [iTOL](https://itol.embl.de/); search for NHX in the iTOL help to see how those values can be displayed.

### Plotting and Analysis

The principal components projection of the samples can be plotted and for example colored according to some per-sample metadata feature, in order to reveal dependencies between the placements of a samples and its metadata:

![First two Edge PCA components projected.](https://github.com/lczech/gappa/blob/master/doc/png/analyze_edgepca_plot.png?raw=true)

Furthermore, if the `--write-...-tree` options are used, the principal components are visualized on the tree:

![First two Edge PCA component trees.](https://github.com/lczech/gappa/blob/master/doc/png/analyze_edgepca_trees.png?raw=true)

These trees allow to interpret how the plot above separates samples; that is, they show which edges contribute most to distinguish samples from each other. These trees can also be re-created using the annotated `eigenvector_*.newick`, so that instead of colors, some other way of visualization can be used.
