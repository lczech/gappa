#!/bin/bash

${GAPPA} analyze dispersion \
    --jplace-path "data/jplace" \
    --mass-norm "relative" \
    --write-newick-tree \
    --write-nexus-tree \
    --write-phyloxml-tree \
    --write-svg-tree \
    --svg-tree-stroke-width 8.0 \
    --svg-tree-ladderize \
    --out-dir ${OUTDIR}

testfile "${OUTDIR}/imbalances_sd_log.newick"             6810 || return 1
testfile "${OUTDIR}/imbalances_sd_log.nexus"             20365 || return 1
testfile "${OUTDIR}/imbalances_sd_log.phyloxml"         704177 || return 1
testfile "${OUTDIR}/imbalances_sd_log.svg"              276473 || return 1
testfile "${OUTDIR}/imbalances_sd.newick"                 6810 || return 1
testfile "${OUTDIR}/imbalances_sd.nexus"                 20365 || return 1
testfile "${OUTDIR}/imbalances_sd.phyloxml"             704365 || return 1
testfile "${OUTDIR}/imbalances_sd.svg"                  276076 || return 1
testfile "${OUTDIR}/imbalances_var_log.newick"            6810 || return 1
testfile "${OUTDIR}/imbalances_var_log.nexus"            20365 || return 1
testfile "${OUTDIR}/imbalances_var_log.phyloxml"        704265 || return 1
testfile "${OUTDIR}/imbalances_var_log.svg"             276498 || return 1
testfile "${OUTDIR}/imbalances_var.newick"                6810 || return 1
testfile "${OUTDIR}/imbalances_var.nexus"                20365 || return 1
testfile "${OUTDIR}/imbalances_var.phyloxml"            704376 || return 1
testfile "${OUTDIR}/imbalances_var.svg"                 276083 || return 1
testfile "${OUTDIR}/masses_cv_log.newick"                 6810 || return 1
testfile "${OUTDIR}/masses_cv_log.nexus"                 20365 || return 1
testfile "${OUTDIR}/masses_cv_log.phyloxml"             704264 || return 1
testfile "${OUTDIR}/masses_cv_log.svg"                  274511 || return 1
testfile "${OUTDIR}/masses_cv.newick"                     6810 || return 1
testfile "${OUTDIR}/masses_cv.nexus"                     20365 || return 1
testfile "${OUTDIR}/masses_cv.phyloxml"                 703830 || return 1
testfile "${OUTDIR}/masses_cv.svg"                      276077 || return 1
testfile "${OUTDIR}/masses_sd_log.newick"                 6810 || return 1
testfile "${OUTDIR}/masses_sd_log.nexus"                 20365 || return 1
testfile "${OUTDIR}/masses_sd_log.phyloxml"             703757 || return 1
testfile "${OUTDIR}/masses_sd_log.svg"                  276501 || return 1
testfile "${OUTDIR}/masses_sd.newick"                     6810 || return 1
testfile "${OUTDIR}/masses_sd.nexus"                     20365 || return 1
testfile "${OUTDIR}/masses_sd.phyloxml"                 704366 || return 1
testfile "${OUTDIR}/masses_sd.svg"                      276486 || return 1
testfile "${OUTDIR}/masses_var_log.newick"                6810 || return 1
testfile "${OUTDIR}/masses_var_log.nexus"                20365 || return 1
testfile "${OUTDIR}/masses_var_log.phyloxml"            704170 || return 1
testfile "${OUTDIR}/masses_var_log.svg"                 276502 || return 1
testfile "${OUTDIR}/masses_var.newick"                    6810 || return 1
testfile "${OUTDIR}/masses_var.nexus"                    20365 || return 1
testfile "${OUTDIR}/masses_var.phyloxml"                704399 || return 1
testfile "${OUTDIR}/masses_var.svg"                     276885 || return 1
testfile "${OUTDIR}/masses_vmr_log.newick"                6810 || return 1
testfile "${OUTDIR}/masses_vmr_log.nexus"                20365 || return 1
testfile "${OUTDIR}/masses_vmr_log.phyloxml"            703753 || return 1
testfile "${OUTDIR}/masses_vmr_log.svg"                 276501 || return 1
testfile "${OUTDIR}/masses_vmr.newick"                    6810 || return 1
testfile "${OUTDIR}/masses_vmr.nexus"                    20365 || return 1
testfile "${OUTDIR}/masses_vmr.phyloxml"                704367 || return 1
testfile "${OUTDIR}/masses_vmr.svg"                     275696 || return 1
