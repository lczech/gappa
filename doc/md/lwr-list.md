## Description

The command is a very simply tool to extract the likelihood weight ratios (LWRs)
of all pqueries in a set of `jplace` samples.
It turns the input into table formats to be used for further analysis,
with each pquery listed in a row.

## Details

The resulting comma-separated `lwr-list.csv` table contains the LWRs for each pquery of each sample.
The table contains several columns:
Sample name (using the input base file name), pquery name (one line for each name for pqueries with
multiple names), the weight (multiplicity) of the pquery, and the first LWR values of that pquery.
The number of values/columns being printed is controlled via `--num-lwrs`.

If the `--num-lwrs` value is set to 0, *all* LWRs per pquery are printed.
As each pquery can have a different number of LWR values stored in the input file,
this might result in a list that is no longer a table where each row has the same number of columns.
