## Description

The command cleans a tree in Newick format (and some of its extensions) by removing parts that might lead some downstream parsers to fail.

The Newick file format for phylogenetic trees in its original standard only supports node names (taxa names) and branch lengths. Over the years, many ad-hoc and custom extensions have been suggested and used in practice, to compensate for missing flexibility of the format.
This however lead to many downstream parsers not being able to work with all those dialects of the format, see

> A Critical Review on the Use of Support Values in Tree Viewers and Bioinformatics Toolkits.<br />
> Czech L, Huerta-Cepas J, Stamatakis A.<br />
> Molecular Biology and Evolution, 17(4), 2017.<br />
> https://doi.org/10.1093/molbev/msx055

for some of the issues that might arise.

This command can be used to clean some of those difficult extensions/annotations, by simply removing them. It is meant as a cleaning tool for other software packages that cannot read a given Newick tree. When all options are activated, all types of extra data (that we know of) are removed, leading to a tree with just node names at the terminal (leaf) nodes, and branch lengths. Note that branch lengths might slightly change even if nothing is removed, due to numerical rounding.
