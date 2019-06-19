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

#include "commands/analyze/krd.hpp"

#include "options/global.hpp"
#include "tools/cli_setup.hpp"

#include "CLI/CLI.hpp"

#include "genesis/placement/function/functions.hpp"
#include "genesis/placement/function/emd.hpp"
#include "genesis/placement/function/operators.hpp"

#include "genesis/tree/common_tree/distances.hpp"
#include "genesis/tree/common_tree/functions.hpp"
#include "genesis/tree/function/distances.hpp"
#include "genesis/tree/function/functions.hpp"
#include "genesis/tree/mass_tree/emd.hpp"
#include "genesis/tree/mass_tree/functions.hpp"
#include "genesis/tree/mass_tree/tree.hpp"

#include "genesis/utils/containers/matrix.hpp"
#include "genesis/utils/containers/matrix/operators.hpp"
#include "genesis/utils/io/output_stream.hpp"

#include <cassert>
#include <fstream>

#ifdef GENESIS_OPENMP
#   include <omp.h>
#endif

// =================================================================================================
//      Setup
// =================================================================================================

void setup_krd( CLI::App& app )
{
    // Create the options and subcommand objects.
    auto opt = std::make_shared<KrdOptions>();
    auto sub = app.add_subcommand(
        "krd",
        "Calcualte the pairwise Kantorovich-Rubinstein (KR) distance matrix between samples."
    );

    // File input
    opt->jplace_input.add_jplace_input_opt_to_app( sub );

    // Exponent for kr integration
    sub->add_option(
        "--exponent",
        opt->exponent,
        "Exponent for KR integration.",
        true
    )->group( "Settings" );

    // Normalize to tree length
    sub->add_flag(
        "--normalize",
        opt->normalize,
        "Divide the KR distance by the tree length to get normalized values."
    )->group( "Settings" );

    // Further input settings
    opt->jplace_input.add_point_mass_opt_to_app( sub );
    opt->jplace_input.add_ignore_multiplicities_opt_to_app( sub );

    // Output
    opt->matrix_output.add_matrix_output_opts_to_app( sub );

    // Set the run function as callback to be called when this subcommand is issued.
    // Hand over the options by copy, so that their shared ptr stays alive in the lambda.
    sub->callback( gappa_cli_callback(
        sub,
        {
            "Matsen2011-edgepca-and-squash-clustering"
        },
        [ opt ]() {
            run_krd( *opt );
        }
    ));
}

// =================================================================================================
//      Run
// =================================================================================================

void run_krd( KrdOptions const& options )
{
    using namespace genesis;
    using namespace genesis::placement;
    using namespace genesis::tree;
    using namespace genesis::utils;

    // Check if any of the files we are going to produce already exists. If so, fail early.
    options.matrix_output.check_nonexistent_output_files();

    // Print some user output.
    options.jplace_input.print();

    // Base check
    if( options.jplace_input.file_count() < 2 ) {
        throw std::runtime_error( "Cannot run krd with fewer than 2 samples." );
    }

    // Read files.
    auto const mass_trees = options.jplace_input.mass_tree_set();

    // Calculate result matrix.
    LOG_MSG1 << "Calculating pairwise KR distances.";
    auto krd_matrix = earth_movers_distance( mass_trees, options.exponent );

    // Normalize by tree length if necessary.
    if( options.normalize ) {
        assert( mass_trees.size() > 0 );
        auto const len = length( mass_trees[0] );
        for( auto& e : krd_matrix ) {
            e /= len;
        }
    }

    // Write output matrix in the specified format
    LOG_MSG1 << "Writing distance matrix.";
    auto const names = options.jplace_input.base_file_names();
    options.matrix_output.write_matrix( krd_matrix, names, names, "Sample" );
}
