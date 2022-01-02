#ifndef GAPPA_OPTIONS_GLOBAL_H_
#define GAPPA_OPTIONS_GLOBAL_H_

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

#include "tools/cli_option.hpp"
#include "tools/version.hpp"

#include "genesis/utils/core/logging.hpp"

#include <string>
#include <vector>

// =================================================================================================
//      Global Options
// =================================================================================================

class GlobalOptions
{
public:

    // -------------------------------------------------------------------------
    //     Setup Functions
    // -------------------------------------------------------------------------

    /**
     * @brief Init the global options for usage in the main app.
     */
    void initialize( int const argc, char const* const* argv );

    /**
     * @brief Add the global options to all subcommands of a module.
     *
     * This is gappa-specific, as we use the command structure `gappa module subcommand`.
     * This function takes a module, and adds the global options to all its subcommands.
     */
    void add_to_module( CLI::App& module );

    /**
     * @brief Add the global options to a specific subcommand.
     */
    void add_to_subcommand( CLI::App& subcommand );

    // -------------------------------------------------------------------------
    //     Run Functions
    // -------------------------------------------------------------------------

    void run_global();

    // -------------------------------------------------------------------------
    //     Getters
    // -------------------------------------------------------------------------

    std::string command_line() const;

    // -------------------------------------------------------------------------
    //     Option Members
    // -------------------------------------------------------------------------

private:

    // Store variables where the global options store their values as set by the user.
    // We use these exact instances of the variabls for ALL subcommands at the same time,
    // which works, as only one subcommand is called when the program is run, and so they
    // do not conflict. This however means that we cannot use our CliOption class here,
    // as this checks that each option is only set once, which is not the case here.
    // In other words, each subcommand gets its own option added via CLI11, and all of them
    // bind to these variables here.
    bool        opt_allow_file_overwriting_ = false;
    bool        opt_verbose_ = false;
    size_t      opt_threads_ = 0;
    std::string opt_log_file_ = "";

    std::vector<std::string> command_line_;

};

// =================================================================================================
//      Global Instance
// =================================================================================================

/**
 * @brief Store the global options object and its variables.
 *
 * This instance is used by commands to get access to the global options
 * without having to transfer pointers to it all the time.
 * It is alive during the whole run of the program, so that all commands have access to it.
 */
extern GlobalOptions global_options;

/**
 * @brief Store the option name for the flag that allows gappa to overwrite files.
 *
 * We do this in order to have this name available to other parts of the program,
 * for example to give a nice and helpful error message when a file already exists.
 */
extern std::string const allow_file_overwriting_flag;

#endif // include guard
