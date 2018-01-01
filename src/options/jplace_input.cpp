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

#include "options/jplace_input.hpp"

#include "tools/file_input.hpp"

#include "genesis/placement/formats/jplace_reader.hpp"
#include "genesis/utils/core/fs.hpp"

#include <iostream>
#include <stdexcept>

// =================================================================================================
//      Setup Functions
// =================================================================================================

void JplaceInputOptions::add_jplace_input_options( CLI::App* sub )
{
    // TODO put in options group. same for all other options
    // TODO add avg tree option?!

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

void JplaceInputOptions::print_jplace_input_options() const
{
    print_file_paths( jplace_file_paths(), "jplace" );
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
        return resolve_file_paths( jplace_paths_, "jplace" );
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
