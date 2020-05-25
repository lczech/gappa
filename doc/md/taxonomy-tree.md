## Description

The command takes a taxonomy as input, and converts it into a tree in `Newick` format,
which can for example be used as a (taxonomic) constraint for tree inference.
This is particularly useful for a set of sequences that each have a taxonomic assignment.

The required taxonomy can be given in two ways:

  * `--taxon-list-file`: A map from taxon names to their taxonomic path.
  * `--taxonomy-file`: A list of taxonomic paths.

Both of them can be given at the same time, in which case the taxonomic information is combined.
See below for details on the expected formats.

The output of the command is a tree in Newick format, written to a file named `taxonomy-tree.newick`
(additionally using the `--file-prefix` if provided).

## Details

A taxonomy is a hierarchy that can be interpreted as a rooted tree.
The command can be used in general to convert a taxonomy into a (multifurcating) tree.
The most typical use case however is to create a taxonomically constrained tree that can be used
for tree inference, using the taxonomy of the sequences as constraint.

### `--taxon-list-file`

This is useful if the taxonomy is used for a set of sequences that have taxonomic assignments:
One might wish to build a tree where tips correspond to sequences, and the tree topology
reflects the taxonomy of these sequences. For such a use case, we use the taxonomy of the sequences
as input, by mapping sequences names to their taxonomic paths, separated by tabs.

Example:

    AY842031	Eukaryota;Amoebozoa;Myxogastria;Amaurochaete;Amaurochaete_comata
    JQ031957	Eukaryota;Amoebozoa;Myxogastria;Brefeldia;Brefeldia_maxima
    ...

This will create a tree with tips labeled `AY842031`, `JQ031957`, etc, and a topology that reflects
the taxonomic paths.

### `--taxonomy-file`

A more general case is to simply use the full taxonomy to create a tree. This input file needs to
contain a list of semicolon-separated taxonomic paths. Everything after the first tab is ignored.

Example:

```
Eukaryota;	4	domain
Eukaryota;Amoebozoa;	4052	kingdom		119
Eukaryota;Amoebozoa;Myxogastria;	4094	phylum		119
Eukaryota;Amoebozoa;Myxogastria;Amaurochaete;	4095	genus		119
Eukaryota;Amoebozoa;Myxogastria;Badhamia;	4096	genus		119
Eukaryota;Amoebozoa;Myxogastria;Brefeldia;	4097	genus		119
Eukaryota;Amoebozoa;Myxogastria;Comatricha;	4098	genus		119
...
```

Both inputs `--taxon-list-file` and `--taxonomy-file` can be used at the same time,
in which case the resulting tree contains the taxonomic information from both files.

### Settings

It might happen that a taxonomic path goes down several levels with just one taxon at each level.
This would create inner nodes in the tree that just connect two other nodes, that is, nodes that
do not furcate at all. Many downstream programs might have problems with such trees.
Hence, by default, such nodes are collapsed. Use `--keep-singleton-inner-nodes` to include these
inner nodes in the tree.

Furthermore, a taxonomy contains names at every level, while a tree usually does not contain
inner node names. Thus, by default, inner nodes are not named. Use `--keep-inner-node-names`
to also name the inner nodes of the tree.

Lastly, `--max-level` can be used to only use the first few levels (starting at 0) of the taxonomy
for constructing the tree, and stopping after that. Per default, the whole taxonomy is used.
