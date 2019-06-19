#ifndef GAPPA_COMMANDS_RANDOM_RANDOM_PLACEMENTS_H_
#define GAPPA_COMMANDS_RANDOM_RANDOM_PLACEMENTS_H_

/*
    gappa - Genesis Applications for Phylogenetic Placement Analysis
    Copyright (C) 2017-2019 Lucas Czech and HITS gGmbH

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

#include "CLI/CLI.hpp"

#include "options/file_input.hpp"
#include "options/file_output.hpp"

#include <string>
#include <vector>

// =================================================================================================
//      Options
// =================================================================================================

class RandomPlacementsOptions
{
public:

    // Input data.
    std::string input_tree;
    size_t num_pqueries;

    // Output options.
    FileOutputOptions output;
};

// =================================================================================================
//      Functions
// =================================================================================================

void setup_random_placements( CLI::App& app );
void run_random_placements( RandomPlacementsOptions const& options );

#endif // include guard
