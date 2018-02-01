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
#include <stdexcept>

// =================================================================================================
//      Setup Functions
// =================================================================================================

void FileOutputOptions::add_to_app( CLI::App* sub )
{
    add_to_app( sub, "", "Directory to write files to", "." );
}

void FileOutputOptions::add_to_app(
    CLI::App* sub,
    std::string const& name,
    std::string const& description,
    std::string const& initial_value
) {
    auto const optname = "--" + name + ( name.empty() ? "" : "-" ) + "out-dir";

    if( out_dirs_.count( name ) > 0 ) {
        throw std::domain_error(
            "Output dir '" + optname + "dir' added multipe times to options."
        );
    }

    // Add default entry.
    out_dirs_[ name ] = initial_value;

    // Add option
    auto opt_out_dir = sub->add_option(
        optname,
        out_dirs_[ name ],
        description,
        true
    );
    opt_out_dir->check( CLI::ExistingDirectory );
    opt_out_dir->group( group_name() );

    // TODO instead of expecting an existing dir, create it if needed.
    // TODO add function to overwrite files, which sets the genesis option for this
}

// =================================================================================================
//      Run Functions
// =================================================================================================

std::string FileOutputOptions::out_dir() const
{
    return out_dir( "" );
}

std::string FileOutputOptions::out_dir( std::string const& name ) const
{
    if( out_dirs_.count( name ) == 0 ) {
        auto const optname = "--" + name + ( name.empty() ? "" : "-" ) + "out-dir";
        throw std::domain_error(
            "Output dir '" + optname + "' not part of the options."
        );
    }
    return genesis::utils::dir_normalize_path( out_dirs_.at( name ));
}

void FileOutputOptions::check_nonexistent_output_files(
    std::vector<std::string> const& filenames
) const {
    return check_nonexistent_output_files( filenames, "" );
}

void FileOutputOptions::check_nonexistent_output_files(
    std::vector<std::string> const& filenames, std::string const& name
) const {
    using namespace genesis::utils;

    // Get basic strings
    auto const outdir = out_dir( name );
    auto const optname = "--" + name + ( name.empty() ? "" : "-" ) + "out-dir";

    // Check if any of the files exists. Old version without regex.
    // std::string const dir = dir_normalize_path( outdir );
    // for( auto const& file : filenames ) {
    //     if( file_exists( dir + file ) ) {
    //         throw CLI::ValidationError(
    //             "--out-dir (" + outdir +  ")", "Output file already exists: " + file
    //         );
    //     }
    // }

    // Check if any of the files exists.
    for( auto const& file : filenames ) {
        auto const dir_cont = dir_list_contents( outdir, true, file );
        if( ! dir_cont.empty() ) {
            throw CLI::ValidationError(
                optname + " (" + outdir +  ")", "Output path already exists: " + file
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
            optname, "Output file name used multiple times: " + ( *adj )
        );
    }

    // TODO there is a change that multiple named output dirs are set to the same real dir,
    // and that then files with the same names are written.
}
