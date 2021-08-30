## Description

The command takes one or more `jplace` samples, and prints a histogram showing the
likelihood weight ratios (LWRs) of all pqueries, as shown below.

## Details

The command produces a histogram table that contains the `--num-lwrs` most likely
likelihood weight ratios (LWRs) of the pqueries (query sequences) in the input `jplace` files.
This can be used in spreadsheet tools to produce a graph that allows an overview of the LWRs
for easy assessment and quality control.
The output contains columns for the absolute values (summed multiplicities of all pqueries at
the particular bin), relative values (as percentages), as well as accumulated absolute and relative
values.

We provide an [R script to plot these histograms](https://github.com/lczech/gappa/blob/master/scripts/plot-lwr-histogram.R), which for example visualizes a histogram like this:

![Example of the LWR histograms for the first three most likely LWRs.](https://github.com/lczech/gappa/blob/master/doc/png/lwr-histogram-small.png?raw=true)

The histogram shows the percentage of the first, second, and third most likely placement.
The x-axis are likelihood weight ratios (always in the range 0.0 to 1.0), the y-axis shows how
many of the query sequences have their first, second and third most likely placement
at that LWR value. (More exactly, it shows the summed *multiplicities* of the pqueries.)

For example, the highest bin of `LWR.1` on the right hand side of the plot
indicates that ~20% of the query sequences have a first
(most likely) placement position at or above an LWR of 0.95. That is, these placements have a high
LWR and are hence placed with high certainty at their respective branches.

Note that the second most likely placement can never have a LWR of more than 1/2
(otherwise, it would be the most likely), the third most likely not more than 1/3
(otherwise, it would be the second most likely), and so forth.

See Supplementary Figure 14 of [Mahé *et al.*, *Nature Ecology and Evolution*, 2017](https://doi.org/10.1038/s41559-017-0091)
for an example of these histograms in practice.
