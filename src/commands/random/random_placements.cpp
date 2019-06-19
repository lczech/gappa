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

#include "commands/random/random_placements.hpp"

#include "options/global.hpp"
#include "tools/cli_setup.hpp"
#include "tools/misc.hpp"

#include "CLI/CLI.hpp"

#include "genesis/placement/function/operators.hpp"
#include "genesis/placement/sample.hpp"
#include "genesis/placement/simulator/distributions.hpp"
#include "genesis/placement/simulator/functions.hpp"
#include "genesis/placement/simulator/simulator.hpp"
#include "genesis/placement/formats/jplace_writer.hpp"
#include "genesis/tree/common_tree/newick_reader.hpp"
#include "genesis/tree/common_tree/tree.hpp"
#include "genesis/tree/formats/newick/reader.hpp"
#include "genesis/tree/tree.hpp"
#include "genesis/utils/io/input_source.hpp"

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
        "placements",
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

    // -----------------------------------------------------------
    //     Output Options
    // -----------------------------------------------------------

    opt->output.add_output_dir_opt_to_app( sub );
    opt->output.add_file_prefix_opt_to_app( sub );

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

    // Check if the output file name already exists. If so, fail early.
    options.output.check_nonexistent_output_files({
        options.output.file_prefix() + "random-placements.jplace"
    });


    // Get an empty sample with the reference tree.
    auto sample = get_empty_sample( options );

    // Init the simulator to some good values.
    Simulator sim;
    sim.extra_placement_distribution().placement_number_weights = { 1.0, 1.0, 1.0, 1.0, 1.0 };
    sim.extra_placement_distribution().placement_path_length_weights = { 0.0, 4.0, 3.0, 2.0, 1.0 };
    sim.like_weight_ratio_distribution().intervals = { 0.0, 1.0 };
    sim.like_weight_ratio_distribution().weights = { 0.0, 1.0 };

    // Generate pqueries.
    sim.generate( sample, options.num_pqueries );

    // Write result file.
    JplaceWriter().to_file(
        sample,
        options.output.file_prefix() + "random-placements.jplace"
    );
}
