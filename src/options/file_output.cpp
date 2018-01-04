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

#include "options/file_output.hpp"

#include "genesis/utils/core/fs.hpp"

#include <algorithm>

// =================================================================================================
//      Setup Functions
// =================================================================================================

void FileOutputOptions::add_output_dir_options( CLI::App* sub )
{
    auto opt_out_dir = sub->add_option(
        "--out-dir",
        out_dir_,
        "Directory to write files to",
        true
    );
    opt_out_dir->check( CLI::ExistingDirectory );
    opt_out_dir->group( output_files_group_name() );

    // TODO instead of expecting an existing dir, create it if needed.
    // TODO add function to overwrite files, which sets the genesis option for this
}

// =================================================================================================
//      Run Functions
// =================================================================================================

std::string FileOutputOptions::out_dir() const
{
    return genesis::utils::dir_normalize_path( out_dir_ );
}

void FileOutputOptions::check_nonexistent_output_files( std::vector<std::string> const& filenames ) const
{
    using namespace genesis::utils;

    // Check if any of the files exists. Old version without regex.
    // std::string const dir = dir_normalize_path( out_dir_ );
    // for( auto const& file : filenames ) {
    //     if( file_exists( dir + file ) ) {
    //         throw CLI::ValidationError(
    //             "--out-dir (" + out_dir_ +  ")", "Output file already exists: " + file
    //         );
    //     }
    // }

    // Check if any of the files exists.
    for( auto const& file : filenames ) {
        auto const dir_cont = dir_list_contents( out_dir_, true, file );
        if( ! dir_cont.empty() ) {
            throw CLI::ValidationError(
                "--out-dir (" + out_dir_ +  ")", "Output path already exists: " + file
            );
        }
    }

    // Check if any file name is duplicated.
    // If so, we will have a problem after the first file has been written.
    auto cpy = filenames;
    std::sort( cpy.begin(), cpy.end() );
    auto const adj = std::adjacent_find( cpy.begin(), cpy.end() ) ;
    if( adj != cpy.end() ) {
        throw CLI::ValidationError(
            "--out-dir", "Output file name used multiple times: " + ( *adj )
        );
    }
}
