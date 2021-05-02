#ifndef GAPPA_COMMANDS_SIMULATE_H_
#define GAPPA_COMMANDS_SIMULATE_H_

/*
    gappa - Genesis Applications for Phylogenetic Placement Analysis
    Copyright (C) 2017-2021 Lucas Czech

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

#include "commands/simulate/random_alignment.hpp"
#include "commands/simulate/random_placements.hpp"
#include "commands/simulate/random_tree.hpp"

#include "options/global.hpp"
#include "tools/cli_setup.hpp"

#include <string>
#include <vector>

// =================================================================================================
//      Functions
// =================================================================================================

inline void setup_simulate( CLI::App& app )
{
    // Create the module subcommand objects.
    auto sub = app.add_subcommand(
        "simulate",
        "Commands for random generation of phylogenetic and placement data."
    );
    sub->require_subcommand( 1 );

    // Add module subcommands.
    setup_random_alignment( *sub );
    setup_random_placements( *sub );
    setup_random_tree( *sub );

    // Add the global options to each of the above subcommands.
    global_options.add_to_module( *sub );
    set_module_help_group( *sub );
}

#endif // include guard
