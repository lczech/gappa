## Description

The command takes one or more `jplace` files, sorts all their pqueries by their likelihood weight
ratios (LWRs), and prints a summary table representing the distribution of LWRs.

We provide an
[R script to plot this distribution](https://github.com/lczech/gappa/blob/master/scripts/plot-lwr-distribution.R) in a stacked area plot, see below for examples.
This serves as a quality control check of the placement process, to visualize if the pqueries
could be confidently placed on the reference tree.

## Details

The pqueries of the input files are first sorted by their LWR. By default, we use a weighted
sorting, where the most likely placement location (highest LWR) has weight 1,
the second most likely has weight 1/2, the third most likely has weight 1/3, and so forth.
This generally gives a sorting order that is reasonable to inspect visually.
See `--numerical-sort` for an alternative sorting order that focuses more on the most likely
(highest LWR) placement location.

In the sorting process, multiplicities of each pquery are ignored, as we are here interested in a
per-pquery distribution. Pqueries with multiple names are added multiple times to the sorted list.

After the sorting, `--num-entries` many representative pqueries are picked at equidistant positions
in the list, which serve as representatives of the total LWR distribution of all pqueries.
This is the length of the output list; the higher this value, the more detail can be visualized.

The columns of the table contain the sorting `Index` of each representative pquery, its name,
and the LWR entries, sorted from most likely to least likely placement location.
We print `--num-lwrs` of the most likely LWRs, followed by a `Remainder` column that contains
the accumulated sum of all remaining LWRs that are present in the input file for the given pquery.

Using the [R script to plot the resulting table](https://github.com/lczech/gappa/blob/master/scripts/plot-lwr-distribution.R), we get a stacked area plot showing the LWR distribution, as shown in the examples below.

### Exemplary Visualization 1

Using an exemplary dataset, a resulting plot might look like this:

<br>

![Likehood Weight Ratio (LWR) distribution.](https://github.com/lczech/gappa/blob/master/doc/png/lwr-distribution-1-small.png?raw=true)

<br>

Along the x-axis, the sorted (representative) pqueries are listed, with the index denoting
their sorting order. The y-axis shows the stacked (accumulated) LWR at each pquery.
Increasing the number of entries in the output table (`--num-entries`) increases the resolution
along the x-axis of the plot, by including more pqueries.

In the exemplary plot above, the first ~20 pqueries (leftmost part of the plot) have all their LWR
in the most likely placement position (the stacked plot only contains `LWR.1`);
this indicates that these pqueries have been confidently placed on one branch of the referen tree.

Furthermore, about half of the pqueries (left half of the plot) have almost all
their placement mass (LWRs) within the first three most likely placement locations;
in other words, the first three LWRs account for almost all the of the stacked distribution.
This indicates that these pqueries have been placed on the reference tree with some ambiguity
between up to three branches, but are generally well placed.

The right half of the plot shows pqueries that have more of their distribution in the Remainder.
This indicates that their placement is more uncertain.

Note that the plot does not show *how far* the individual placement locations that correspond
to these LWRs are from each other on the reference tree; to this end, metrics such as the
[Expected Distance between Placement Locations (EDPL)](../wiki/Subcommand:-edpl) are better suited.

### Exemplary Visualization 2

Another exemplary dataset might yield this plot:

<br>

![Likehood Weight Ratio (LWR) distribution.](https://github.com/lczech/gappa/blob/master/doc/png/lwr-distribution-2-small.png?raw=true)

<br>

Here, the distribution is much less certain than in the first dataset:
Only few pqueries are certainly placed, and for the majority of the pqueries, the first five
most likely placement positions (five LWRs) do not contain most of the placement distribution;
almost all the distribution is in the Remainder.

As the LWRs per pquery are sorted, this means that the placements in this example are very uncertain.
Note that the second most likely LWR cannot be above 0.5 (as otherwise it would be the most likely);
the thirst most likely not above 1/3 (as otherwise it would be the second most likely); and so forth.
Hence, the Remainder contains the sum of many very small LWR values, indicating a very flat
and uncertain distribution for the pqueries on the right hand side of the plot.

For this particular plot, we used a `jplace` file that includes up to 100 of the most likely
placement locations. Note that typically, the placement algorithm cuts off the output
at a lower number of placement locations, which means that the accumulated LWRs in the file
do not sum up to 1.0 any more.
This can also be seen in the plot above: Even with 100 locations in the input file,
not all of the LWR distribution is accounted for. There is still a white area above
the Remainder that contains the missing LWRs in order to stack up to 1.0.
