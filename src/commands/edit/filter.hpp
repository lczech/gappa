#ifndef GAPPA_COMMANDS_EDIT_FILTER_H_
#define GAPPA_COMMANDS_EDIT_FILTER_H_

/*
    gappa - Genesis Applications for Phylogenetic Placement Analysis
    Copyright (C) 2017-2022 Lucas Czech

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
    Lucas Czech <lczech@carnegiescience.edu>
    Department of Plant Biology, Carnegie Institution For Science
    260 Panama Street, Stanford, CA 94305, USA
*/

#include "CLI/CLI.hpp"

#include "options/jplace_input.hpp"
#include "options/file_output.hpp"
#include "tools/cli_option.hpp"

#include <string>
#include <vector>

// =================================================================================================
//      Options
// =================================================================================================

class FilterOptions
{
public:

    JplaceInputOptions jplace_input;
    // CliOption<bool>    merge_input;

    // Before filter processing
    CliOption<bool> normalize_before;

    // Placement property filters
    CliOption<double> min_accumulated_mass;
    CliOption<double> min_mass_threshold;
    CliOption<size_t> max_n_placements;
    CliOption<double> min_pendant_len;
    CliOption<double> max_pendant_len;

    // After filter processing
    CliOption<bool> normalize_after;
    CliOption<bool> no_remove_empty;

    // Name filters
    CliOption<std::string> keep_names;
    CliOption<std::string> remove_names;

    FileOutputOptions jplace_output;
};

// =================================================================================================
//      Functions
// =================================================================================================

void setup_filter( CLI::App& app );
void run_filter( FilterOptions const& options );

#endif // include guard
