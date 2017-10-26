#ifndef GAPPA_TOOLS_INPUT_H_
#define GAPPA_TOOLS_INPUT_H_

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

#include "genesis/placement/sample.hpp"
#include "genesis/placement/sample_set.hpp"

#include <string>
#include <vector>

// =================================================================================================
//      Functions
// =================================================================================================

/**
 * @brief Given a list of paths, find all jplace files.
 *
 * This function takes a list of paths. For each of them, it checks whether it is a file or a
 * directory. Files are immediately added to the result list, while directories are scanned
 * for any `.jplace` files in them, which are then added to the result list.
 *
 * This allows the users to hand over either their own files (no matter their extension),
 * or whole directory paths for convenience, which then however only use files ending in `.jplace`.
 */
std::vector<std::string> get_jplace_files( std::vector<std::string> const& paths );

/**
 * @brief Read in jplace files and return them as a SampleSet.
 *
 * If the optional parameter @p resolve is set to `true`, the list of paths is first resolved
 * using get_jplace_files(). If left at the default (`false`), the paths are taken as they are.
 */
genesis::placement::SampleSet get_sample_set( std::vector<std::string> const& paths, bool resolve = false );

/**
 * @brief Get the file names of a list of files.
 *
 * This function calls genesis::utils::file_basename() and genesis::utils::file_filename() for
 * all paths. The result is for example useful for user output.
 */
std::vector<std::string> get_file_names( std::vector<std::string> const& paths );

#endif // include guard
