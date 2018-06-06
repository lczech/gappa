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

#include "commands/analyze/ikmeans.hpp"

#include "options/global.hpp"

#include "CLI/CLI.hpp"

#include "genesis/placement/function/epca.hpp"
#include "genesis/placement/function/operators.hpp"
#include "genesis/tree/function/tree_set.hpp"
#include "genesis/tree/mass_tree/emd.hpp"
#include "genesis/tree/mass_tree/functions.hpp"
#include "genesis/tree/mass_tree/kmeans.hpp"
#include "genesis/tree/mass_tree/tree.hpp"
#include "genesis/utils/io/output_stream.hpp"
#include "genesis/utils/text/string.hpp"

#include <fstream>

#ifdef GENESIS_OPENMP
#   include <omp.h>
#endif

// =================================================================================================
//      Setup
// =================================================================================================

void setup_ikmeans( CLI::App& app )
{
    // Create the options and subcommand objects.
    auto opt = std::make_shared<IkmeansOptions>();
    auto sub = app.add_subcommand(
        "kmeans",
        "Run Imbalance k-means clustering on a set of samples."
    );

    // Add common options.
    opt->jplace_input.add_jplace_input_opt_to_app( sub );
    opt->jplace_input.add_point_mass_opt_to_app( sub );
    opt->jplace_input.add_ignore_multiplicities_opt_to_app( sub );

    // Number of clusters to find.
    auto k_opt = sub->add_option(
        "-k,--k",
        opt->ks,
        "Number of clusters to find. Can be a comma-separated list of multiple values or "
        "ranges for k: 1-5,8,10,12",
        true
    );
    k_opt->group( "Settings" );
    k_opt->required();

    // Color.
    opt->color_map.add_color_list_opt_to_app( sub, "spectral" );

    // Output files.
    opt->tree_output.add_tree_output_opts_to_app( sub );
    opt->file_output.add_output_dir_opt_to_app( sub );
    opt->file_output.add_file_prefix_opt_to_app( sub, "", "ikmeans" );

    // Set the run function as callback to be called when this subcommand is issued.
    // Hand over the options by copy, so that their shared ptr stays alive in the lambda.
    sub->set_callback( [ opt ]() {
        run_ikmeans( *opt );
    });
}

// =================================================================================================
//      Helper Functions
// =================================================================================================

std::vector<size_t> get_k_values( IkmeansOptions const& options )
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
     IkmeansOptions const& options,
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

void write_cluster_trees(
    IkmeansOptions const& options,
    genesis::tree::Tree const& tree,
    std::vector<size_t> columns,
    std::vector<std::vector<double>> const& centroids,
    size_t k
) {
    using namespace genesis;
    using namespace genesis::utils;

    if( centroids.size() != k ) {
        throw std::runtime_error(
            "Internal Error: Differing number of centroids (" + std::to_string( centroids.size() ) +
            ") and  k (" + std::to_string( k ) + ")."
        );
    }

    // Get color map and norm.
    auto color_map  = options.color_map.color_map();
    auto color_norm = options.color_norm.get_diverging_norm();

    // Out base file name
    auto const base_fn
        = options.file_output.out_dir() + options.file_output.file_prefix()
        + "k_" + std::to_string( k ) + "_centroid_"
    ;

    // Write all centroid trees
    for( size_t ci = 0; ci < centroids.size(); ++ci ) {
        auto const& centroid = centroids[ci];

        // Init color vector with medium color of the diverging palette.
        auto colors = std::vector<utils::Color>( tree.edge_count(), color_map( color_norm, 0.0 ));

        // Overwrite color values for the columns that were used during kmeans.
        for( size_t j = 0; j < columns.size(); ++j ) {
            assert( -1.0 <= centroid[ j ] && centroid[ j ] <= 1.0 );
            colors[ columns[j] ] = color_map( color_norm, centroid[ j ]);
        }

        // Now, make a color vector and write to files.
        auto const cntr_fn = base_fn + std::to_string( ci );
        options.tree_output.write_tree_to_files(
            tree,
            colors,
            color_map,
            color_norm,
            cntr_fn
        );
    }
}

// =================================================================================================
//      Main Run Function
// =================================================================================================

void run_ikmeans( IkmeansOptions const& options )
{
    using namespace genesis;
    using namespace genesis::placement;
    using namespace genesis::tree;
    using namespace genesis::utils;

    // Print some user output.
    options.jplace_input.print();

    // Base check
    if( options.jplace_input.file_count() < 2 ) {
        throw std::runtime_error( "Cannot run k-means with fewer than 2 samples." );
    }

    // Read input data into imbalances matrix. Filter columns.
    auto profile = options.jplace_input.placement_profile( true );
    auto const columns = epca_filter_constant_columns( profile.edge_imbalances, 0.001 );

    // Move data into vectors, as this is what the kmeans needs.
    // It is a bit stupid, but otherweise the const column filtering would be weird...
    auto edge_imb_vec = std::vector<std::vector<double>>();
    edge_imb_vec.resize( profile.edge_imbalances.rows() );
    for( size_t i = 0; i < profile.edge_imbalances.rows(); ++i ) {
        edge_imb_vec[i].resize( profile.edge_imbalances.cols() );
        for( size_t j = 0; j < profile.edge_imbalances.cols(); ++j ) {
            edge_imb_vec[i][j] = profile.edge_imbalances(i, j);
        }
    }

    // Set up kmeans.
    auto ikmeans = EuclideanKmeans( profile.edge_imbalances.cols() );
    ikmeans.report_iteration = [&]( size_t iteration ){
        if( global_options.verbosity() >= 2 ) {
            std::cout << " - Iteration " << iteration << "\n";
        }
    };

    // Run kmeans for every specified k.
    auto const ks = get_k_values( options );
    for( auto const& k : ks ) {

        // Run it.
        std::cout << "Running Imbalance Kmeans with k=" << k << "\n";
        auto const iterations = ikmeans.run( edge_imb_vec, k );
        std::cout << "Finished after " << iterations << " iterations\n";

        // Write output.
        write_assignment_file( options, ikmeans.assignments(), k );
        write_cluster_trees( options, profile.tree, columns, ikmeans.centroids(), k );
    }
}
