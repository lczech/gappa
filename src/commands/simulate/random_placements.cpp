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
    Lucas Czech <lucas.czech@h-its.org>
    Exelixis Lab, Heidelberg Institute for Theoretical Studies
    Schloss-Wolfsbrunnenweg 35, D-69118 Heidelberg, Germany
*/

#include "commands/simulate/random_placements.hpp"

#include "options/global.hpp"
#include "tools/cli_setup.hpp"
#include "tools/misc.hpp"

#include "CLI/CLI.hpp"

#include "genesis/placement/formats/jplace_writer.hpp"
#include "genesis/placement/function/operators.hpp"
#include "genesis/placement/sample.hpp"
#include "genesis/placement/simulator/distributions.hpp"
#include "genesis/placement/simulator/functions.hpp"
#include "genesis/placement/simulator/simulator.hpp"
#include "genesis/tree/common_tree/newick_reader.hpp"
#include "genesis/tree/common_tree/tree.hpp"
#include "genesis/tree/formats/newick/reader.hpp"
#include "genesis/tree/function/functions.hpp"
#include "genesis/tree/iterator/preorder.hpp"
#include "genesis/tree/tree.hpp"
#include "genesis/tree/tree/subtree.hpp"
#include "genesis/utils/io/input_source.hpp"
#include "genesis/utils/io/output_target.hpp"

#include <cassert>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

// =================================================================================================
//      Setup
// =================================================================================================

void setup_random_placements( CLI::App& app )
{
    // Create the options and subcommand objects.
    auto opt = std::make_shared<RandomPlacementsOptions>();
    auto sub = app.add_subcommand(
        "random-placements",
        "Create a set of random phylogenetic placements on a given reference tree."
    );

    // -----------------------------------------------------------
    //     Input Data
    // -----------------------------------------------------------

    // Reference tree
    auto input_tree_opt = sub->add_option(
        "--reference-tree",
        opt->input_tree,
        "File containing a reference tree in newick format."
    );
    input_tree_opt->group( "Input" );
    input_tree_opt->required();

    // Number of pqueries
    auto num_pqueries_opt = sub->add_option(
        "--pquery-count",
        opt->num_pqueries,
        "Number of pqueries to create."
    );
    num_pqueries_opt->group( "Input" );
    num_pqueries_opt->required();

    // Number of pqueries
    auto subtree_opt = sub->add_option(
        "--subtree",
        opt->subtree,
        "If given, only generate random placements in one of the subtrees of the root node. "
        "For example, if the root is a trifurcation, values 0-2 are allowed."
    );
    subtree_opt->group( "Input" );

    // -----------------------------------------------------------
    //     Output Options
    // -----------------------------------------------------------

    opt->file_output.add_default_output_opts_to_app( sub );
    opt->file_output.add_file_compress_opt_to_app( sub );

    // -----------------------------------------------------------
    //     Callback
    // -----------------------------------------------------------

    // Set the run function as callback to be called when this subcommand is issued.
    // Hand over the options by copy, so that their shared ptr stays alive in the lambda.
    sub->callback( gappa_cli_callback(
        sub,
        {},
        [ opt ]() {
            run_random_placements( *opt );
        }
    ));
}

// =================================================================================================
//      Helper Functions
// =================================================================================================

/**
 * @brief Read the reference tree and return a sample with that tree, but wihtout any placements.
 */
genesis::placement::Sample get_empty_sample( RandomPlacementsOptions const& options )
{
    using namespace genesis;

    auto const common_tree = tree::CommonTreeNewickReader().read(
        utils::from_file( options.input_tree )
    );
    auto const placement_tree = placement::convert_common_tree_to_placement_tree( common_tree );
    return placement::Sample( placement_tree );
}

// =================================================================================================
//      Run
// =================================================================================================

void run_random_placements( RandomPlacementsOptions const& options )
{
    using namespace ::genesis;
    using namespace ::genesis::placement;
    using namespace ::genesis::tree;

    // Check if the output file name already exists. If so, fail early.
    options.file_output.check_output_files_nonexistence( "random-placements", "jplace" );

    // Get an empty sample with the reference tree.
    auto sample = get_empty_sample( options );

    // Init the simulator to some good values.
    Simulator sim;
    sim.extra_placement_distribution().placement_number_weights = { 1.0, 1.0, 1.0, 1.0, 1.0 };
    sim.extra_placement_distribution().placement_path_length_weights = { 0.0, 4.0, 3.0, 2.0, 1.0 };
    sim.like_weight_ratio_distribution().intervals = { 0.0, 1.0 };
    sim.like_weight_ratio_distribution().weights = { 0.0, 1.0 };

    // Only simulate in certain subtrees.
    if( options.subtree > -1 ) {
        auto const degr = static_cast<int>( degree( sample.tree().root_node() ));
        if( options.subtree >= degr ) {
            throw CLI::ValidationError(
                "--subtree (" + std::to_string( options.subtree ) +  ")",
                "Invalid value; has to be between 0 and " + std::to_string( degr - 1 ) +
                " for the given tree."
            );
        }

        // Find the correct subtree.
        auto link = &sample.tree().root_link();
        for( size_t i = 0; i < static_cast<size_t>( options.subtree ); ++i ) {
            link = & link->next();
        }
        auto const subtr = Subtree( link->outer() );

        // Build an edge vector that only has weights in the subtree.
        auto& edge_weights = sim.edge_distribution().edge_weights;
        edge_weights = std::vector<double>( sample.tree().edge_count(), 0.0 );
        for( auto it : preorder( subtr )) {
            edge_weights[ it.edge().index() ] = 1.0;
        }
        // Also set the root edge to 1.
        edge_weights[ subtr.edge().index() ] = 1.0;
    }

    // Generate pqueries.
    sim.generate( sample, options.num_pqueries );

    // Write result file.
    JplaceWriter().write(
        sample,
        options.file_output.get_output_target( "random-placements", "jplace" )
    );
}
