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

#include "commands/analyze/pkmeans.hpp"

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

void setup_pkmeans( CLI::App& app )
{
    // Create the options and subcommand objects.
    auto opt = std::make_shared<PkmeansOptions>();
    auto sub = app.add_subcommand(
        "kmeans",
        "Run Phylogenetic k-means clustering on a set of samples."
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

    // Binning.
    auto bins_opt = sub->add_option(
        "--bins",
        opt->bins,
        "Bin the masses per-branch in order to save time and memory. "
        "Default is 0, that is, no binning. If set, we recommend to use 50 bins or more.",
        true
    );
    bins_opt->group( "Settings" );

    // Color.
    opt->color_map.add_color_list_opt_to_app( sub, "BuPuBk" );
    opt->color_norm.add_log_scaling_opt_to_app( sub );

    // Output files.
    opt->tree_output.add_tree_output_opts_to_app( sub );
    opt->file_output.add_output_dir_opt_to_app( sub );
    opt->file_output.add_file_prefix_opt_to_app( sub, "", "pkmeans" );

    // Set the run function as callback to be called when this subcommand is issued.
    // Hand over the options by copy, so that their shared ptr stays alive in the lambda.
    sub->set_callback( [ opt ]() {
        run_pkmeans( *opt );
    });
}

// =================================================================================================
//      Helper Functions
// =================================================================================================

std::vector<size_t> get_k_values( PkmeansOptions const& options )
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
     PkmeansOptions const& options,
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
    PkmeansOptions const& options,
    std::vector<genesis::tree::MassTree> const& centroids,
    size_t k
) {

    if( centroids.size() != k ) {
        throw std::runtime_error(
            "Internal Error: Differing number of centroids (" + std::to_string( centroids.size() ) +
            ") and  k (" + std::to_string( k ) + ")."
        );
    }

    // Get color map and norm.
    auto color_map  = options.color_map.color_map();
    auto color_norm = options.color_norm.get_sequential_norm();

    // Out base file name
    auto const base_fn
        = options.file_output.out_dir() + options.file_output.file_prefix()
        + "k_" + std::to_string( k ) + "_centroid_"
    ;

    // Write all centroid trees
    for( size_t ci = 0; ci < centroids.size(); ++ci ) {
        auto const& centroid = centroids[ci];

        // Prepare colors
        auto const masses = mass_tree_mass_per_edge( centroid );
        color_norm->autoscale_max( masses );

        // Now, make a color vector and write to files.
        auto const colors = color_map( *color_norm, masses );
        auto const cntr_fn = base_fn + std::to_string( ci );
        options.tree_output.write_tree_to_files(
            centroid,
            colors,
            color_map,
            *color_norm,
            cntr_fn
        );
    }
}

// =================================================================================================
//      Main Run Function
// =================================================================================================

void run_pkmeans( PkmeansOptions const& options )
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

    if( global_options.verbosity() >= 1 ) {
        std::cout << "Reading samples.\n";
    }

    // Prepare storage.
    auto const set_size = options.jplace_input.file_count();
    auto mass_trees = std::vector<MassTree>( set_size );
    size_t file_count = 0;

    // TODO branch length and compatibility checks!

    // Load files.
    #pragma omp parallel for schedule(dynamic)
    for( size_t fi = 0; fi < set_size; ++fi ) {

        // User output.
        if( global_options.verbosity() >= 2 ) {
            #pragma omp critical(GAPPA_NHD_PRINT_PROGRESS)
            {
                ++file_count;
                std::cout << "Reading file " << file_count << " of " << set_size;
                std::cout << ": " << options.jplace_input.file_path( fi ) << "\n";
            }
        }

        // Read in file.
        auto const sample = options.jplace_input.sample( fi );

        // Turn it into a mass tree.
        mass_trees[fi] = convert_sample_to_mass_tree( sample ).first;

        // Binning.
        if( options.bins > 0 ) {
            mass_tree_binify_masses( mass_trees[fi], options.bins );
        }
    }

    // Check for compatibility.
    if( ! mass_tree_all_identical_topology( mass_trees ) ) {
        throw std::runtime_error( "Sample reference trees do not have identical topology." );
    }

    // Make sure all have the same branch lengths.
    mass_trees_make_average_branch_lengths( mass_trees );

    // Set up kmeans.
    auto mkmeans = MassTreeKmeans();
    mkmeans.report_iteration = [&]( size_t iteration ){
        if( global_options.verbosity() >= 2 ) {
            std::cout << " - Iteration " << iteration << "\n";
        }
    };
    if( options.bins > 0 ) {
        mkmeans.accumulate_centroid_masses( options.bins );
    }

    // Run kmeans for every specified k.
    auto const ks = get_k_values( options );
    for( auto const& k : ks ) {

        // Run it.
        std::cout << "Running Phylogenetic Kmeans with k=" << k << "\n";
        auto const iterations = mkmeans.run( mass_trees, k );
        std::cout << "Finished after " << iterations << " iterations\n";

        // Write output.
        write_assignment_file( options, mkmeans.assignments(), k );
        write_cluster_trees( options, mkmeans.centroids(), k );
    }
}
