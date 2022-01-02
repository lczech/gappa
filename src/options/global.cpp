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

#include "options/global.hpp"

#include "tools/version.hpp"

#include "genesis/utils/core/logging.hpp"
#include "genesis/utils/core/options.hpp"

#include <thread>

// =================================================================================================
//      Setup Functions
// =================================================================================================

void GlobalOptions::initialize( int const argc, char const* const* argv )
{
    // By default, use the OpenMP or hardware threads, taking hypterthreding into account.
    opt_threads_ = genesis::utils::Options::get().guess_number_of_threads();

    // If hardware value is not available, just use 1 thread.
    // This is executed if the call to the above function fails.
    if( opt_threads_ == 0 ) {
        opt_threads_ = 1;
    }

    // Set number of threads for genesis.
    genesis::utils::Options::get().number_of_threads( opt_threads_ );

    // Set verbosity to max, just in case.
    genesis::utils::Logging::max_level( genesis::utils::Logging::LoggingLevel::kDebug4 );

    // Store all arguments in the array.
    command_line_.clear();
    for (int i = 0; i < argc; i++) {
        command_line_.push_back(argv[i]);
    }
}

void GlobalOptions::add_to_module( CLI::App& module )
{
    for( auto subcomm : module.get_subcommands({}) ) {
        add_to_subcommand( *subcomm );
    }
}

void GlobalOptions::add_to_subcommand( CLI::App& subcommand )
{
    // Allow to overwrite files.
    auto opt_allow_file_overwriting = subcommand.add_flag(
        // "--allow-file-overwriting",
        allow_file_overwriting_flag,
        opt_allow_file_overwriting_,
        "Allow to overwrite existing output files instead of aborting the command."
    );
    opt_allow_file_overwriting->group( "Global Options" );

    // Verbosity
    auto opt_verbose = subcommand.add_flag(
        "--verbose",
        opt_verbose_,
        "Produce more verbose output."
    );
    opt_verbose->group( "Global Options" );

    // Threads
    auto opt_threads = subcommand.add_option(
        "--threads",
        opt_threads_,
        "Number of threads to use for calculations."
    );
    opt_threads->group( "Global Options" );

    // Log File
    auto opt_log_file = subcommand.add_option(
        "--log-file",
        opt_log_file_,
        "Write all output to a log file, in addition to standard output to the terminal."
    );
    opt_log_file->group( "Global Options" );

    // TODO add random seed option
}

// =================================================================================================
//      Run Functions
// =================================================================================================

void GlobalOptions::run_global()
{
    // If user did not provide number, use hardware value.
    if( opt_threads_ == 0 ) {
        opt_threads_ = genesis::utils::Options::get().guess_number_of_threads();
        // opt_threads_ = std::thread::hardware_concurrency();
    }

    // If hardware value is not available, just use 1 thread.
    // This is executed if the call to hardware_concurrency fails.
    if( opt_threads_ == 0 ) {
        opt_threads_ = 1;
    }

    // Set number of threads for genesis.
    genesis::utils::Options::get().number_of_threads( opt_threads_ );

    // Allow to overwrite files. Has to be done before adding the log file (coming below),
    // as this might already fail if the log file exists.
    if( opt_allow_file_overwriting_ ) {
        genesis::utils::Options::get().allow_file_overwriting( true );
    }

    // Set log file.
    if( ! opt_log_file_.empty() ) {
        genesis::utils::Logging::log_to_file( opt_log_file_ );
    }

    // Set verbosity level for logging output.
    if( opt_verbose_ ) {
        genesis::utils::Logging::max_level( genesis::utils::Logging::LoggingLevel::kMessage2 );
    } else {
        genesis::utils::Logging::max_level( genesis::utils::Logging::LoggingLevel::kMessage1 );
    }
}

// =================================================================================================
//      Getters
// =================================================================================================

std::string GlobalOptions::command_line() const
{
    std::string ret = "";
    for (size_t i = 0; i < command_line_.size(); ++i) {
        ret += ( i==0 ? "" : " " ) + command_line_[i];
    }
    return ret;
}

// =================================================================================================
//      Global Instance
// =================================================================================================

/**
 * @brief Instanciation of the global options object. This is alive during the whole program run.
 */
GlobalOptions global_options;

std::string const allow_file_overwriting_flag = "--allow-file-overwriting";
