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

#include "commands/common/kmeans.hpp"

#include "options/global.hpp"

#include "CLI/CLI.hpp"

#include "genesis/utils/io/output_stream.hpp"
#include "genesis/utils/text/string.hpp"

#ifdef GENESIS_OPENMP
#   include <omp.h>
#endif

// =================================================================================================
//      Functions
// =================================================================================================

std::vector<size_t> get_k_values( KmeansOptions const& options )
{
    using namespace genesis::utils;

    // Prepare an exception with nice text.
    auto excpt = CLI::ValidationError(
        "--k (" + options.ks +  ")",
        "Invalid list of values for k. Needs to be a comma-separated list of positive numbers or "
        "ranges, e.g., 5-10,12,15"
    );

    // Try to get the ks by splitting the list.
    std::vector<size_t> result;
    try {
        result = split_range_list( options.ks );
    } catch ( ... ) {
        throw excpt;
    }

    // Additional condition: need numbers, but no zero.
    if( result.size() == 0 || result[0] == 0 ) {
        throw excpt;
    }
    return result;
}

void write_assignment_file(
     KmeansOptions const& options,
     std::vector<size_t> const& assignments,
     size_t k
) {
    auto const set_size = options.jplace_input.file_count();

    // Saftey
    if( assignments.size() != set_size ) {
        throw std::runtime_error(
            "Internal Error: Differing number of assignments (" + std::to_string( assignments.size() ) +
            ") and sample set size (" + std::to_string( set_size ) + ")."
        );
    }


    // Prepare assignments file.
    // TODO check with file overwrite settings
    auto const assm_fn
        = options.file_output.out_dir() + options.file_output.file_prefix()
        + "k_" + std::to_string( k ) + "_assignments.csv"
    ;
    std::ofstream assm_os;
    genesis::utils::file_output_stream( assm_fn, assm_os );

    // Write assignments
    for( size_t fi = 0; fi < set_size; ++fi ) {
        assm_os << options.jplace_input.base_file_name( fi );
        assm_os << "\t" << assignments[fi];
        assm_os << "\n";
    }
}
