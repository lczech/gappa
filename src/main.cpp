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

#include "commands/analyze.hpp"
#include "commands/edit.hpp"
#include "commands/examine.hpp"
#include "commands/prepare.hpp"
#include "commands/tools.hpp"

#include "options/global.hpp"

#include "tools/cli_formatter.hpp"
#include "tools/cli_setup.hpp"
#include "tools/references.hpp"
#include "tools/version.hpp"

// =================================================================================================
//      Main Program
// =================================================================================================

void setup_main_app_options( CLI::App& app )
{
    // Version. We use the callback to immediately process the flag if set,
    // similar to how --help works. This way, no subcommand is required when this option is used.
    app.add_flag_callback(
        "--version",
        [] () {
            std::cout << gappa_version() << "\n";
            throw CLI::Success{};
        },
        "Print the gappa version and exit."
    );
}

int main( int argc, char** argv )
{
    // -------------------------------------------------------------------------
    //     Logging
    // -------------------------------------------------------------------------

    // Activate logging.
    genesis::utils::Logging::log_to_stdout();
    genesis::utils::Logging::details.level = false;
    genesis::utils::Logging::details.time = true;

    // -------------------------------------------------------------------------
    //     App Setup
    // -------------------------------------------------------------------------

    // Set up the main CLI app.
    CLI::App app{ gappa_header() };
    app.name( "gappa" );
    app.footer( "\n" + gappa_title() );

    // We use a custom formatter for the help messages that looks nicer.
    app.formatter( std::make_shared<GappaFormatter>() );

    // We don't like short options in gappa. Reset the help option.
    // This is inherited by subcommands automatically.
    app.set_help_flag( "--help", "Print this help message and exit." );
    // app.get_help_ptr()->group( "Global Options" );

    // We want all options to capture their default values, so that we see them in the help.
    // No idea why CLI does not do that all the time. Hopefully, this is the correct way to solve this.
    app.option_defaults()->always_capture_default( true );

    // Gappa expects exactly one subcommand.
    app.require_subcommand( 1 );

    // The main app offers a few options as well.
    setup_main_app_options( app );

    // -------------------------------------------------------------------------
    //     Subcommand Setup
    // -------------------------------------------------------------------------

    // Init global options. This ensures that all subcommands that need the global options
    // have them initialized to proper values that they can use (eg for the help output).
    global_options.initialize( argc, argv );

    // Set up all subcommands.
    setup_analyze( app );
    setup_edit( app );
    setup_examine( app );
    setup_prepare( app );
    setup_tools( app );

    // -------------------------------------------------------------------------
    //     Final Checks and Steps
    // -------------------------------------------------------------------------

    // General checks before running.
    // These are mainly to support the development, to avoid bugs and mistakes.
    check_all_citations();
    check_unique_command_names( app );
    check_subcommand_names( app );

    // Final steps before we can run.
    // Make sure that we have all defaults captured,
    // so that they can be used in the header print of each command.
    fix_cli_default_values( app );

    // -------------------------------------------------------------------------
    //     Go Go Go
    // -------------------------------------------------------------------------

    try {
        app.parse( argc, argv );
    } catch ( CLI::ParseError const& e ) {
        return app.exit( e );
    }

    return 0;
}
