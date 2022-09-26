#ifndef GAPPA_COMMANDS_PREPARE_TAXONOMY_TREE_H_
#define GAPPA_COMMANDS_PREPARE_TAXONOMY_TREE_H_

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

#include "CLI/CLI.hpp"

#include "options/file_input.hpp"
#include "options/file_output.hpp"
#include "options/tree_output_newick.hpp"
#include "options/tree_output.hpp"

#include <string>
#include <vector>

// =================================================================================================
//      Options
// =================================================================================================

class TaxonomyTreeOptions
{
public:

    // Input data.
    std::string taxonomy_file;
    std::string taxon_list_file;

    // Settings.
    bool keep_singleton_inner_nodes = false;
    bool keep_inner_node_names = false;
    int  max_level = -1;
    bool replace_invalid_chars = false;

    // Output options.
    FileOutputOptions file_output;
    NewickTreeOutputOptions newick_tree_output;
};

// =================================================================================================
//      Functions
// =================================================================================================

void setup_taxonomy_tree( CLI::App& app );
void run_taxonomy_tree( TaxonomyTreeOptions const& options );

#endif // include guard
