#!/usr/bin/env Rscript

# gappa - Genesis Applications for Phylogenetic Placement Analysis
# Copyright (C) 2017-2021 Lucas Czech
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
suppressMessages(library(viridis))

# Get input and output files from command line, or use defaults.
args = commandArgs(trailingOnly=TRUE)
if (length(args) == 0) {
    infile="lwr-histogram.csv"
    outfile = "lwr-histogram"
} else if (length(args) == 1) {
    infile = args[1]
    outfile = "lwr-histogram"
} else if (length(args) == 2) {
    infile = args[1]
    outfile = args[2]
} else {
    stop( "Usage: plot-lwr-histogram.R [infile] [outfile]")
}
print(paste("Infile: ", infile))
print(paste("Outfile:", outfile))

# Read table, and change the Start column to text with two decimals and a greater sign...
data <- read.table(infile, sep=",", header=TRUE)
data$Start <- format(data$Start, digits=2, nsmall=2)
data$Start <- paste0("> ", data$Start)

# ... and turn it into the R long format, putting the previous column names into `Group`,
# and the values into `LWR`, taking all columns with LWR.x as needed.
# Uncomment the second line to put all values of the table into the histogram.
# By default, we only do the first three, as otherwise, it's a bit too crowded.
data_long <- gather(data, Group, LWR, `Percentage.1`:`Percentage.3`, factor_key=TRUE)
# data_long <- gather(data, Group, LWR, `Percentage.1`:`Percentage.Remainder`, factor_key=TRUE)

# For plotting, we change the name of the group column, so that the output legend is nicer.
data_long$Group <- gsub('Percentage', 'LWR', data_long$Group)

# Plot the data and save to file.
# We rotate the x-axis label so that they all fit. This is a bit of trial-and-error...
ggplot(data_long, aes(x=Start, y=LWR, fill=Group)) +
    geom_bar(alpha=0.8, stat="identity", position=position_dodge()) +
    scale_fill_viridis(discrete = T, end = 0.8) +
    xlab("LWR") +
    ylab("Percentage") +
    theme(legend.title=element_blank()) +
    theme(axis.text.x = element_text(angle = 60, vjust = 1.2, hjust = 1.2))
ggsave(paste0(outfile,".png"), width=12, height=8)
