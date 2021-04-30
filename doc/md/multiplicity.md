## Description

The command edits the multiplicities of `jplace` files and sets them to values given as input.
The command takes one or more `jplace` files as input, as well as an input that lists the new
multiplicities for each `pquery` in the `jplace` files. There are two ways of input for the new
multiplicities:

 * `--multiplicity-file`: A simple tab-separated list for each pquery.
 * `--fasta-path`: A set of `fasta` files, from which the header information is used.

See below for the expected format for each.
A file that can be used for the first way can be produced with the `--write-multiplicity-file`
flag, as explained below.

## Details

As defined in the specification of
[the jplace standard](https://journals.plos.org/plosone/article?id=10.1371/journal.pone.0031009),
each query in a `jplace` file can have multiple names associated with it. This is for example
useful if there are duplicate sequences in the data, but which have different names in the original
`fasta` file: If the sequences are identical, so will be their placements. It thus makes sense
to summarize the placement positions, and store the list of names for these duplicates, instead
of repeating all placements for every name again and again.

Furthermore, each such name can have a so called *multiplicity*, which can be understood as
a form of weight for the name. This is for example useful if duplicate sequences in the original
data also share the same name (e.g., the hash of the sequence). In this case, not only their
placements are identical - so are their name. In order to not lose track of how often the sequence
appeared in the original data, its multiplicity can be set accordingly in the `jplace` file.

The command edits the multiplicity for pqueries by setting them to given values.
No other data of the input `jplace` files is changed. The files are *not* edited in place,
but new files are written to the `--out-dir`, potentially prefixed by `--file-prefix` and
`--file-suffix`.

### `--multiplicity-file`

The simplest way to provide new multiplicities is via a list.
This tab-separated list file can be given in two formats: with two columns, or with three columns.

Two columns are interpreted as "pquery name" and "new multiplicity". This also works when multiple
`jplace` files are provided - but in this case, it might be better to use the three-column format,
in order to avoid accidental duplicates.

Three columns are interpreted as "sample name", "pquery name", and "new multiplicity".
The sample name is the file name of the `jplace` file without the `.jplace` extension:

    p1z1r2	FUM0LCO01BV7G2	24
    p1z1r2	FUM0LCO01DOIHD	31
    p1z1r2	FUM0LCO01CKWR0	5
    ...

Entries in the table can be wrapped in double quotation marks (`"..."`) if they
contain tab characters themselves. If duplicates occur, a warning is printed, and the last
multiplicity value for a given pquery name is used. The provided multiplicities can be floating
point numbers (e.g., `3.14`).

### `--fasta-path`

In many pre-processing pipelines, identical sequences are deduplicated prior to analyses to reduce
overhead. See for example [vsearch](https://github.com/torognes/vsearch) for a tool to achieve this.
Such tools can annotate the resulting reduced files in order to keep track of the original number
of identical sequences (their "abundance"). One popular way is to annotate the sequence label
in its `fasta` file like this:

    >FUM0LCO01BV7G2;size=24;
    ACGT
    >FUM0LCO01DOIHD;size=31;
    GATACA
    >FUM0LCO01CKWR0;size=5;
    CATTAG
    ...

This information can be used here to set multiplicities. The command expects the base name of the
`fasta` files (that is, without the `.fasta` or `.fasta.gz` extension) to be identical to the base
name of the corresponding `.jplace` (or `.jplace.gz`) file, in order to know which multiplicities
to use for which sample.

The following annotation formats are supported:

 * Via the `>name;size=123;` annotation.
 * Via the `>name;weight=3.14;` annotation.
 * Via underscore at the end of the label: `>name_123`

The first and the last option are common annotations, see [swarm](https://github.com/torognes/swarm)
for a popular OTU clustering tool that supports both of them. They expect integer numbers.
In order to also support floating point numbers, we additionally allow to use the `weight` annotation,
as shown above. Note that if both `size` and `weight` are provided, they are multiplied to get
the final multiplicity for the pquery.

By default, the pquery name is assumed to be just the first part of the `fasta` label,
that is, the above annotations (and, if present, other semicolon-separated attributes) are removed.
However, typical placement programs do not remove this information, but rather name the pquery
using the full `fasta` label. Hence, the pquery name in a `jplace` file might be
`FUM0LCO01BV7G2;size=24;`. In order to use this full label for finding pqueries,
set the `--keep-full-label` flag.

### `--write-multiplicity-file`

If set, a file listing the current multiplicities of the pqueries in the input `jplace` files
is written. That is, no new `jplace` files are produced.
The values in the file can then be changed as needed, and the file can be used as input to
`--multiplicity-file` for actually changing the multiplicities in the `jplace` files.
The file always uses the three columns format as explained above;
the file is named `multiplicities.csv`, potentially prefixed by `--file-prefix` and `--file-suffix`.

## See Also

Our pipeline for reducing overhead and increasing load balances when placing a large number
of samples also uses multiplicities to keep track how often a sequences appears in the data.
See the [chunkify](../wiki/Subcommand:-chunkify) and [unchunkify](../wiki/Subcommand:-unchunkify)
commands for details. They automatically change the multiplicity as needed, so it is not necessary
to run this command for them.
