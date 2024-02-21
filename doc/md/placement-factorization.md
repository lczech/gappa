## Description

The command runs [Placement-Factorization](https://doi.org/10.1371/journal.pone.0217050)
on a set of `jplace` input samples.

Placement-Factorization is an adaptation of [PhyloFactorization](https://doi.org/10.1002/ecm.1353) to phylogenetic placement data. It iteratively finds edges in the tree across which the abundances in the samples exhibit a strong relationship with given meta-data (such as environmental variables).

The command can be understood as an extension of the [correlation](../wiki/Subcommand:-correlation) command. There, we compute a simple correlation between per-edge or per-clade abundance measures and one meta-data feature. Here, we also find nested relationships (by breaking down the tree into smaller clades in each iteration), and we allow to use multiple meta-data features at once (by using a GLM instead of a simple correlation coefficient).

For the publication describing the method, see [here](https://doi.org/10.1371/journal.pone.0217050). However, for a thorough step-by-step introduction to the method, we recommend [this PhD Thesis](https://doi.org/10.5445/IR/1000105237), see Chapters 6 and 7 there.

## Details

### Input Meta-Data

The input meta-data table (`--metadata-table-file`) needs to be a character-separated table file,
with a header line that uniquely names the meta-data features, and a first column that contains
the sample names.
These sample names are simply the names of the input `jplace` files, without the `.jplace[.gz]`
extension. The name of the first column is irrelevant.

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
That is, transformations or interactions, which are often used in analyses with Generalized Linear Models, are not directly supported. It would be tricky to offer such procedures via a command line.
Hence, if these are needed, any transformation or interaction terms have to be applied beforehand to the data, and directly put into the table as additional columns. For instance, for a given column `X`,
add a column `X^2` with all squared values of the `X` column, so that the GLM can use the squares
as a predictor as well. By examining the GLM coefficients later (see below), one can then disentangle these terms again.

### Algorithm

For details, we recommend to read the [article](https://doi.org/10.1371/journal.pone.0217050) or even better the [PhD Thesis](https://doi.org/10.5445/IR/1000105237) describing the method. In brief:

Placement-Factorization takes as input a set of placement samples (jplace files), where each sample captures the abundances of species at a location or point in time. Further, it takes a meta-data table as input, with features for instance describing environmental variables for each sample, such as pH of the soil, depth of the water, etc, where each sample was taken.

The input samples are transformed into per-edge balances, which is a measure of contrast between the abundances (total sum of LWRs) in the two parts of the tree that are split by the edge. In other words, the balance across an edge indicates which of the two sub-trees on either side of the edge has a higher abundance (measured by the geometric mean of edge masses).

Then, the algorithm fits a Generalized Linear Model (GLM) at each edge of the tree, by using the meta-data features as predictors, and the balance at the edge as the response (predicted) variable. For each GLM (i.e., at each edge), the goodness of the model fit is measured, which we call the "objective value" of the factorization. More specifically, we compute the model fit as the difference between the null deviance (model fitted with just an intercept) and the deviation of the actual full model - how much better the model with predictors (meta-data) is at explaining the variance in the response variable (balance) compared to the null model (intercept only).

![Schmeme of the first two iterations of Placement-Factorization.](https://github.com/lczech/gappa/blob/master/doc/png/pf-algorithm.png?raw=true)

The edge where this fit is the highest is declared the "winner": It is the edge that exhibits the strongest relationship between abundances (or rather, their balances) and the meta-data. In the figure, this is edge e_1, which maximizes the objective function (the GLM model fit). This indicates that this edge is an important factor with respect to the meta-data features, and can be interpreted similarly to an edge that has a high [correlation](../wiki/Subcommand:-correlation) with some meta-data feature.

Next, the tree is split into two parts by the winning edge. This yields two smaller trees. The algorithm then moves to the second iteration, and repeats the above process, but only within each of those subtrees, as shown in the figure. This allows to find nested factors: By splitting the tree and considering the two parts separately, we essentially "factored out" the effect of the first edge. Each subsequent iteration hence splits one of the subtrees resulting from a previous iteration again.

The algorithm typically should terminate after a certain number of factors. At some point, the relationship with meta-data is expected to drop of, and no more significant edges can be found. This is for instance indicated by the lower objective values after some iterations.

### Output Files

The output is written to several files:

 * `objective_values_n`: A visualization of the objective function values of each branch for each factor.
 * `factors_tree`: A tree showing the clades that are split by each factor.
 * `factor_edges_n`: More detailled trees for each factor, showing the taxa being split.
 * `factor_taxa.csv`: A list of the taxa being split in each factor.
 * `factor_balances.csv`: A list of the balance of the winning edge per samples for all factors.
 * `factor_glm_coefficients.csv`: A list of the GLM coefficients at the winning edge per factor.

All output tree files use the "Tree Output" and "Svg Tree Output" options.
Note that at least one of the `--write-...-tree` settings has to be set in order to write tree files
in the respective format.

#### Objective Values

These `objective_values_n` tree files visualize the value of the objective function (the model fit of the GLM) per edge of the tree, for each factor. For instance, for the first two factors, the trees might look like this:

![Example of the trees visualizing the objective values for the first to factors.](https://github.com/lczech/gappa/blob/master/doc/png/pf-objective-values.png?raw=true)

The darker a branch, the better the GLM was able to predict balances from the meta-data. That means, these abundances across these edges exhibit a strong relationship with the meta-data. The winning edge (the edge with the best model fit, i.e., the strongest relationshop) of each iteration is marked by a black arrow, and the resulting clade is marked with a black arc.

#### Factors Tree, Factor Edges, Factor Taxa

Each iteration of the algorithm splits away another clade of the tree, at the winning edge. This indicates that the abundances in the clade exhibit a strong relationship with the meta-data. Across iterations, this leads to more and more clades being split. The `factors_tree` shows these clades:

![Example of the factors tree that shows the clades split by each factor.](https://github.com/lczech/gappa/blob/master/doc/png/pf-factors-tree.png?raw=true)

The `factor_edges_n` trees provide some more detail about these clades, visualizing the edge where the split happens, as well as the two resulting clades:

![Example of the factor edges trees that shows the clades in more details.](https://github.com/lczech/gappa/blob/master/doc/png/pf-factor-edges.png?raw=true)

Any clade that had been split in a previous iteration does not partake in that split any more, and is hence grayed out in these trees.

Lastly, the `factor_taxa.csv` table contains the taxa (names of the tree tips) for each factor, indicating whether the taxon is on the root side or non-root side of the split. This can be useful to downstream process the splits.

#### Factor Balances

The `factor_balances.csv` table contains the balances of the winning edge of each iteration. This is the data of the winning edge that the GLM was fitted to using the meta-data as predictors. These balances can be interpreted as an ordination of the data, where each factor is one axis:

![Example of an ordination using the balances aross the first two factors.](https://github.com/lczech/gappa/blob/master/doc/png/pf-balances-ordination.png?raw=true)

The figure shows the ordination visualization plots of the balances of the first two factors, (a) with and (b) without taxon weighting (`--taxon-weight-tendency	none --taxon-weight-norm none`). That is, the axes correspond to the splits induced by the first two factors, while values along the axes are the balances of each sample calculated on the sets of edges of each split - simply a scatter plot of the first two columns of the `factor_balances.csv` table. Samples are colored by one of the meta-data features used, for visualization purposes.

For details on and effects of taxon weighting, see the original [article](https://doi.org/10.1371/journal.pone.0217050) or better Chapter 6 of the [PhD Thesis](https://doi.org/10.5445/IR/1000105237) describing them. In short, the default taxon weighting scheme downweights the influence of low abundant taxa, which can have spurious data and more noise. We recommend to run the algorithm at least once with the the defaults and once without taxon weighting, to see the effects.

#### GLM Coefficients

Lastly, the `factor_glm_coefficients.csv` table provides the coefficients of the GLM of the winning edge for each iteration (each factor). These are the coefficients estimated by the GLM that maximize the fit from the meta-data to the balance of the edge. In combination with the `factor_balances.csv` table from above, these values allow for instance to recreate the fit of the model for that edge:

![Example of the GLM coefficients plotted against the balances.](https://github.com/lczech/gappa/blob/master/doc/png/pf-glm-coeffs.png?raw=true)

We provide an [R script](https://github.com/lczech/gappa/blob/master/scripts/plot-pf-glm-coeffs.R) to visualize these fits. We currently only allow the Gaussian/normal family and identity link function for the GLM. Hence, the model can be simply computed as a linear combination of the meta-data features and the balances.

Examining the coefficients allows to disentangle the effects of each meta-data feature. This also allows to work "backwards" from interaction terms. In the example from above, a feature `X` was amended by a feature `X^2` in the meta-data table to include this term in the GLM as well. The model will then output coefficients for both, allowing to express the model fit as `Intercept + a * X + b * X^2`.

NB: Our underlying implementation supports other links as well, and other objective functions, similar to the [article](https://doi.org/10.1002/ecm.1353) on the underlying idea of PhyloFactorization. However, it would be cumbersome to specify those via a command line interface. If this is relevant to you, please open an [issue](https://github.com/lczech/gappa/issues), or check out the underlying C++ implementation in [genesis](https://github.com/lczech/genesis).
