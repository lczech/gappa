#!/bin/bash

${GAPPA} edit merge \
    --jplace-path "data/jplace/" \
    --out-dir ${OUTDIR}

testfile "${OUTDIR}/merge.jplace" 7794675 || return 1
