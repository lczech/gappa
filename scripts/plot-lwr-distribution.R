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
    infile="lwr-distribution.csv"
    outfile = "lwr-distribution"
} else if (length(args) == 1) {
    infile = args[1]
    outfile = "lwr-distribution"
} else if (length(args) == 2) {
    infile = args[1]
    outfile = args[2]
} else {
    stop( "Usage: plot-lwr-distribution.R [infile] [outfile]")
}
print(paste("Infile: ", infile))
print(paste("Outfile:", outfile))

# Read table...
data <- read.table(infile, sep=",", header=TRUE)

# ... and turn it into the R long format, putting the previous column names into `Group`,
# and the values into `LWR`, taking all columns with LWR.x and the Remainder.
data_long <- gather(data, Group, LWR, `LWR.1`:`Remainder`, factor_key=TRUE)

# Plot the data and save to file.
# We use a stacked area plot, reverse the order in the plot so that the most likely is at the bottom
# of the plot, while the legend stays with LWR.1 as the first. This looks best, I think.
# We Fill with viridis, which has the nice property to make the Remainder pure yellow,
# which is a nice color for this.
# Furthermore, we force the y-axis limit to 0..1, which necessitates to use a trick to not let
# R delete data if the LWR is slightly above 1.0 due to rounding issues... R is messy.
ggplot(data_long, aes(x=Index, y=LWR, fill=Group)) +
    geom_area(alpha = .8, position = position_stack(reverse = TRUE)) +
    scale_fill_viridis(discrete = T) +
    coord_cartesian(ylim=c(0, 1)) +
    xlab("Pquery Index") +
    ylab("Accumulated LWR") +
    theme(legend.title=element_blank())
ggsave(paste0(outfile,".png"), width=12, height=8)
