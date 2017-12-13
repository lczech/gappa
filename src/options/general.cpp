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

#include "options/general.hpp"

#include "tools/version.hpp"

#include "genesis/utils/core/options.hpp"

#ifdef GENESIS_PTHREADS
#    include <thread>
#endif

// =================================================================================================
//      Setup Functions
// =================================================================================================

void GeneralOptions::add_general_options( CLI::App& app )
{
    // Verbosity
    auto v_s = app.add_option(
        "--verbosity",
        verbosity_,
        "Verbosity level [0-3]",
        true
    );
    auto v_c = app.add_flag(
        "-v",
        verbosity_cnt_,
        "Verbosity; add multiple times for more (-vvv)"
    );
    v_s->excludes( v_c );
    v_c->excludes( v_s );

    // Threads
    app.add_option(
        "--threads",
        threads_,
        "Number of threads to use for calculations"
    );

    // Run the app wide callback
    app.set_callback([ this ](){
        callback();
        print_general_options();
    });

    // Footer
    app.set_footer( gappa_title() );
}

// =================================================================================================
//      Run Functions
// =================================================================================================

void GeneralOptions::print_general_options() const
{
    if( verbosity() == 0 ) {
        return;
    }

    // Print our nice header.
    std::cout << gappa_header() << "\n";

    if( verbosity() > 1 ) {
        std::cout << "Number of threads: " << threads() << "\n";
    }
}

size_t GeneralOptions::verbosity() const
{
    return ( verbosity_cnt_ > 0 ? verbosity_cnt_ + 1 : verbosity_ );
}

size_t GeneralOptions::threads() const
{
    return threads_;
}

void GeneralOptions::callback()
{
    // If user did not provide number, use hardware value.
    #if defined( GENESIS_PTHREADS )

        if( threads_ == 0 ) {
            threads_ = std::thread::hardware_concurrency();
        }

    #endif

    // If hardware value is not available, just use 1 thread.
    // This is executed both when the above ifdef is not compiled and
    // if the call to hardware_concurrency fails.
    if( threads_ == 0 ) {
        threads_ = 1;
    }

    // Set number of threads for genesis.
    genesis::utils::Options::get().number_of_threads( threads_ );
}
