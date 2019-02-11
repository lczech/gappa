## Description

The command splits one or more `jplace` files into several new `jplace` files, which each
contain a specified subset of the `pqueries` in the original files.
The required subsets can be given in two ways:

  * `--split-file`: A simple comma-separated table that lists which pquery/read
    should be put in which output file.
  * `--otu-table-file`: A tab-separated OTU table that contains entries for all pqueries
    and all output files.

See below for details on the expected formats.

## Details

The command is typically used to split one `jplace` file into multiple files.
If multiple `jplace` input files are provided, they are simply treated as one large collection
of placed sequences. This necessitates that they all use the same underlying reference tree.

A typical analysis pipeline is to dereplicate input sequences prior to phylogenetic placement,
for example by removing duplicates across samples, and creating one large `fasta` file of all
unique sequences in a study. This saves computational effort, as identical sequences are
placed identically. Later on, one might then however wish to create per-sample `jplace` files,
as if those had been produced by individual per-sample placement in the first place.
This can be achieved with this command. The resulting `jplace` files can then for example
be used for our per-sample analysis methods, see [here](../wiki).

Note however that we introduced a dedicated pipeline for the above use case,
that already takes care of the whole bookkeeping of which sequences belong to which sample.
See the [chunkify](../wiki/Subcommand:-chunkify) and [unchunkify](../wiki/Subcommand:-unchunkify)
commands for details.

### `--split-file`

The format follows the specification of
[split placefiles](http://matsen.github.io/pplacer/generated_rst/guppy.html#split-placefiles)
of the [pplacer/guppy](https://matsen.fhcrc.org/pplacer/) suite of programs.
It expects a comma-separated list of which pquery/read should be put in which output sample
(output `jplace` file):

    "read_1","smpl_a"
    "read_2","smpl_b"
    ...

This will produce two files, `smpl_a.jplace` and `smpl_b.jplace`, each containing one
read from the original input file(s). The names of these reads ("read_1" and "read_2")
need to correspond to pquery names in the input `jplace` files.

Our split command also supports an optional third column containing the new multiplicity
for the read as a (floating point) number. By default, a multiplicity of 1.0 is used.
That is, the original multiplicity of the input `jplace` file is not used.

### `--otu-table-file`

A typical format to specify which reads/sequences/OTUs occur how often in which sample
are OTU tables: They list the abundance (what we call multiplicity in the placement context)
for each sequence in each sample:

    read	smpl_a	smpl_b
    read_1	12	0
    read_2	0	5
    ...

The table is tab-separated. The first line (header) contains the names of the samples;
here, two output files `smpl_a.jplace` and `smpl_b.jplace` are produced.
The first column lists the pquery names as they occur in the input `jplace` file.

OTU tables are typically quite spare, that is, they contain mostly zeros.
In order to efficiently process such tables, the command does not keep the whole table in memory,
but only the non-zero entries. This should allow to process large tables on typical desktop
computers.
