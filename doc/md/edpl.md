## Description

Calculates the expected distance between placement locations (EDPL) for all pqueries in the given samples.
The command is a re-implementation of [`guppy edpl`](http://matsen.github.io/pplacer/generated_rst/guppy_edpl.html),
see there for more details.

## Details

The EDPL is a measure of uncertainty of how far the placements of a pquery (query sequence) are
spread across the branches of the reference tree. In a reference tree with similar sequences,
a query sequence might be placed on several nearby branches with relatively high likelihood (LWR).
This still constitutes a high confidence in the placement, as the spreading is due to the similar
reference sequences, and not due to inherent uncertainty in the placement itself. This is opposed
to a query sequence whose placements are spread all across the tree, which might indicate that a
fitting reference sequence is missing from the tree, and hence yields uncertain placements.

This can be assessed with the EDPL, which calculates the distances between different placements, weighted by their respective LWRs:

![Example of the EDPL for a pquery with three placement locations.](https://github.com/lczech/gappa/blob/master/doc/png/edpl.png?raw=true)

The `p` values in the figure represent likelihood weight ratios of the placements at these locations.
The distances `d` are calculated using the branch lengths of the tree on the path between the
placement locations. Hence, a low EDPL indicates that the placements of a pquery (query sequence)
are focused in a narrow region of the tree, whereas a high EDPL indicates that the placements are
spread across the tree.

See http://matsen.github.io/pplacer/generated_rst/guppy_edpl.html for more information.

The command produces two tables:

 * `list.csv`: A list of the EDPL for each pquery of each sample. The list contains four columns:
   Sample name (using the input file name), pquery name (one line for each name for pqueries with
   multiple names), the weight (multiplicity) of the pquery, and the EDPL value of that pquery.
   As this list needs quite some memory (about as much as the input jplace files),
   it can also be deactivated with `--no-list-file`.
 * `histogram.csv`: A summary histogram of the EDPL values. This can be used in spreadsheet
   tools to produce a graph that allows an overview of the values for easy assessment.
   Using the settings `--histogram-bins` and `--histogram-max`, the histogram output can be refined.

The histogram can for example be visualized as follows:

![Example of an EDPL histogram.](https://github.com/lczech/gappa/blob/master/doc/png/edpl_histogram.png?raw=true)

The histogram shows the accumulated EDPL values: The x-axis are EDPLs, the y-axis shows how
many of the query sequences have an EDPL at or below the respecive value.
For example, the lowest bin indicates that more than 60% of the query sequences have an EDPL
between 0.0 and 0.02.

## Citation

When using this method, please do not forget to cite

> F. A. Matsen, R. B. Kodner, and E. V. Armbrust,
> **"pplacer: linear time maximum-likelihood and Bayesian phylogenetic placement of sequences onto a fixed reference tree."**,
> *BMC Bioinformatics*, vol. 11, no. 1, p. 538, 2010,
> doi: [https://doi.org/10.1186/1471-2105-11-538](https://bmcbioinformatics.biomedcentral.com/articles/10.1186/1471-2105-11-538)
