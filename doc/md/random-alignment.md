## Description

The command creates a random alignment with a given number of sequences of a given length.
The sequences are named with simple letter combinations, going `a, ..., z, aa, ..., az, ba, ...`.
The characters in the alignment sequences are randmonly chosen from the provided character set.

At least one of the output format option flags `--write-fasta`, `--write-strict-phylip`,
and `--write-relaxed-phylip` has to be provided, but not both of the `phylip` formats at the same time.
The output files are named `random-alignment.fasta` and `random-alignment.phylip`, respectively,
potentially using the `--file-prefix` and `--file-suffix` if provided.

The differences between strict and relaxed phylip are as follows:
Strict phylip is the original specification, which uses exactly the first 10 characters of a line
to denote the name (filled with spaces if shorter), and requires the whole sequence to be in the rest
of the (potentially very long) line.
Relaxed phylip allows arbitrarily long names, separated by at least one white space from the actual sequence,
and the sequence can be broken down into multiple lines.
