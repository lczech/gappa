## Description

The auxiliary command prints references that need to be cited when using gappa. By default, it simply prints the genesis/gappa reference itself.

It can however also be used to print other references used by commands in gappa. Each reference has a citation key (similar to how BibTeX uses them). If such `keys` are listed after the command, only the specificed references are printed:

    gappa tools citation Czech2020-genesis-and-gappa

The `--list` flag prints a list of all available citation keys.
Furthermore, the `--all` flag prints out all available references that are used in gappa commands.
Using the `--format` option, the output format can be chosen as needed.
