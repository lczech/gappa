## Description

Print histograms of the likelihood weight ratios (LWRs) of all pqueries.

## Details

The command produces a histogram table that contains histograms of the `--num-lwrs` most likely
likelihood weight ratios (LWRs) of the pqueries (query sequences) in the input `jplace` files.
This can be used in spreadsheet tools to produce a graph that allows an overview of the LWRs
for easy assessment. The histograms can for example be visualized as follows:

![Example of the LWR histograms for the first three most likely LWRs.](https://github.com/lczech/gappa/blob/master/doc/png/lwr_histogram.png?raw=true)

The histogram shows the percentage of the first, second, and third most likely placement.
The x-axis are likelihood weight ratios (always in the range 0.0 to 1.0), the y-axis shows how
many of the query sequences have their first, second and third most likely placement
at that LWR value.
For example, the highest bin indicates that almost 50% of the query sequences have a first
(most likely) placement position at or above an LWR of 0.96. That is, these placements have a high
LWR and are hence placed with high certainty at their respective branches.

Note that the second most likely placement can never have a probability of more than 50%
(otherwise, it would be the most likely one), the third most likely not more than 33%, and so forth.

These histograms can also be interpreted as the distributions of the LWRs.
See Supplementary Figure 14 of [1] for an example of these histograms in practice.

> [1] F. Mahé, C. de Vargas, D. Bass, L. Czech, A. Stamatakis, *et.al.*,
> **"Parasites dominate hyperdiverse soil protist communities in Neotropical rainforests,"**
> *Nature Ecology and Evolution*, vol. 1, no. 4, p. 0091, Mar. 2017.
> doi: [https://doi.org/10.1038/s41559-017-0091](https://www.nature.com/articles/s41559-017-0091)
