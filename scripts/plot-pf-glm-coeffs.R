#!/usr/bin/env Rscript

# gappa - Genesis Applications for Phylogenetic Placement Analysis
# Copyright (C) 2017-2024 Lucas Czech
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Contact:
# Lucas Czech <lczech@carnegiescience.edu>
# Department of Plant Biology, Carnegie Institution For Science
# 260 Panama Street, Stanford, CA 94305, USA

# Load packages
library(tidyr)
library(ggplot2)
library(cowplot)
theme_set(theme_cowplot())

# Get input and output files from command line, or use defaults.
meta_file     = "meta.csv"
balances_file = "factor_balances.csv"
coeffs_file   = "factor_glm_coefficients.csv"
out_pref      = "pf-glm-coeffs-"
args = commandArgs(trailingOnly=TRUE)
if (length(args) >= 1) {
    meta_file     = args[1]
}
if (length(args) >= 2) {
    balances_file = args[2]
}
if (length(args) >= 3) {
    coeffs_file   = args[3]
}
if (length(args) <= 4) {
    out_pref      = args[4]
} else {
    stop( "Usage: plot-pf-glm-coeffs.R [meta-data] [balances] [glm-coefficients] [out-prefix]")
}

print(paste("balances:         ", balances_file))
print(paste("glm-coefficients: ", coeffs_file))
print(paste("meta-data:        ", meta_file))
print(paste("out-prefix:       ", out_pref))

Balances     <- read.table(balances_file, sep="\t", header=TRUE)
Coefficients <- read.table(coeffs_file,   sep="\t", header=TRUE)
Meta         <- read.table(meta_file,     sep="\t", header=TRUE)

# Match the Samples between Meta and Balances
# The Meta table might not have the first column named Samples,
# so we just take whatever is the first column here.
matched_samples <- match(Meta[, colnames(Meta)[1]], Balances$Sample)

# Loop through each row in the Coefficients table
for(i in 1:nrow(Coefficients)) {
    print(paste("Factor", i))

    # Extract the current row's coefficients and intercept
    # Exclude Factor and Intercept columns
    current_coefficients <- as.numeric(Coefficients[i, -c(1, 2)])
    current_intercept <- Coefficients$Intercept[i]

    # Compute the dot product for the current coefficients against all Meta rows
    # Then add the current intercept
    result <- as.matrix(Meta[, -1]) %*% current_coefficients + current_intercept

    # Find the corresponding column in the Balances table
    balance_column_name <- paste("Factor", i, sep = "_")
    if(!(balance_column_name %in% names(Balances))) {
        print(paste("Invalid column", balance_column_name))
        next
    }

    # Create a data frame for plotting.
    # Add the result with the corresponding balance value.
    # Note: Ensure that the matched_samples indices are valid and there are no NAs
    df <- data.frame(
        Result = result,
        Balance = Balances[matched_samples, balance_column_name]
    )

    # Generate the scatter plot.
    p <- ggplot(df, aes(x = Balance, y = Result)) +
        geom_point() +
        # geom_smooth(method = "lm", color = "red", se = FALSE, formula="y~x", orientation = "y") +
        labs(
            title = balance_column_name,
            x = "Balance Value",
            y = "GLM Prediction"
        ) +
        geom_abline(slope=1, intercept=0, color='#6666FF') +
        coord_fixed() +
        theme(legend.title=element_blank())

    # Display the plot
    # print(p)
    ggsave(paste0(out_pref, i,".png"), width=12, height=8)
}
