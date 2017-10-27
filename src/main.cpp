/*
    gappa - Genesis Applications for Phylogenetic Placement Analysis
    Copyright (C) 2017 Lucas Czech

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

#include "commands/squash.hpp"

int main( int argc, char** argv )
{
    // Activate logging.
    // utils::Logging::log_to_stdout();
    // utils::Logging::details.time = true;
    //
    // utils::Options::get().number_of_threads( 4 );
    // LOG_BOLD << utils::Options::get().info();

    CLI::App app{ "gappa - Genesis Applications for Phylogenetic Placement Analysis" };
    app.require_subcommand( 1 );

    // Set up all subcommands.
    setup_squash( app );

    try {
        app.parse( argc, argv );
    } catch ( CLI::ParseError const& e ) {
        return app.exit( e );
    }

    return 0;
}
