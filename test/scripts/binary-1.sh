#!/bin/bash

# Make sure that genesis is linked statically into gappa,
# meaning that we do not find it via `ldd` in the gappa binary.
STATIC_GENESIS=`ldd ${GAPPA} | grep "genesis"`
[[ -z ${STATIC_GENESIS} ]] || return 1
