## Description

Calculates the expected distance between placement locations (EDPL) for all pqueries in the given
samples.
The command is a re-implementation of [`guppy edpl`](http://matsen.github.io/pplacer/generated_rst/guppy_edpl.html), see there for more details.

## Details

The command produces two tables:

 * `list.csv`: A list of the EDPL for each pquery of each sample. The list contains three columns:
   Sample name (using the input file name), pquery name (one line for each name for pqueries with
   multiple names), and the EDPL value of that pquery.
 * `histogram.csv`: A summary histogram of the EDPL values. This can be used in spreadsheet
   tools to produce a graph that allows an overview of the values for easy assessment.

Using the settings `--histogram-bins` and `--histogram-max`, the histogram output can be refined.

## Citation

When using this method, please do not forget to cite

> F. A. Matsen, R. B. Kodner, and E. V. Armbrust,
> **"pplacer: linear time maximum-likelihood and Bayesian phylogenetic placement of sequences onto a fixed reference tree."**,
> *BMC Bioinformatics*, vol. 11, no. 1, p. 538, 2010,
> doi: [https://doi.org/10.1186/1471-2105-11-538](https://bmcbioinformatics.biomedcentral.com/articles/10.1186/1471-2105-11-538)
