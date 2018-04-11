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

#include "commands/analyze/dispersion.hpp"

#include "options/global.hpp"

#include "CLI/CLI.hpp"

#include "genesis/placement/function/epca.hpp"
#include "genesis/placement/function/functions.hpp"
#include "genesis/placement/function/helper.hpp"
#include "genesis/placement/function/sample_set.hpp"
#include "genesis/utils/containers/matrix.hpp"
#include "genesis/utils/math/matrix.hpp"

#ifdef GENESIS_OPENMP
#   include <omp.h>
#endif

// =================================================================================================
//      Setup
// =================================================================================================

void setup_dispersion( CLI::App& app )
{
    // Create the options and subcommand objects.
    auto options = std::make_shared<DispersionOptions>();
    auto sub = app.add_subcommand(
        "dispersion",
        "Calcualte the Edge Dispersion between samples."
    );

    // Add common options.
    options->jplace_input.add_jplace_input_opt_to_app( sub );
    options->jplace_input.add_point_mass_opt_to_app( sub );

    // Color. We allow max, but not min, as this is always 0.
    options->color_map.add_color_list_opt_to_app( sub, "viridis" );
    options->color_map.add_mask_color_opt_to_app( sub );

    // Output files.
    options->tree_output.add_tree_output_opts_to_app( sub );
    options->file_output.add_output_dir_opt_to_app( sub );
    options->file_output.add_file_prefix_opt_to_app( sub, "tree", "tree" );

    // Edge value representation
    sub->add_set_ignore_case(
        "--edge-values",
        options->edge_values,
        { "masses", "imbalances", "both" },
        "Values per edge used to calculate the dispersion.",
        true
    );

    // Dispersion method
    sub->add_set_ignore_case(
        "--method",
        options->method,
        { "var", "var-log", "cv", "vmr", "vmr-log", "all" },
        "Method of dispersion. Variance (var), variance log-scaled (var-log), "
        "coefficient of variation (cv, standard deviation divided by mean), "
        "variance to mean ratio (vmr, Index of Dispersion), "
        "variance to mean ratio log-scaled (vmr-log) or all of them.",
        true
    );

    sub->add_flag(
        "--normalize",
        options->normalize,
        "If set, and if multiple input samples are provided, their masses are normalized first, "
        "so that each sample contributes a total mass of 1 to the result."
    );

    // Set the run function as callback to be called when this subcommand is issued.
    // Hand over the options by copy, so that their shared ptr stays alive in the lambda.
    sub->set_callback( [ options ]() {
        run_dispersion( *options );
    });
}

// =================================================================================================
//      Make Color Tree
// =================================================================================================

void make_color_tree(
    DispersionOptions const&   options,
    std::vector<double> const& values,
    bool                       log_scaling,
    genesis::tree::Tree const& tree,
    std::string const&         prefix
) {
    using namespace genesis::utils;

    // Get color norm and map.
    auto color_map = options.color_map.color_map();
    std::unique_ptr<ColorNormalizationLinear> color_norm;
    if( log_scaling ) {
        color_norm = make_unique<ColorNormalizationLogarithmic>();
        color_map.clip_under( true );
    } else {
        color_norm = make_unique<ColorNormalizationLinear>();
    }

    // Scale correctly. This checks for invalid values as well.
    color_norm->autoscale_max( values );

    // Some combinations do not work. Skip them.
    if( log_scaling && color_norm->max_value() < 1.0 ) {
        std::cout << "Skipping dispersion" << prefix << ", because this combination does not work.\n";
        return;
    }

    // Now, make a color vector and write to files.
    auto const colors = color_map( *color_norm, values );
    options.tree_output.write_tree_to_files(
        tree,
        colors,
        color_map,
        *color_norm,
        options.file_output.out_dir() + options.file_output.file_prefix() + "dispersion" + prefix
    );
}

// =================================================================================================
//      Run with Matrix
// =================================================================================================

/**
 * @brief Run with either the masses or the imbalances matrix.
 */
void run_with_matrix(
    DispersionOptions const&              options,
    genesis::utils::Matrix<double> const& values,
    genesis::tree::Tree const&            tree,
    std::string const&                    prefix
) {
    if( values.cols() != tree.edge_count() ) {
        throw std::runtime_error( "Internal Error: Edge values does not have corrent length." );
    }

    // Calculate things. We cheat and calculate everyting, because this is not that much
    // data and effort. Then, only the intended files are written.
    auto const mean_stddev = matrix_col_mean_stddev( values );
    auto var_vec = std::vector<double>( mean_stddev.size(), 0.0 );
    auto cv_vec  = std::vector<double>( mean_stddev.size(), 0.0 );
    auto vmr_vec = std::vector<double>( mean_stddev.size(), 0.0 );
    for( size_t i = 0; i < mean_stddev.size(); ++i ) {
        var_vec[ i ] = mean_stddev[ i ].stddev * mean_stddev[ i ].stddev;
        cv_vec[ i ]  = mean_stddev[ i ].stddev / mean_stddev[ i ].mean;
        vmr_vec[ i ] = mean_stddev[ i ].stddev * mean_stddev[ i ].stddev / mean_stddev[ i ].mean;
    }

    if(( options.method == "all" ) || ( options.method == "var" )) {
        make_color_tree( options, var_vec, false, tree, prefix + "_var" );
    }
    if(( options.method == "all" ) || ( options.method == "var-log" )) {
        make_color_tree( options, var_vec, true, tree, prefix + "_var-log" );
    }
    if(( options.method == "all" ) || ( options.method == "cv" )) {
        make_color_tree( options, cv_vec, false, tree, prefix + "_cv" );
    }
    if(( options.method == "all" ) || ( options.method == "vmr" )) {
        make_color_tree( options, vmr_vec, false, tree, prefix + "_vmr" );
    }
    if(( options.method == "all" ) || ( options.method == "vmr-log" )) {
        make_color_tree( options, vmr_vec, true, tree, prefix + "_vmr-log" );
    }
}

// =================================================================================================
//      Run
// =================================================================================================

void run_dispersion( DispersionOptions const& options )
{
    using namespace genesis;
    using namespace genesis::placement;
    using namespace genesis::tree;
    using namespace genesis::utils;

    // TODO progress and file info and file existence cheecks

    // Get samples. This is memory-expensive, but for now, that's okay.
    // Can optimize later, and process one file at a time instead.
    auto const sample_set = options.jplace_input.sample_set();
    auto const tree       = average_branch_length_tree( sample_set );

    // Calculate things as needed.
    if(( options.edge_values == "both" ) || ( options.edge_values == "imbalances" )) {
        auto const edge_masses = placement_weight_per_edge( sample_set );
        run_with_matrix( options, edge_masses, tree, "_masses" );
    }
    if(( options.edge_values == "both" ) || ( options.edge_values == "masses" )) {
        auto const edge_imbals = epca_imbalance_matrix( sample_set, true );
        run_with_matrix( options, edge_imbals, tree, "_imbalances" );
    }
}
