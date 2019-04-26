## Description

The command creates a random alignment with a given number of sequences of a given length.
The sequences are named with simple letter combinations, going `a, ..., z, aa, ..., az, ba, ...`.
The characters in the alignment sequences are randmonly chosen from the provided character set.

At least one of the output format option flags `--write-fasta` and `--write-phylip` has to be provided.
The output files are named `random-alignment.fasta` and `random-alignment.phylip`, respectively,
potentially using the `--file-prefix` if provided.
