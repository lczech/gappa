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

#include "options/file_input.hpp"

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
//      Setup Functions
// =================================================================================================

void FileInputOptions::add_file_input_options(
    CLI::App* sub, std::string const& type, std::string const& extension
){
    // TODO put in options group. same for all other options

    file_type_ = type;
    file_ext_  = extension;

    auto opt_input_files = sub->add_option(
        type + "_files",
        raw_paths_,
        "List of " + type + " files or directories to process"
    );
    opt_input_files->required();
    opt_input_files->check([]( std::string const& path ){
        if( ! genesis::utils::path_exists( path ) ) {
            return std::string( "Path is neither a file nor a directory: " + path );
        }
        return std::string();
    });
}

// =================================================================================================
//      Run Functions
// =================================================================================================

std::vector<std::string> const& FileInputOptions::input_file_paths() const
{
    if( ! resolved_paths_.empty() ) {
        return resolved_paths_;
    }

    using namespace genesis::utils;
    for( auto const& path : raw_paths_ ) {
        if( is_file( path ) ) {

            resolved_paths_.push_back( path );

        } else if( is_dir( path ) ) {

            auto const list = dir_list_files( path, true, ".*\\." + file_ext_ + "$" );
            for( auto const& jplace : list ) {
                resolved_paths_.push_back( jplace );
            }

        } else {
            // throw std::runtime_error( "Not a valid file or directory: " + path );
            throw CLI::ValidationError(
                file_type_ + "_files", "Not a valid file or directory: " + path
            );
        }
    }
    return resolved_paths_;
}

size_t FileInputOptions::input_file_count() const
{
    return input_file_paths().size();
}

std::string const& FileInputOptions::input_file_path( size_t index ) const
{
    auto const& files = input_file_paths();
    if( index >= files.size() ) {
        throw std::runtime_error( "Invalid file index." );
    }
    return files[ index ];
}

void FileInputOptions::input_files_print() const
{
    std::string type = file_type_;
    if( ! type.empty() ) {
        type = " " + type;
    }

    // Print list of files, depending on verbosity.
    auto const& files = input_file_paths();
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

std::vector<std::string> const& FileInputOptions::input_file_cli_paths() const
{
    return raw_paths_;
}

std::vector<std::string> FileInputOptions::input_files_base_file_names() const
{
    using namespace genesis::utils;

    auto paths = input_file_paths();
    for( auto& path : paths ) {
        path = file_filename( file_basename( path ));
    }
    return paths;
}
