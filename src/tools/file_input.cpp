/*
    gappa - Genesis Applications for Phylogenetic Placement Analysis
    Copyright (C) 2017-2018 Lucas Czech and HITS gGmbH

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

#include "tools/file_input.hpp"

#include "options/global.hpp"

#include "genesis/utils/core/fs.hpp"

#include <iostream>
#include <stdexcept>

// Relative path resolution for printing.
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <linux/limits.h>

// =================================================================================================
//      File Input Tools
// =================================================================================================

std::vector<std::string> resolve_file_paths(
    std::vector<std::string> const& paths, std::string const& extension
) {
    std::vector<std::string> result;
    using namespace genesis::utils;
    for( auto const& path : paths ) {
        if( is_file( path ) ) {

            result.push_back( path );

        } else if( is_dir( path ) ) {

            auto const list = dir_list_files( path, true, ".*\\." + extension + "$" );
            for( auto const& jplace : list ) {
                result.push_back( jplace );
            }

        } else {
            throw std::runtime_error( "Not a valid file or directory: " + path );
        }
    }
    return result;
}

void print_file_paths( std::vector<std::string> const& files, std::string type )
{
    if( ! type.empty() ) {
        type = " " + type;
    }

    if( global_options.verbosity() == 0 ) {
        return;
    } else if( global_options.verbosity() == 1 ) {
        std::cout << "Found " << files.size() << type << " files.\n";
    } else if( global_options.verbosity() == 2 ) {
        std::cout << "Found " << files.size() << type << " files: ";
        for( auto const& file : files ) {
            if( &file != &files[0] ) {
                std::cout << ",  ";
            }
            std::cout << genesis::utils::file_basename( file );
        }
        std::cout << "\n";
    } else {
        std::cout << "Found " << files.size() << type << " files:\n";

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
