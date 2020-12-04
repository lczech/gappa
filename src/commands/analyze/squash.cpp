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

#include "commands/analyze/squash.hpp"

#include "options/global.hpp"
#include "tools/cli_setup.hpp"

#include "CLI/CLI.hpp"

#include "genesis/placement/function/operators.hpp"
#include "genesis/placement/function/functions.hpp"
#include "genesis/tree/mass_tree/functions.hpp"
#include "genesis/tree/mass_tree/squash_clustering.hpp"
#include "genesis/utils/core/std.hpp"
#include "genesis/utils/io/output_stream.hpp"

#include <fstream>

// =================================================================================================
//      Setup
// =================================================================================================

void setup_squash( CLI::App& app )
{
    // Create the options and subcommand objects.
    auto opt = std::make_shared<SquashOptions>();
    auto sub = app.add_subcommand(
        "squash",
        "Perform Squash Clustering for a set of samples."
    );

    // Add common options.
    opt->jplace_input.add_jplace_input_opt_to_app( sub );

    // Add custom options.
    sub->add_option(
        "--exponent",
        opt->exponent,
        "Exponent for KR integration.",
        true
    )->group( "Settings" );


    // Other jplace settings
    opt->jplace_input.add_point_mass_opt_to_app( sub );
    opt->jplace_input.add_ignore_multiplicities_opt_to_app( sub );

    // Color.
    opt->color_map.add_color_list_opt_to_app( sub, "BuPuBk" );
    opt->color_norm.add_log_scaling_opt_to_app( sub );

    // Output options.
    opt->file_output.add_default_output_opts_to_app( sub );
    opt->tree_output.add_tree_output_opts_to_app( sub );

    // Set the run function as callback to be called when this subcommand is issued.
    // Hand over the options by copy, so that their shared ptr stays alive in the lambda.
    sub->callback( gappa_cli_callback(
        sub,
        {
            "Matsen2011-edgepca-and-squash-clustering"
        },
        [ opt ]() {
            run_squash( *opt );
        }
    ));
}

// =================================================================================================
//      Run
// =================================================================================================

void run_squash( SquashOptions const& options )
{
    using namespace genesis;

    if( options.exponent <= 0.0 ) {
        throw CLI::ValidationError(
            "--exponent (" + std::to_string( options.exponent ) +  ")",
            "Invalid exponent value for KR distance calculation. Has to be > 0.0."
        );
    }

    // Check if any of the files we are going to produce already exists. If so, fail early.
    std::vector<std::pair<std::string, std::string>> files_to_check;
    files_to_check.push_back({ "cluster", "newick" });
    for( auto const& e : options.tree_output.get_extensions() ) {
        files_to_check.push_back({ "tree_*", e });
    }
    options.file_output.check_output_files_nonexistence( files_to_check );

    // User is warned when not using any tree outputs.
    options.tree_output.check_tree_formats();

    // Print some user output.
    options.jplace_input.print();

    // Get color map and norm.
    auto color_map  = options.color_map.color_map();
    auto color_norm = options.color_norm.get_sequential_norm();

    // Read in the trees and immediately convert them to mass trees to save storage.
    auto mass_trees = options.jplace_input.mass_tree_set();

    // Set up squash clustering.
    auto sc = tree::SquashClustering();
    sc.p( options.exponent );
    sc.report_initialization = [&](){
        LOG_MSG2 << " - Initializing";
    };
    sc.report_step = [&]( size_t i, size_t total ){
        LOG_MSG2 << " - Step " << i << " of " << total;
    };
    sc.write_cluster_tree = [&]( tree::MassTree const& cluster_tree, size_t index ){
        // Prepare colors
        auto const masses = tree::mass_tree_mass_per_edge( cluster_tree );
        color_norm->autoscale_max( masses );

        // Now, make a color vector and write to files.
        auto const colors = color_map( *color_norm, masses );
        options.tree_output.write_tree_to_files(
            cluster_tree,
            colors,
            color_map,
            *color_norm,
            options.file_output,
            "tree_" + std::to_string( index )
        );
    };

    // Run, Forrest, run!
    LOG_MSG1 << "Running Squash Clustering";
    sc.run( std::move( mass_trees ) );

    LOG_MSG1 << "Writing output files";

    // Write output cluster tree to newick.
    auto file_clust_tree = options.file_output.get_output_target( "cluster", "newick" );
    (*file_clust_tree) << sc.tree_string( options.jplace_input.base_file_names() );
}
