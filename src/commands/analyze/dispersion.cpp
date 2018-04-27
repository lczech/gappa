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
//      Internal Helper Classes
// =================================================================================================

/**
 * @brief Helper struct that stores one of the variants of the dispersion methods and its properties.
 *
 * In the run function, we create a list of these, according to which options the user specified.
 * This list is then iteratored to produces the resulting coloured trees for each variant.
 */
struct DispersionMethod
{
    enum InputMatrix
    {
        kMasses,
        kImbalances
    };

    enum DispersionValue
    {
        kVar,
        kCv,
        kVmr
    };

    DispersionMethod( std::string const& n, InputMatrix m, DispersionValue d, bool l )
        : name(n)
        , inp_mat(m)
        , disp_val(d)
        , log_scaling(l)
    {}

    std::string      name;
    InputMatrix      inp_mat;
    DispersionValue  disp_val;
    bool             log_scaling = false;
};

// =================================================================================================
//      Setup
// =================================================================================================

void setup_dispersion( CLI::App& app )
{
    // Create the options and subcommand objects.
    auto options = std::make_shared<DispersionOptions>();
    auto sub = app.add_subcommand(
        "dispersion",
        "Calculate the Edge Dispersion between samples."
    );

    // Input.
    options->jplace_input.add_jplace_input_opt_to_app( sub );
    options->jplace_input.add_point_mass_opt_to_app( sub );

    // Edge value representation
    sub->add_set_ignore_case(
        "--edge-values",
        options->edge_values,
        { "masses", "imbalances", "both" },
        "Values per edge used to calculate the dispersion.",
        true
    )->group( "Settings" );

    // Dispersion method
    sub->add_set_ignore_case(
        "--method",
        options->method,
        { "var", "var-log", "cv", "cv-log" "vmr", "vmr-log", "all" },
        "Method of dispersion. Variance (var), variance log-scaled (var-log), "
        "coefficient of variation (cv, standard deviation divided by mean), "
        "coefficient of variation log-scaled (cv-log), "
        "variance to mean ratio (vmr, Index of Dispersion), "
        "variance to mean ratio log-scaled (vmr-log) "
        "or all of them (as far as they are applicable).",
        true
    )->group( "Settings" );

    // Extra settings.
    sub->add_flag(
        "--normalize",
        options->normalize,
        "If set, the masses of the input files are normalized first, "
        "so that each sample contributes a total mass of 1 to the result."
    )->group( "Settings" );

    // Color. We allow max, but not min, as this is always 0.
    options->color_map.add_color_list_opt_to_app( sub, "viridis" );
    options->color_map.add_mask_color_opt_to_app( sub );

    // Output files.
    options->tree_output.add_tree_output_opts_to_app( sub );
    options->file_output.add_output_dir_opt_to_app( sub );
    options->file_output.add_file_prefix_opt_to_app( sub, "tree", "dispersion_" );

    // Set the run function as callback to be called when this subcommand is issued.
    // Hand over the options by copy, so that their shared ptr stays alive in the lambda.
    sub->set_callback( [ options ]() {
        run_dispersion( *options );
    });
}

// =================================================================================================
//      Output File Name
// =================================================================================================

std::string output_file_name(
    DispersionOptions const&   options,
    std::string const&         prefix
) {
    return options.file_output.file_prefix() + prefix;
}

// =================================================================================================
//      Make Color Tree
// =================================================================================================

void make_color_tree(
    DispersionOptions const&   options,
    std::vector<double> const& values,
    bool                       log_scaling,
    genesis::tree::Tree const& tree,
    std::string const&         full_prefix
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
        std::cout << "Skipping " << full_prefix << ", ";
        std::cout << "because this combination does not work with values < 1.0\n";
        return;
    }

    // Just in case...
    if( values.size() != tree.edge_count() ) {
        throw std::runtime_error( "Internal error: Trees and matrices do not fit to each other." );
    }

    // Now, make a color vector and write to files.
    auto const colors = color_map( *color_norm, values );
    options.tree_output.write_tree_to_files(
        tree,
        colors,
        color_map,
        *color_norm,
        options.file_output.out_dir() + output_file_name( options, full_prefix )
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
    std::vector<DispersionMethod> const&  methods,
    genesis::utils::Matrix<double> const& values,
    DispersionMethod::InputMatrix         inp_mat,
    genesis::tree::Tree const&            tree
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

    // Loop over all methods that have been set.
    for( auto const& method : methods ) {

        // Only process the methods that have the current input metrix.
        // This is ugly, I know. But the distinction has to be made somewhere...
        if( method.inp_mat != inp_mat ) {
            continue;
        }

        // Get the data vector that we want to use for this method.
        std::vector<double> const* vec;
        switch( method.disp_val ) {
            case DispersionMethod::kVar:
                vec = &var_vec;
                break;
            case DispersionMethod::kCv:
                vec = &cv_vec;
                break;
            case DispersionMethod::kVmr:
                vec = &vmr_vec;
                break;
            default:
                throw std::runtime_error( "Internal Error: Invalid dispersion method." );
        }

        // Make a tree using the data vector and name of the method.
        make_color_tree( options, *vec, method.log_scaling, tree, method.name );
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

    // User output.
    options.jplace_input.print_files();

    // Activate methods according to options being set.
    // For imbalances, only variance makes sense.
    std::vector<DispersionMethod> methods;
    if(( options.edge_values == "both" ) || ( options.edge_values == "masses" )) {
        if(( options.method == "all" ) || ( options.method == "var" )) {
            methods.push_back({ "masses_var", DispersionMethod::kMasses, DispersionMethod::kVar, false });
        }
        if(( options.method == "all" ) || ( options.method == "var-log" )) {
            methods.push_back({ "masses_var_log", DispersionMethod::kMasses, DispersionMethod::kVar, true });
        }
        if(( options.method == "all" ) || ( options.method == "cv" )) {
            methods.push_back({ "masses_cv", DispersionMethod::kMasses, DispersionMethod::kCv, false });
        }
        if(( options.method == "all" ) || ( options.method == "cv-log" )) {
            methods.push_back({ "masses_cv_log", DispersionMethod::kMasses, DispersionMethod::kCv, true });
        }
        if(( options.method == "all" ) || ( options.method == "vmr" )) {
            methods.push_back({ "masses_vmr", DispersionMethod::kMasses, DispersionMethod::kVmr, false });
        }
        if(( options.method == "all" ) || ( options.method == "vmr-log" )) {
            methods.push_back({ "masses_vmr_log", DispersionMethod::kMasses, DispersionMethod::kVmr, true });
        }
    }
    if(( options.edge_values == "both" ) || ( options.edge_values == "imbalances" )) {
        if(( options.method == "all" ) || ( options.method == "var" )) {
            methods.push_back({ "imbalances_var", DispersionMethod::kImbalances, DispersionMethod::kVar, false });
        }
        // if(( options.method == "all" ) || ( options.method == "var-log" )) {
        //     methods.push_back({ "imbalances_var_log", DispersionMethod::kImbalances, DispersionMethod::kVar, true });
        // }
    }

    // Check for existing output files.
    // We currently check for the file names, but not the correct extensions.
    // This would require to check here already which tree types will be written. Not worth the effort for now.
    std::vector<std::string> files_to_check;
    for( auto const& m : methods ) {
        files_to_check.push_back( output_file_name( options, m.name ) + "\\.*" );
    }
    options.file_output.check_nonexistent_output_files( files_to_check );

    // Read all samples. This is memory-expensive, but for now, that's okay.
    // Can optimize later, and process one file at a time instead.
    auto const sample_set = options.jplace_input.sample_set();
    Tree tree;
    try{
        tree = average_branch_length_tree( sample_set );
    } catch( ... ) {
        throw std::runtime_error( "Input jplace files have differing reference trees." );
    }

    // Calculate things as needed.
    if(( options.edge_values == "both" ) || ( options.edge_values == "masses" )) {
        auto edge_masses = placement_weight_per_edge( sample_set );

        // Normalize per row if needed.
        if( options.normalize ) {
            auto const rsums = matrix_row_sums( edge_masses );

            #pragma omp parallel for
            for( size_t r = 0; r < edge_masses.rows(); ++r ) {
                for( size_t c = 0; c < edge_masses.cols(); ++c ) {
                    edge_masses( r, c ) /= rsums[ r ];
                }
            }
        }

        run_with_matrix( options, methods, edge_masses, DispersionMethod::kMasses, tree );
    }
    if(( options.edge_values == "both" ) || ( options.edge_values == "imbalances" )) {

        // Imbalances are already normalized.
        auto const edge_imbals = epca_imbalance_matrix( sample_set, true );
        run_with_matrix( options, methods, edge_imbals, DispersionMethod::kImbalances, tree );
    }
}
