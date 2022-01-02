## Description

The command runs [Placement-Factorization](https://doi.org/10.1371/journal.pone.0217050)
on a set of `jplace` input samples.

## Details

### Input Meta-Data

The input meta-data table needs to be a character-separated table file, with a header line
that uniquely names the meta-data features, and a first column that contains the sample names.
These sample names are simply the names of the input `jplace` files, without the `.jplace` extension.
The name of the first column is irrelevant.

Example:

    specimen,amsel,nugent,pH
    p4z2r18,1,10,0
    p4z2r38,1,8,5.3
    p4z2r15,0,0,4.4
    p4z2r39,0,0,4.4
    p4z1r18,0,0,4

This table describes three meta-data features (`amsel`, `nugent`, and `pH`)
for five samples (`p4z2r18`, `p4z2r38`, ...).

The values of the columns can be numerical, boolean, or categorical:

 * Numerical values are simply numbers (e.g., `42`, `3.14`, or `10e10`).
 * Boolean values are `true`/`false` values, which are also called binary values in statistical
   analyses. They can be given in several formats: `true`/`false`, `1`/`0`, `on`/`off`, or `yes`/`no`.
 * Categorical values is everything else, for example strings,
   and can be used to describe body sites, locations, or the like.
   Such data is transformed into a set of dummy variables (called "factors" in R)
   in order to work with Generalized Linear Models.

The command only supports to use the meta-data features directly as provided.
That is, transformations or interactions, which are often used in analyses with Generalized Linear Models,
are not directly supported.
It would be cumbersome to offer such procedures via a command line.
Hence, if these are needed, such procedures have to be applied beforehand to the data,
and directly put into the table.

### Output Files

The output is written to several files:

 * `factor_balances.csv`: A list of the balance per samples for all factors.
 * `factor_taxa.txt`: A list of the taxa being split in each factor.
 * `factors_tree`: A tree showing the clades that are split by each factor.
 * `factor_edges_n`: More detailled trees for each factor, showing the taxa being split.
 * `objective_values_n`: A visualization of the objective function values of each branch for each factor.

All output tree files use the "Tree Output" and "Svg Tree Output" options.
Note that at least one of the `--write-...-tree` settings has to be set in order to write tree files
in the respective format.
