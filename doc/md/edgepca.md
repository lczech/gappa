## Description

Performs [Edge PCA](http://journals.plos.org/plosone/article?id=10.1371/journal.pone.0056859). The command is a re-implementation of [`guppy epca`](http://matsen.github.io/pplacer/generated_rst/guppy_epca.html), see there for more details.

## Details

Edge PCA is an analysis method for phylogenetic placement data that reveals consistent differences between samples (`jplace` files). It uses the imbalance of placements across the edges of tree, which allows to find differences in placements that may be close in the tree.

The command produces two tables that contain the result of the analysis. The `projection` contains the `jplace` samples projected into principal coordinate space, and the `transformation` lists the top eigenvalues (first column) and their corresponding eigenvectors (remaining columns).

The principal components projection of the samples can be plotted and for example colored according to some per-sample metadata feature, in order to reveal dependencies between the placements of a samples and its metadata:

![First two Edge PCA components projected.](https://github.com/lczech/gappa/blob/master/doc/png/analyze_edgepca_plot.png?raw=true)

Furthermore, if the `--write-...-tree` options are used, the principal components are visualized on the tree:

![First two Edge PCA component trees.](https://github.com/lczech/gappa/blob/master/doc/png/analyze_edgepca_trees.png?raw=true)

These trees allow to interpret how the plot above separates samples; that is, they show which edges contribute most to distinguish samples from each other.

<!--
## Citation

When using this method, please do not forget to cite

> Matsen FA, Evans SN (2013),
> **"Edge Principal Components and Squash Clustering: Using the Special Structure of Phylogenetic Placement Data for Sample Comparison."**,
> *PLOS ONE 8(3): e56859*, doi:[10.1371/journal.pone.0056859](http://journals.plos.org/plosone/article?id=10.1371/journal.pone.0056859)
-->
