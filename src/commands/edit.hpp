#ifndef GAPPA_COMMANDS_EDIT_H_
#define GAPPA_COMMANDS_EDIT_H_

/*
    gappa - Genesis Applications for Phylogenetic Placement Analysis
    Copyright (C) 2017-2025 Lucas Czech

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
    Lucas Czech <lucas.czech@sund.ku.dk>
    University of Copenhagen, Globe Institute, Section for GeoGenetics
    Oster Voldgade 5-7, 1350 Copenhagen K, Denmark
*/

#include "CLI/CLI.hpp"

#include "commands/edit/accumulate.hpp"
#include "commands/edit/extract.hpp"
#include "commands/edit/filter.hpp"
#include "commands/edit/merge.hpp"
#include "commands/edit/multiplicity.hpp"
#include "commands/edit/split.hpp"

#include "options/global.hpp"
#include "tools/cli_setup.hpp"

#include <string>
#include <vector>

// =================================================================================================
//      Functions
// =================================================================================================

inline void setup_edit( CLI::App& app )
{
    // Create the module subcommand objects.
    auto sub = app.add_subcommand(
        "edit",
        "Commands for editing and manipulating files like jplace, fasta or newick."
    );
    sub->require_subcommand( 1 );

    // Add module subcommands.
    setup_accumulate( *sub );
    setup_extract( *sub );
    setup_filter( *sub );
    setup_merge( *sub );
    setup_multiplicity( *sub );
    setup_split( *sub );

    // Add the global options to each of the above subcommands.
    global_options.add_to_module( *sub );
    set_module_help_group( *sub );
}

#endif // include guard
