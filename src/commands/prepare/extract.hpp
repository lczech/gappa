#ifndef GAPPA_COMMANDS_PREPARE_EXTRACT_H_
#define GAPPA_COMMANDS_PREPARE_EXTRACT_H_

/*
    gappa - Genesis Applications for Phylogenetic Placement Analysis
    Copyright (C) 2017-2025 Lucas Czech

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
    Lucas Czech <lucas.czech@sund.ku.dk>
    University of Copenhagen, Globe Institute, Section for GeoGenetics
    Oster Voldgade 5-7, 1350 Copenhagen K, Denmark
*/

#include "CLI/CLI.hpp"

#include "options/file_input.hpp"
#include "options/file_output.hpp"
#include "options/jplace_input.hpp"
#include "options/sequence_input.hpp"

#include <string>
#include <vector>

// =================================================================================================
//      Options
// =================================================================================================

class ExtractOptions
{
public:

    std::string          clade_list_file;
    JplaceInputOptions   jplace_input;
    SequenceInputOptions sequence_input;

    std::string          color_tree_file;
    FileOutputOptions    jplace_output;
    FileOutputOptions    sequence_output;

    double threshold = 0.95;
    bool exclude_clade_stems = false;
    std::string basal_clade_name = "basal";
    std::string uncertain_clade_name = "uncertain";
};

// =================================================================================================
//      Functions
// =================================================================================================

void setup_extract( CLI::App& app );
void run_extract( ExtractOptions const& options );

#endif // include guard
