#ifndef GAPPA_COMMANDS_EXAMINE_H_
#define GAPPA_COMMANDS_EXAMINE_H_

/*
    gappa - Genesis Applications for Phylogenetic Placement Analysis
    Copyright (C) 2017-2019 Lucas Czech and HITS gGmbH

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
    Lucas Czech <lucas.czech@h-its.org>
    Exelixis Lab, Heidelberg Institute for Theoretical Studies
    Schloss-Wolfsbrunnenweg 35, D-69118 Heidelberg, Germany
*/

#include "CLI/CLI.hpp"

#include "commands/examine/assign.hpp"
#include "commands/examine/edpl.hpp"
#include "commands/examine/graft.hpp"
#include "commands/examine/heat_tree.hpp"
#include "commands/examine/info.hpp"
#include "commands/examine/lwr.hpp"

#include "options/global.hpp"
#include "tools/cli_setup.hpp"

#include <string>
#include <vector>

// =================================================================================================
//      Functions
// =================================================================================================

inline void setup_examine( CLI::App& app )
{
    // Create the module subcommand objects.
    auto sub = app.add_subcommand(
        "examine",
        "Commands for examining, visualizing, and tabulating information in placement data."
    );
    sub->require_subcommand( 1 );

    // Add module subcommands.
    setup_assign( *sub );
    setup_edpl( *sub );
    setup_graft( *sub );
    setup_heat_tree( *sub );
    setup_info( *sub );
    setup_lwr( *sub );

    // Add the global options to each of the above subcommands.
    global_options.add_to_module( *sub );
    set_module_help_group( *sub );
}

#endif // include guard
