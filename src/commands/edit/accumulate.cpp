/*
    gappa - Genesis Applications for Phylogenetic Placement Analysis
    Copyright (C) 2017-2020 Lucas Czech and HITS gGmbH

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

#include "commands/edit/accumulate.hpp"

#include "options/global.hpp"
#include "tools/cli_setup.hpp"

#include "CLI/CLI.hpp"

#include "genesis/placement/formats/jplace_writer.hpp"
#include "genesis/placement/function/functions.hpp"
#include "genesis/placement/function/masses.hpp"
#include "genesis/tree/function/functions.hpp"
#include "genesis/tree/iterator/postorder.hpp"
#include "genesis/utils/io/output_target.hpp"

#include <cassert>
#include <limits>
#include <utility>

#ifdef GENESIS_OPENMP
#   include <omp.h>
#endif

// =================================================================================================
//      Setup
// =================================================================================================

void setup_accumulate( CLI::App& app )
{
    // Create the options and subcommand objects.
    auto options = std::make_shared<AccumulateOptions>();
    auto sub = app.add_subcommand(
        "accumulate",
        "Accumulate the masses of each query in jplace files into basal branches "
        "so that they exceed a given mass threshold."
    );

    // -----------------------------------------------------------
    //     Input options
    // -----------------------------------------------------------

    // Jplace input
    options->jplace_input.add_jplace_input_opt_to_app( sub );

    // Accumulate file
    auto threshold_opt = sub->add_option(
        "--threshold",
        options->threshold,
        "Threshold of how much mass needs to be accumulated into a basal branch.",
        true
    );
    threshold_opt->group( "Settings" );
    threshold_opt->check( CLI::Range( 0.5, 1.0 ));

    // -----------------------------------------------------------
    //     Output options
    // -----------------------------------------------------------

    options->jplace_output.add_output_dir_opt_to_app( sub );
    options->jplace_output.add_file_prefix_opt_to_app( sub );

    // -----------------------------------------------------------
    //     Callback
    // -----------------------------------------------------------

    // Set the run function as callback to be called when this subcommand is issued.
    // Hand over the options by copy, so that their shared ptr stays alive in the lambda.
    sub->callback( gappa_cli_callback(
        sub,
        {},
        [ options ]() {
            run_accumulate( *options );
        }
    ));
}

// =================================================================================================
//      Run
// =================================================================================================

void run_accumulate( AccumulateOptions const& options )
{
    using namespace genesis;
    using namespace genesis::placement;
    using namespace genesis::tree;

    // Check if any of the files we are going to produce already exists. If so, fail early.
    options.jplace_output.check_nonexistent_output_files({
        options.jplace_output.file_prefix() + "accumulated\\.jplace"
    });

    // Print some user output.
    options.jplace_input.print();

    // User output.
    LOG_MSG1 << "Reading sample" << ( options.jplace_input.file_count() > 1 ? "s" : "" ) << ".";

    // Get all queries of all samples. Requires that all have the same ref tree.
    // Normalize each pquery to a mass of 1.0, which is reasonable for the threshold.
    auto sample = options.jplace_input.merged_samples();
    normalize_weight_ratios( sample );
    auto& tree = sample.tree();

    // We will delete the pqueries that cannot be accumulated. Store them here.
    std::vector<size_t> removal_collector;

    // Replace the placements of each pquery by one placement
    // that accumulates the mass at a basal branch.
    for( size_t i = 0; i < sample.size(); ++i ) {
        auto& pqry = sample.at(i);

        // We need the masses per edge of the pquery.
        auto masses = std::vector<double>( tree.edge_count(), 0.0 );

        // Furthermore, store the weighted pendant length of all placements in the subtree.
        // We want the resulting pendant length to be the weighted average of all pendant lengths
        // in the accumulated subtree. So, we propagate the weighted sum of pend lengths up the tree,
        // and divide by the sum of weights in the end to get the average.
        // This sum of weights happens to be the sum of lwrs that is already stored in masses[].
        auto pendant = std::vector<double>( tree.edge_count(), 0.0 );

        // Init both with the values from the pquery.
        for( auto const& place : pqry.placements() ) {
            assert( masses[ place.edge().index() ] == 0.0 );
            masses[ place.edge().index() ] = place.like_weight_ratio;
            pendant[ place.edge().index() ] = place.like_weight_ratio * place.pendant_length;
        }

        // Move the masses up the tree until they exceed the threshold.
        bool exceeded_threshold = false;
        size_t result_edge = std::numeric_limits<size_t>::max();
        for( auto it : postorder( tree )) {

            // We are interested in edges, so skip the last iteration.
            if( it.is_last_iteration() ) {
                continue;
            }

            auto& cur_mass = masses[ it.edge().index() ];
            auto& cur_pend = pendant[ it.edge().index() ];

            // Add subtree masses and pendant lengths.
            auto link = &it.link().next();
            while( link != &it.link() ) {
                cur_mass += masses[ link->edge().index() ];
                cur_pend += pendant[ link->edge().index() ];
                link = &link->next();
            }

            // Check result.
            if( cur_mass >= options.threshold ) {
                exceeded_threshold = true;
                result_edge = it.edge().index();
                break;
            }
        }
        assert( result_edge < std::numeric_limits<size_t>::max() );

        // If there is a branch where the accumualtion worked, use it.
        // If not, put on removal list! This can happen if the masses are distributed across different
        // directions from the root - in that case, we do not consider this a valid accumulation.
        if( exceeded_threshold ) {
            pqry.clear_placements();
            auto& place = pqry.add_placement( tree.edge_at( result_edge ));

            // Set a new lwr of 1.0, as we throw away everything else.
            place.like_weight_ratio = 1.0;

            // Set the pendant length to the weighted average of the lengths in the subtree.
            place.pendant_length = pendant[ result_edge ] / masses[ result_edge ];

            // Set the position along the branch. It does not really matter that much,
            // so do a nice little thing and set it at the position along the branch
            // that corresponds to the accumulated mass. So, if the placement represents 96% of
            // the mass, it is placed at 0.96 along the branch.
            // This way, pqueries get distinct prox lengths, which might help in downstream
            // analyses.
            auto const bl = tree.edge_at( result_edge ).data<CommonEdgeData>().branch_length;
            assert( masses[ result_edge] >= options.threshold );
            place.proximal_length = bl * masses[ result_edge ];
        } else {
            removal_collector.push_back( i );
        }
    }

    // User output about the removal.
    if( ! removal_collector.empty() ) {
        LOG_MSG1 << "The following pquries have their placement masses distributed "
                 << "across clades in different directions away from the root, "
                 << "so that they could not be properly accumulated into basal branches:";
    }

    // Now delete the non accumulated pqueries in order to get a non-confusing result.
    // We go backwards, so that the indices remain stable after deletion.
    for( auto it = removal_collector.rbegin(); it != removal_collector.rend(); ++it ) {

        // Some user output.
        auto const& pqry = sample.at( *it );
        for( auto const& name : pqry.names() ) {
            LOG_MSG1 << " - " << name.name;
        }

        // Do the deletion!
        sample.remove( *it );
    }

    // More user output about the removal.
    if( ! removal_collector.empty() ) {
        LOG_MSG1 << "Those pqueries are removed from the output!";
    }

    // Write the new sample to a file.
    std::string const filename = options.jplace_output.file_prefix() + "accumulated.jplace";
    JplaceWriter().write(
        sample,
        genesis::utils::to_file( options.jplace_output.out_dir() + filename )
    );
}
