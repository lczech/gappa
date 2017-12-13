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

#include "options/jplace_input.hpp"

#include "genesis/placement/formats/jplace_reader.hpp"
#include "genesis/utils/core/fs.hpp"

#include <iostream>
#include <stdexcept>

// Relative path resolution for printing.
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <linux/limits.h>

// =================================================================================================
//      Setup Functions
// =================================================================================================

void JplaceInputOptions::add_jplace_input_options( CLI::App* sub )
{
    auto opt_placefiles = sub->add_option(
        "placefiles",
        jplace_paths_,
        "List of jplace files or directories to process"
    );
    opt_placefiles->required();
    opt_placefiles->check([]( std::string const& path ){
        if( ! genesis::utils::path_exists( path ) ) {
            return std::string( "Path is neither a file nor a directory: " + path );
        }
        return std::string();
    });
}

// =================================================================================================
//      Run Functions
// =================================================================================================

void JplaceInputOptions::print_jplace_input_options( size_t verbosity ) const
{
    auto const files = jplace_file_paths();
    if( verbosity == 0 ) {
        return;
    } else if( verbosity == 1 ) {
        std::cout << "Found " << files.size() << " jplace files.\n";
    } else if( verbosity == 2 ) {
        std::cout << "Found " << files.size() << " jplace files: ";
        for( auto const& file : files ) {
            if( &file != &files[0] ) {
                std::cout << ",  ";
            }
            std::cout << genesis::utils::file_basename( file );
        }
        std::cout << "\n";
    } else {
        std::cout << "Found " << files.size() << " jplace files:\n";

        char resolved_path[PATH_MAX];
        for( auto const& file : files ) {
            auto ptr = realpath( file.c_str(), resolved_path );
            if( errno == 0 ) {
                std::cout << "  - " << ptr << "\n";
            } else {
                std::cout << "  - " << file << "\n";
                // use std::strerror(errno) to get error message
                errno = 0;
            }
        }
    }
}

std::vector<std::string> const& JplaceInputOptions::cli_paths() const
{
    return jplace_paths_;
}

std::vector<std::string> JplaceInputOptions::jplace_base_file_names() const
{
    using namespace genesis::utils;

    auto jplace_files = jplace_file_paths();
    for( auto& path : jplace_files ) {
        path = file_filename( file_basename( path ));
    }
    return jplace_files;
}

std::vector<std::string> JplaceInputOptions::jplace_file_paths() const
{
    // TODO check for file existence
    // TODO use mutable cache vec to avoid recomp.
    // TODO add sample( size_t ) function that just reads one file, using the proper jplace reader settings
    // TODO add/offer validity checks etc
    // TODO offer avg tree option

    if( resolve_paths_ ) {
        return resolve_jplace_paths_();
    } else {
        return jplace_paths_;
    }
}

genesis::placement::SampleSet JplaceInputOptions::sample_set() const
{
    // TODO dont report errors in jplace. offer subcommand for that
    // TODO nope. also report them here. just not while reading, but use a validator function.

    auto reader = genesis::placement::JplaceReader();
    return reader.from_files( jplace_file_paths() );
}

// =================================================================================================
//      Helper Functions
// =================================================================================================

std::vector<std::string> JplaceInputOptions::resolve_jplace_paths_() const
{
    std::vector<std::string> result;
    using namespace genesis::utils;
    for( auto const& path : jplace_paths_ ) {
        if( is_file( path ) ) {

            result.push_back( path );

        } else if( is_dir( path ) ) {

            auto const list = dir_list_files( path, true, ".*\\.jplace$" );
            for( auto const& jplace : list ) {
                result.push_back( jplace );
            }

        } else {
            throw std::runtime_error( "Not a valid file or directory: " + path );
        }
    }
    return result;
}
