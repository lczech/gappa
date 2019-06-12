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

#include "commands/tools/version.hpp"
#include "tools/references.hpp"
#include "tools/version.hpp"

// =================================================================================================
//      Setup
// =================================================================================================

void setup_version( CLI::App& app )
{
    // Good place to check citations. This is executed every time with gappa,
    // so we never miss the check when editing the citation list.
    check_all_citations();

    // Create the options and subcommand objects.
    auto options = std::make_shared<VersionOptions>();
    auto sub = app.add_subcommand(
        "version",
        "Extended version information about gappa."
    );

    // Set the run function as callback to be called when this subcommand is issued.
    // Hand over the options by copy, so that their shared ptr stays alive in the lambda.
    sub->callback( [ options ]() {
        run_version( *options );
    });
}

// =================================================================================================
//      Run
// =================================================================================================

void run_version( VersionOptions const& options )
{
    (void) options;

    std::cout << gappa_header();
    std::cout << "\n";
    std::cout << "For citation information, call  `gappa tools citation`\n";
    std::cout << "For license information, call  `gappa tools license`\n";
    std::cout << "\n";
    std::cout << gappa_title() << "\n";
}
