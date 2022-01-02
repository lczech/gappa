## Description

The command offers filtering for two aspects of jplace files:

  1. The placement locations, that is, individual placements within a pquery.
  2. The pquery names.

See below for details on each.
All filtering options can also be combined, and are executed on after another in the order that
is listed above; the order in which they are provided on the command line is not taken into account.

Note that when providing multiple jplace files as input (or a directory containing multiple jplace
files), the input files are merged prior to filtering. If instead filtering shall be applied
per file, simply call this command individually for each file.

## 1. Filtering by placement location

Each pquery (i.e., a placed sequence) can contain multiple placement locations, that is,
placements on multiple branches of the reference tree. Each of these locations is typically
annotated with a Likelihood Weight Ratio (LWR), which can be interpreted as a probability that the
placement on this branch is correct.
The placement locations can be filtered by their LWR, using several filtering options.

In theory, for a given pquery, the sum of all LWRs across all branches is 1.
However, to save storage, not all placement locations might be stored in a jplace file,
in which case the sum is lower than 1.
In order to have the remaining locations to sum to 1 again, we offer settings
for normalizing (proportially re-scale) the LWRs:

![Normalization of LWRs.](https://github.com/lczech/gappa/blob/master/doc/png/normalize.png?raw=true)

This can be applied before and/or after the filtering. Applying the normalization before of course
can change the effect of the thresholds provided for filtering, as the LWRs can change.
Applying it after is a simple way to ensure unit probability of each pquery;
this however hides the fact that not all probability mass is still represented
by the remaining placement locations.

## 2. Filtering by pquery names

Each sequence that is placed on the reference tree is stored in the jplace file using a name,
typically the name from the input fasta file used with the placement program.
This name can also be used for filtering, either as a list of names to keep or remove,
or as a [regular expression](https://en.wikipedia.org/wiki/Regular_expression) to match
all names to keep or remove.
