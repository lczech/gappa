## Description

The command takes one or more jplace files as input and visualizes the distribution of placements on the branches of the tree. It uses color coding to show how much placement mass there is per branch.

![Placements visualized by per-branch colors.](https://github.com/lczech/gappa/blob/master/doc/png/analyze_visualize_color.png?raw=true)

The tree shows the distribution of placements across the tree. That is, it sums the LWR per branch for all query sequences that have placement mass on that branch (theoretically, all of them have, but usually, jplace files only store the top `n` most likely locations). Note that this hence does _not_ correspond to an actual numer of sequences per branch - it is a distribution!

However, when using `--point-mass`, each query sequence is reduced to only its most likely placement location, and the LWR of that location is set to 1.0. Hence, when this flag is set, the heat-tree visualization produced here also corresponds to individual query sequences, with the downside of losing the uncertainty information contained in the placement distribution.

**Important remark:**
If multiple jplace files are provided as input, their combined placements are visualized. It is then critical to correctly set the `--mass-norm` option. If set to `absolute`, no normalization is performed per jplace file - thus, absolute abundances are shown. However, if set to `relative`, the placement mass in each input file is normalized to unit mass 1.0 first, thus showing relative abundances.
