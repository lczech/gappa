#ifndef GAPPA_COMMANDS_PREPARE_CLEAN_TREE_H_
#define GAPPA_COMMANDS_PREPARE_CLEAN_TREE_H_

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
#include "options/tree_output.hpp"

#include <string>
#include <vector>

// =================================================================================================
//      Options
// =================================================================================================

class CleanTreeOptions
{
public:

    // Input data.
    std::string tree_file;

    // Settings.
    bool remove_inner_labels     = false;
    bool replace_invalid_chars   = false;
    bool remove_comments_and_nhx = false;
    bool remove_extra_numbers    = false;
    bool remove_jplace_tags      = false;

    // Output options.
    FileOutputOptions file_output;
};

// =================================================================================================
//      Functions
// =================================================================================================

void setup_clean_tree( CLI::App& app );
void run_clean_tree( CleanTreeOptions const& options );

#endif // include guard
