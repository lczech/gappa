#ifndef GAPPA_TOOLS_FILE_INPUT_H_
#define GAPPA_TOOLS_FILE_INPUT_H_

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

#include <string>
#include <vector>

// =================================================================================================
//      File Input Tools
// =================================================================================================

/**
 * @brief Given a list of paths, find all files with a certain extension.
 *
 * This function takes the list of paths. For each of them, it checks whether it is a file or a
 * directory. Files are immediately added to the result list, while directories are scanned
 * for any files with the extension in them, which are then added to the result list.
 *
 * This allows the users to hand over either their own files (no matter their extension), or
 * whole directory paths for convenience, which then however only use files ending in the extension.
 */
std::vector<std::string> resolve_file_paths(
    std::vector<std::string> const& paths, std::string const& extension
);

void print_file_paths( std::vector<std::string> const& files, std::string type = "" );

#endif // include guard
