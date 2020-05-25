/*
    gappa - Genesis Applications for Phylogenetic Placement Analysis
    Copyright (C) 2017-2020 Lucas Czech and HITS gGmbH

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

#include "tools/cli_setup.hpp"

#include "options/global.hpp"

#include "tools/cli_option.hpp"
#include "tools/misc.hpp"
#include "tools/references.hpp"
#include "tools/version.hpp"

#include "genesis/utils/core/algorithm.hpp"
#include "genesis/utils/text/string.hpp"
#include "genesis/utils/tools/date_time.hpp"

#include <stdexcept>
#include <unordered_set>

// =================================================================================================
//      CLI11 Setup Internal Functions
// =================================================================================================

/**
 * @brief Magic constant for the left column width when printing information.
 */
static const size_t left_column_width_ = 35;

void print_header( CLI::App const* sub )
{
    // Print the nice gappa header.
    LOG_BOLD << gappa_header();
    LOG_BOLD;

    // Get the command usage line.
    std::string usage = sub->get_name();
    auto parent = sub->get_parent();
    while( parent ) {
        usage  = parent->get_name() + " " + usage;
        parent = parent->get_parent();
    }

    // Print basic command information.
    LOG_BOLD << format_columns( "Invocation:", global_options.command_line(), left_column_width_ );
    LOG_BOLD << format_columns( "Command:", usage, left_column_width_ );
    LOG_BOLD;
}

std::string get_option_value( CLI::Option const* option )
{
    std::string value;

    // Non-flags
    if( option->get_type_size() != 0 ) {

        // If the option was found on command line
        if( option->count() > 0 ) {
            value = CLI::detail::ini_join( option->results() );
            // value = genesus::utils::join( option->results(), " " );

        // Else use the default
        } else {
            value = option->get_default_str();
            // + " (default)";
        }

    // Flag, one passed
    } else if( option->count() == 1 ) {
        value = "true";

    // Flag, multiple passed
    } else if( option->count() > 1 ) {
        value = std::to_string(option->count());

    // Flag, not present
    } else if( option->count() == 0 ) {
        value = "false";
    }

    return value;
}

void print_option_values( CLI::App const* subcommand )
{
    // Store per-group output, so that it is properly sorted.
    // The vector keeps the order in which groups are added.
    // Its content are: group name and full output to be printed.
    std::vector<std::pair<std::string, std::string>> group_output;

    // Helper function to add text to an option group.
    // First check if the group was already used, and if not add it.
    auto get_group_content = [ &group_output ]( std::string const& name ) -> std::string& {
        for( auto& elem : group_output ) {
            if( elem.first == name ) {
                return elem.second;
            }
        }
        group_output.push_back({ name, "" });
        return group_output.back().second;
    };

    // Add output for each option.
    for( auto option : subcommand->get_options()) {

        // Do not add help option.
        if( option->get_name() == "-h,--help" || option->get_name() == "--help" ) {
            continue;
        }

        // Add the option to its group.
        auto const line = format_columns(
            "  " + option->get_name(),
            get_option_value( option ),
            left_column_width_
        );
        get_group_content( option->get_group() ) += line;
    }

    // Now we have a nicely sorted list of all options in their groups.
    // Print them!
    for( auto const& group : group_output ) {
        LOG_BOLD << group.first << ":";
        LOG_BOLD << group.second;
        LOG_BOLD;
    }
}

void print_citations( std::vector<std::string> const& citations )
{
    LOG_BOLD << "Run the following command to get the references that need to be cited:";
    LOG_BOLD << "`gappa tools citation " << genesis::utils::join( citations, " " ) << "`";
    LOG_BOLD;
}

// =================================================================================================
//      CLI11 Setup
// =================================================================================================

std::function<void()> gappa_cli_callback(
    CLI::App const*          subcommand,
    std::vector<std::string> citations,
    std::function<void()>    run_function
) {
    // Check whether the reference keys are valid.
    // This is immediately run, and not part of the callback itself.
    // This way, the check is always performed, for all subcommands, so that we do not need
    // to execute a command to be sure that its citations are correctly set.
    check_citations( citations );

    // Add the citations to the list, so that they can be used by the wiki command
    // to automatically generate citation lists at the bottom of each page.
    if( citation_list.count( subcommand ) > 0 ) {
        throw std::runtime_error(
            "Internal error: Citation list for subcommand " + subcommand->get_name() +
            " has already been set."
        );
    }
    if( ! citations.empty() ) {
        citation_list[ subcommand ] = citations;
    }

    // If the genesis/gappa citation is not present, add it to the front!
    if( ! genesis::utils::contains( citations, "Czech2020-genesis-and-gappa" )) {
        citations.insert( citations.begin(), "Czech2020-genesis-and-gappa" );
    }

    return [ subcommand, citations, run_function ](){

        // Run the global options callback. Need to this before everything else,
        // so that the number of threads etc are properly set.
        global_options.run_global();

        // Print out the full header, with all option values (including the number of threads
        // that was just set by the above global options callback).
        print_header( subcommand );
        print_option_values( subcommand );
        print_citations( citations );

        LOG_MSG << "Started " << genesis::utils::current_date() << " " << genesis::utils::current_time();
        LOG_BOLD;

        // Run the actual command callback function.
        run_function();

        LOG_BOLD;
        LOG_MSG << "Finished " << genesis::utils::current_date() << " " << genesis::utils::current_time();
    };
}

/**
 * @brief Instanciation of the citation list object. This is alive during the whole program run.
 */
CitationList citation_list;

// =================================================================================================
//      Checks and Helpers
// =================================================================================================

void check_unique_command_names_rec( CLI::App const& app, std::unordered_set<std::string>& names )
{
    auto const name = app.get_name();

    // Do the check. If the group is empty, the command is hidden, for example,
    // because it is a legacy command created by add_legacy_command(). In that case, skip it.
    if( ! app.get_group().empty() ) {
        if( names.count( name ) > 0 ) {
            throw std::runtime_error( "Gappa command name duplicate: " + name );
        }
        names.insert( name );
    }

    // Recursively run this for subcommands.
    for( auto subcom : app.get_subcommands({}) ) {
        check_unique_command_names_rec( *subcom, names );
    }
}

void check_unique_command_names( CLI::App const& app )
{
    std::unordered_set<std::string> names;
    check_unique_command_names_rec( app, names );
}

void check_subcommand_names( CLI::App const& app )
{
    // Check name and description of the command itself.
    if( app.get_name().empty() ) {
        throw std::runtime_error( "Empty subcommand name." );
    }
    if( app.get_description().empty() ) {
        throw std::runtime_error( "Empty subcommand description in " + app.get_name() );
    }

    // Also check all its options.
    for( auto option : app.get_options()) {
        if( option->get_name().empty() ) {
            throw std::runtime_error( "Empty option name in " + app.get_name()  );
        }
        if( option->get_description().empty() ) {
            throw std::runtime_error(
                "Empty option description in " + app.get_name() + " --> " + option->get_name()
            );
        }
    }

    // Recursively run this for subcommands.
    for( auto subcom : app.get_subcommands({}) ) {
        check_subcommand_names( *subcom );
    }
}

void fix_cli_default_values( CLI::App& app )
{
    // Make all option capture their defaults now!
    for( auto option : app.get_options()) {
        // Stupid CLI only exposes const pointers... but the misuse here works,
        // as the function uses a non-const App.
        const_cast<CLI::Option*>( option )->capture_default_str();
    }

    // Recursively run this for subcommands.
    for( auto subcom : app.get_subcommands({}) ) {
        fix_cli_default_values( *subcom );
    }
}

void set_module_help_group( CLI::App& module, std::string const& group_name )
{
    for( auto subcom : module.get_subcommands({}) ) {

        // Get the current settings for the help flag.
        auto const name = subcom->get_help_ptr()->get_name(false, true);
        auto const desc = subcom->get_help_ptr()->get_description();

        // First remove it, then add it again. This way, it is the last one to be added,
        // which is nicer for the help message.
        subcom->set_help_flag();
        subcom->set_help_flag( name, desc );
        subcom->get_help_ptr()->group( group_name );
    }
}

// =================================================================================================
//      CLI11 Option Printing
// =================================================================================================

// -------------------------------------------------------------------------
//     Basic Printing for a Single Option
// -------------------------------------------------------------------------

// template<typename T>
// std::ostream& operator << ( std::ostream& out, CliOption<T> const& option )
// {
//     // Only print if the option is actually used!
//     if( ! option.option ) {
//         return out;
//     }
//
//     // Turn the value into a string.
//     std::stringstream value;
//     value << option.value;
//
//     // Print a standard column.
//     out << format_columns( option.option->get_name(), value.str(), 30 );
//     return out;
// }

// -------------------------------------------------------------------------
//     Variadic Template to Work with Options of Different Template Types
// -------------------------------------------------------------------------

// template<typename T>
// void print_options( CliOption<T> const& option )
// {
//     std::cout << option;
// }
//
// template<typename T, typename... Args>
// void print_options( CliOption<T> const& first, Args... args )
// {
//     print_options( first );
//     print_options( args... );
// }
