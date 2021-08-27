#ifndef GAPPA_COMMANDS_EXAMINE_LWR_DISTRIBUTION_H_
#define GAPPA_COMMANDS_EXAMINE_LWR_DISTRIBUTION_H_

/*
    gappa - Genesis Applications for Phylogenetic Placement Analysis
    Copyright (C) 2017-2021 Lucas Czech

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

#include <string>
#include <vector>

// =================================================================================================
//      Options
// =================================================================================================

class LwrDistributionOptions
{
public:

    size_t num_entries      = 100;
    size_t num_lwrs         = 5;
    bool   numerical_sort   = false;
    bool   no_compat_check  = false;

    JplaceInputOptions jplace_input;
    FileOutputOptions  file_output;
};

// =================================================================================================
//      Functions
// =================================================================================================

void setup_lwr_distribution( CLI::App& app );
void run_lwr_distribution( LwrDistributionOptions const& options );

#endif // include guard
