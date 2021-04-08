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

#include "commands/tools/version.hpp"
#include "options/global.hpp"
#include "tools/references.hpp"
#include "tools/version.hpp"

#include "genesis/utils/core/options.hpp"

// =================================================================================================
//      Setup
// =================================================================================================

void setup_version( CLI::App& app )
{
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

    LOG_BOLD << gappa_header();
    LOG_BOLD;
    LOG_BOLD << "gappa version: " << gappa_version();
    LOG_BOLD;
    LOG_BOLD << genesis::utils::Options::get().info_compile_time();
    LOG_BOLD;
    LOG_BOLD << "For citation information, call  `gappa tools citation`";
    LOG_BOLD << "For license information, call  `gappa tools license`";
    LOG_BOLD;
    LOG_BOLD << gappa_title();
}
