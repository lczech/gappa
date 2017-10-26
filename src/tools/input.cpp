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

#include "tools/input.hpp"

#include "genesis/placement/formats/jplace_reader.hpp"
#include "genesis/utils/core/fs.hpp"

#include <stdexcept>

// =================================================================================================
//      Functions
// =================================================================================================

std::vector<std::string> get_jplace_files( std::vector<std::string> const& paths )
{
    std::vector<std::string> result;
    using namespace genesis::utils;
    for( auto const& path : paths ) {
        if( is_file( path ) ) {
            result.push_back( path );
        } else if( is_dir( path ) ) {
            auto const list = dir_list_files( path, ".*\\.jplace" );
            for( auto const& jplace : list ) {
                result.push_back( jplace );
            }
        } else {
            throw std::runtime_error( "Not a valid file or directory: " + path );
        }
    }
    return result;
}

genesis::placement::SampleSet get_sample_set( std::vector<std::string> const& paths, bool resolve )
{
    // Slightly inefficient due to copying. But we don't expect billions of files.
    std::vector<std::string> jplace_files;
    if( resolve ) {
        jplace_files = get_jplace_files( paths );
    } else {
        jplace_files = paths;
    }

    auto reader = genesis::placement::JplaceReader();
    // TODO dont report errors in jplace. offer subcommand for that
    return reader.from_files( jplace_files );
}

std::vector<std::string> get_file_names( std::vector<std::string> const& paths )
{
    std::vector<std::string> result;
    using namespace genesis::utils;
    for( auto const& path : paths ) {
        result.push_back( file_filename( file_basename( path )));
    }
    return result;
}
