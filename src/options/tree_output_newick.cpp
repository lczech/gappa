/*
    gappa - Genesis Applications for Phylogenetic Placement Analysis
    Copyright (C) 2017-2022 Lucas Czech

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Contact:
    Lucas Czech <lczech@carnegiescience.edu>
    Department of Plant Biology, Carnegie Institution For Science
    260 Panama Street, Stanford, CA 94305, USA
*/

#include "options/tree_output_newick.hpp"

#include "genesis/utils/io/output_target.hpp"

#include <iostream>
#include <stdexcept>

// =================================================================================================
//      Setup Functions
// =================================================================================================

void NewickTreeOutputOptions::add_newick_tree_output_opts_to_app(
    CLI::App* sub, CLI::Option* newick_tree_opt
) {
    add_newick_tree_branch_length_precision_opt_to_app( sub, newick_tree_opt );
    add_newick_tree_quote_invalid_chars_opt_to_app( sub, newick_tree_opt );
}

void NewickTreeOutputOptions::add_newick_tree_branch_length_precision_opt_to_app(
    CLI::App* sub, CLI::Option* newick_tree_opt
) {
    // branch length precision
    auto branch_length_precision_opt = sub->add_option(
        "--newick-tree-branch-length-precision",
        branch_length_precision_,
        "Number of digits to print for branch lengths in Newick format.",
        true
    );
    branch_length_precision_opt->group( "Newick Tree Output" );
    if( newick_tree_opt ) {
        branch_length_precision_opt->needs( newick_tree_opt );
    }
}

void NewickTreeOutputOptions::add_newick_tree_quote_invalid_chars_opt_to_app(
    CLI::App* sub, CLI::Option* newick_tree_opt
) {
    // invalid characters
    auto quote_invalid_chars_opt = sub->add_flag(
        "--newick-tree-quote-invalid-chars",
        quote_invalid_chars_,
        "If set, node labels that contain characters that are invalid in the Newick format "
        "(i.e., spaces and `:;()[],{}`) are put into quotation marks. "
        "If not set (default), these characters are instead replaced by underscores, "
        "which changes the names, but works better with most downstream tools."
    );
    quote_invalid_chars_opt->group( "Newick Tree Output" );
    if( newick_tree_opt ) {
        quote_invalid_chars_opt->needs( newick_tree_opt );
    }
}

// =================================================================================================
//      Run Functions
// =================================================================================================

void NewickTreeOutputOptions::write_tree(
    genesis::tree::CommonTree const& tree,
    std::shared_ptr<genesis::utils::BaseOutputTarget> target
) const {
    auto writer = genesis::tree::CommonTreeNewickWriter();
    write_tree( writer, tree, target );
}

void NewickTreeOutputOptions::write_tree(
    genesis::tree::CommonTreeNewickWriter& writer,
    genesis::tree::CommonTree const& tree,
    std::shared_ptr<genesis::utils::BaseOutputTarget> target
) const {
    writer.replace_invalid_chars( !quote_invalid_chars_ );
    writer.branch_length_precision( branch_length_precision_ );
    writer.write( tree, target );
}
