/*
    gappa - Genesis Applications for Phylogenetic Placement Analysis
    Copyright (C) 2017-2024 Lucas Czech

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
    Lucas Czech <lczech@carnegiescience.edu>
    Department of Plant Biology, Carnegie Institution For Science
    260 Panama Street, Stanford, CA 94305, USA
*/

#include "commands/analyze/placement_factorization.hpp"

#include "options/global.hpp"
#include "tools/cli_setup.hpp"

#include "CLI/CLI.hpp"

#include "genesis/placement/function/functions.hpp"
#include "genesis/placement/function/operators.hpp"
#include "genesis/tree/mass_tree/functions.hpp"
#include "genesis/tree/mass_tree/phylo_factor_colors.hpp"
#include "genesis/tree/mass_tree/phylo_factor.hpp"
#include "genesis/utils/containers/matrix/writer.hpp"
#include "genesis/utils/core/std.hpp"
#include "genesis/utils/io/output_stream.hpp"
#include "genesis/utils/io/output_target.hpp"
#include "genesis/utils/math/regression/dataframe.hpp"
#include "genesis/utils/math/regression/glm.hpp"
#include "genesis/utils/color/color.hpp"
#include "genesis/utils/color/color.hpp"
#include "genesis/utils/color/list_sequential.hpp"

#include <cmath>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <unordered_set>
#include <vector>

// =================================================================================================
//      Setup
// =================================================================================================

void setup_placement_factorization( CLI::App& app )
{
    // Create the options and subcommand objects.
    auto opt = std::make_shared<PlacementFactorizationOptions>();
    auto sub = app.add_subcommand(
        "placement-factorization",
        "Perform Placement-Factorization on a set of samples."
    );

    // -----------------------------------------------------------
    //     Input options
    // -----------------------------------------------------------

    // Jplace input
    opt->jplace_input.add_jplace_input_opt_to_app( sub );

    // Metadata table input.
    opt->metadata_input.add_table_input_opt_to_app( sub, true );
    opt->metadata_input.add_separator_char_opt_to_app( sub );
    opt->metadata_input.add_column_selection_opts_to_app( sub );

    // Jplace Settings
    opt->jplace_input.add_point_mass_opt_to_app( sub );
    opt->jplace_input.add_ignore_multiplicities_opt_to_app( sub );

    // -----------------------------------------------------------
    //     Balance Settings
    // -----------------------------------------------------------

    opt->factors = sub->add_option(
        "--factors",
        opt->factors.value(),
        "Number of phylogenetic factors to compute.",
        true
    )->group( "Settings" );

    // Taxon weights
    opt->taxon_weight_tendency = sub->add_option(
        "--taxon-weight-tendency",
        opt->taxon_weight_tendency.value(),
        "Tendency term to use for calculating taxon weights.",
        true
    )->group( "Settings" )
    ->transform(
        CLI::IsMember({ "geometric-mean", "arithmetic-mean", "median", "none" }, CLI::ignore_case )
    );
    opt->taxon_weight_norm = sub->add_option(
        "--taxon-weight-norm",
        opt->taxon_weight_norm.value(),
        "Norm term to use for calculating taxon weights.",
        true
    )->group( "Settings" )
    ->transform(
        CLI::IsMember({ "manhattan", "euclidean", "maximum", "aitchison", "none" }, CLI::ignore_case )
    );

    // Pseudo counts
    opt->pseudo_count_summand_all = sub->add_option(
        "--pseudo-count-summand-all",
        opt->pseudo_count_summand_all.value(),
        "Constant that is added to all taxon masses to avoid zero counts.",
        true
    )->group( "Settings" );
    opt->pseudo_count_summand_zeros = sub->add_option(
        "--pseudo-count-summand-zeros",
        opt->pseudo_count_summand_zeros.value(),
        "Constant that is added to taxon masses that are zero, to avoid zero counts.",
        true
    )->group( "Settings" );

    // -----------------------------------------------------------
    //     Output options
    // -----------------------------------------------------------

    // Color.
    // opt->color_map.add_color_list_opt_to_app( sub, "viridis" );
    // opt->color_norm.add_log_scaling_opt_to_app( sub );

    opt->file_output.add_default_output_opts_to_app( sub );
    opt->tree_output.add_tree_output_opts_to_app( sub );

    // sub->add_option(
    //     "--factor-tree-file",
    //     opt->factor_tree_file.value(),
    //     "If a path is provided, an svg file with a tree colored by the factors is written.",
    //     true
    // )->group( "Output" );

    // -----------------------------------------------------------
    //     Run Function
    // -----------------------------------------------------------

    // Set the run function as callback to be called when this subcommand is issued.
    // Hand over the options by copy, so that their shared ptr stays alive in the lambda.
    sub->callback( gappa_cli_callback(
        sub,
        {
            "Czech2019-analyzing-and-visualizing-samples",
            "Washburne2017-phylofactorization"
        },
        [ opt ]() {
            run_placement_factorization( *opt );
        }
    ));
}

// =================================================================================================
//      Input Reading
// =================================================================================================

struct MetaMatrix
{
    genesis::utils::Matrix<double> matrix;
    std::vector<std::string> column_names;
    std::vector<std::string> row_names;
};

MetaMatrix read_meta_data( PlacementFactorizationOptions const& options )
{
    MetaMatrix result;

    // Get the metadata.
    // options.metadata_input.print();
    auto df = options.metadata_input.read_string_dataframe();

    // Check if the sorting actually fits.
    if( ! options.metadata_input.check_row_names( df, options.jplace_input.base_file_names() )) {
        throw std::runtime_error(
            "The first column of the metadata table file contains different row names "
            "than the input jplace file names. There needs to be exaclty one metadata line per "
            "input jplace file, using the file name (without the extension .jplace) as identifier."
        );
    }

    // Convert as needed for phylo factorization.
    std::string report;
    auto meta = glm_prepare_dataframe( df, report );
    result.column_names = meta.col_names();
    LOG_MSG1 << report;

    // TODO use glm_convert_dataframe instead?!

    // Copy the meta data in the correct sample order.
    auto const jplace_count = options.jplace_input.file_count();
    result.matrix = genesis::utils::Matrix<double>( jplace_count, meta.cols() );
    result.row_names.resize( jplace_count );
    assert( meta.rows() == jplace_count );
    for( size_t i = 0; i < jplace_count; ++i ) {
        for( size_t c = 0; c < meta.cols(); ++c ) {
            result.matrix( i, c ) = meta[ c ].as<double>()[ options.jplace_input.base_file_name(i) ];
        }
        result.row_names[i] = options.jplace_input.base_file_name(i);
    }

    return result;
}

genesis::tree::BalanceSettings get_balance_settings( PlacementFactorizationOptions const& options )
{
    using namespace genesis::tree;
    BalanceSettings result;

    // Tendency
    if( options.taxon_weight_tendency.value() == "geometric-mean" ) {
        result.tendency = BalanceSettings::WeightTendency::kGeometricMean;
    } else if( options.taxon_weight_tendency.value() == "arithmetic-mean" ) {
        result.tendency = BalanceSettings::WeightTendency::kArithmeticMean;
    } else if( options.taxon_weight_tendency.value() == "median" ) {
        result.tendency = BalanceSettings::WeightTendency::kMedian;
    } else if( options.taxon_weight_tendency.value() == "none" ) {
        result.tendency = BalanceSettings::WeightTendency::kNone;
    } else {
        throw CLI::ValidationError(
            "--taxon-weight-tendency (" + options.taxon_weight_tendency.value() + ")",
            "Invalid option selected for taxon weight tendency term."
        );
    }

    // Norm
    if( options.taxon_weight_norm.value() == "manhattan" ) {
        result.norm = BalanceSettings::WeightNorm::kManhattan;
    } else if( options.taxon_weight_norm.value() == "euclidean" ) {
        result.norm = BalanceSettings::WeightNorm::kEuclidean;
    } else if( options.taxon_weight_norm.value() == "maximum" ) {
        result.norm = BalanceSettings::WeightNorm::kMaximum;
    } else if( options.taxon_weight_norm.value() == "aitchison" ) {
        result.norm = BalanceSettings::WeightNorm::kAitchison;
    } else if( options.taxon_weight_norm.value() == "none" ) {
        result.norm = BalanceSettings::WeightNorm::kNone;
    } else {
        throw CLI::ValidationError(
            "--taxon-weight-norm (" + options.taxon_weight_norm.value() + ")",
            "Invalid option selected for taxon weight norm term."
        );
    }

    // Pseudo Counts
    result.pseudo_count_summand_all   = options.pseudo_count_summand_all.value();
    result.pseudo_count_summand_zeros = options.pseudo_count_summand_zeros.value();

    return result;
}

genesis::tree::BalanceData read_balance_data( PlacementFactorizationOptions const& options )
{
    // Read in the trees and immediately convert them to mass trees to save storage.
    // Make sure that they are not normalized, by providing false here.
    auto const mass_trees = options.jplace_input.mass_tree_set( false );

    // Use the trees for getting balance data.
    auto const settings = get_balance_settings( options );
    return mass_balance_data( mass_trees, settings );
}

struct GlmCoefficients
{
    using Coefficients = std::vector<double>;
    std::vector<Coefficients> edge_coefficients;
};

std::vector<GlmCoefficients> prepare_glm_coefficients(
    PlacementFactorizationOptions const& options,
    genesis::tree::BalanceData const& balances
) {
    auto result = std::vector<GlmCoefficients>( options.factors.value() );
    for( auto& factor : result ) {
        factor.edge_coefficients.resize( balances.tree.edge_count() );
    }
    return result;
}

// =================================================================================================
//      Output Writing
// =================================================================================================

void write_factor_tree(
    PlacementFactorizationOptions const& options,
    std::vector<genesis::tree::PhyloFactor> const& factors,
    genesis::tree::Tree const& tree
) {
    using namespace genesis::tree;

    // if( options.factor_tree_file.value().empty() ) {
    //     return;
    // }

    // TODO make these colors settings!

    // Prepare color settings.
    // Dummy fill so that we have enough colors for all factors.
    PhyloFactorCladeColors clade_cols;
    for( size_t i = clade_cols.clade_colors.size(); i < factors.size(); ++i ) {
        clade_cols.clade_colors.push_back( clade_cols.clade_colors[ i % clade_cols.clade_colors.size() ] );
    }

    // Make a tree with the edges of that factor.
    auto all_edge_cols = phylo_factor_clade_colors( tree, factors, 0, clade_cols );

    options.tree_output.write_tree_to_files(
        tree, all_edge_cols, options.file_output, "factors_tree"
    );
    // write_color_tree_to_svg_file(
    //     tree, options.tree_output.svg_tree_output_opt().layout_parameters(),
    //     all_edge_cols, options.file_output.out_dir() + "factors.tree"
    // );
}

void write_factor_edges(
    PlacementFactorizationOptions const& options,
    std::vector<genesis::tree::PhyloFactor> const& factors,
    genesis::tree::Tree const& tree
) {
    for( size_t i = 0; i < factors.size(); ++i ) {
        // Make a tree with the edges of that factor.
        auto edge_cols = phylo_factor_single_factor_colors( tree, factors, i );

        options.tree_output.write_tree_to_files(
            tree, edge_cols, options.file_output, "factor_edges_" + std::to_string( i+1 )
        );
    }
}

void write_factor_objective_values(
    PlacementFactorizationOptions const& options,
    std::vector<genesis::tree::PhyloFactor> const& factors,
    genesis::tree::Tree const& tree
) {
    using namespace genesis::utils;

    for( size_t i = 0; i < factors.size(); ++i ) {
        auto const& factor = factors[i];

        // write objective value trees
        auto cm = ColorMap( color_list_viridis() );
        cm.mask_color( Color( 0.8, 0.8, 0.8 ));

        auto const cn = ColorNormalizationLinear( factor.all_objective_values );
        auto const edge_cols = cm( cn, factor.all_objective_values );

        options.tree_output.write_tree_to_files(
            tree, edge_cols, cm, cn,
            options.file_output, "objective_values_" + std::to_string( i+1 )
        );
    }
}

void write_factor_taxa(
    PlacementFactorizationOptions const& options,
    std::vector<genesis::tree::PhyloFactor> const& factors,
    genesis::tree::Tree const& tree
) {
    using namespace genesis::tree;
    using namespace genesis::utils;

    // Open the file
    auto factor_taxa_of = options.file_output.get_output_target( "factor_taxa", "csv" );

    // Write a line to the file. Factor index, then taxon name, then indicator of which side
    auto write_taxa_list = [&](
        size_t factor, std::unordered_set<size_t> indices, std::string const& side
    ) {
        std::unordered_set<std::string> edge_names;
        for( auto const ei : indices ) {
            auto const& ed = tree.edge_at( ei ).secondary_link().node().data<CommonNodeData>();
            if( ! ed.name.empty() ) {
                edge_names.insert( ed.name );
            }
        }
        for( auto const& en : edge_names ) {
            (*factor_taxa_of) << factor << "\t" << en << "\t" << side << "\n";
        }
    };

    // Write the table, for each factor, and for each side of it the
    (*factor_taxa_of) << "Factor\tTaxon\tRootSide\n";
    for( size_t i = 0; i < factors.size(); ++i ) {
        write_taxa_list( i+1, factors[i].edge_indices_primary, "1" );
        write_taxa_list( i+1, factors[i].edge_indices_secondary, "0" );
    }
}

void write_balances_table(
    PlacementFactorizationOptions const& options,
    std::vector<genesis::tree::PhyloFactor> const& factors
) {
    using namespace genesis::utils;
    if( factors.size() == 0 ) {
        return;
    }

    // Prepare result matrix.
    auto balances = Matrix<double>( factors[0].balances.size(), factors.size() );
    auto col_names = std::vector<std::string>( factors.size() );

    // Fill matrix.
    for( size_t i = 0; i < factors.size(); ++i ) {
        assert( factors[i].balances.size() == balances.rows() );
        balances.col(i) = factors[i].balances;
        col_names[ i ] = "Factor_" + std::to_string( i + 1 );
    }

    // Write balances of the factors.
    auto target = options.file_output.get_output_target( "factor_balances", "csv" );
    MatrixWriter<double>().write(
        balances, target, options.jplace_input.base_file_names(), col_names, "Sample"
    );
}

void write_glm_coefficients(
    PlacementFactorizationOptions const& options,
    std::vector<genesis::tree::PhyloFactor> const& factors,
    MetaMatrix const& meta,
    std::vector<GlmCoefficients> const& glm_coeffs
) {
    auto target = options.file_output.get_output_target( "factor_glm_coefficients", "csv" );

    // Write the header. One column per meta
    (*target) << "Factor\tIntercept";
    for( auto const& col_name : meta.column_names ) {
        (*target) << "\t" << col_name;
    }
    (*target) << "\n";

    // Write the coefficients of the winning edge.
    assert( glm_coeffs.size() == factors.size() );
    for( size_t i = 0; i < factors.size(); ++i ) {
        auto const winning_edge_idx = factors[i].edge_index;
        assert( glm_coeffs[i].edge_coefficients.size() == factors[i].all_objective_values.size() );
        auto const& edge_vals = glm_coeffs[i].edge_coefficients[winning_edge_idx];
        (*target) << (i+1);
        for( auto v : edge_vals ) {
            (*target) << "\t" << v;
        }
        (*target) << "\n";
    }
}

// =================================================================================================
//      Run
// =================================================================================================

void run_placement_factorization( PlacementFactorizationOptions const& options )
{
    using namespace genesis;
    using namespace genesis::tree;
    using namespace genesis::utils;

    // -------------------------------------------------------------------------
    //     Preparations
    // -------------------------------------------------------------------------

    // Check if any of the files we are going to produce already exists. If so, fail early.
    std::vector<std::pair<std::string, std::string>> files_to_check;
    for( auto const& e : options.tree_output.get_extensions() ) {
        files_to_check.push_back({ "factors_tree", e });
        for( size_t i = 0; i < options.factors.value(); ++i ) {
            files_to_check.push_back({ "factor_edges_" + std::to_string( i+1 ), e });
            files_to_check.push_back({ "objective_values_" + std::to_string( i+1 ), e });
        }
    }
    files_to_check.push_back({ "factor_taxa", "csv" });
    files_to_check.push_back({ "factor_balances", "csv" });
    files_to_check.push_back({ "factor_glm_coefficients", "csv" });
    options.file_output.check_output_files_nonexistence( files_to_check );

    // Print some user output.
    options.jplace_input.print();

    // User is warned when not using any tree outputs.
    options.tree_output.check_tree_formats();

    // -------------------------------------------------------------------------
    //     Read Data
    // -------------------------------------------------------------------------

    auto const meta = read_meta_data( options );
    auto const balances = read_balance_data( options );

    // -------------------------------------------------------------------------
    //     Calculations and Output
    // -------------------------------------------------------------------------

    // We capture the GLM coefficients of all factors and edges. The outer vector has elements
    // per factor (iteration), and the inner per edge index.
    auto glm_coeffs = prepare_glm_coefficients( options, balances );

    // Ruuuuuun!
    auto const factors = phylogenetic_factorization(
        balances,
        [&]( size_t iteration, size_t edge_index, std::vector<double> const& balances ){
            auto const fit = glm_fit( meta.matrix, balances, glm_family_gaussian() );

            // Store the coefficients computed from the fitting
            assert( iteration < glm_coeffs.size() );
            assert( edge_index < glm_coeffs[iteration].size() );
            auto& coeff = glm_coeffs[iteration].edge_coefficients[edge_index];
            coeff = glm_coefficients( meta.matrix, balances, fit );

            // If something did not work in the GLM, we return a nan,
            // so that this edge is not considered downstream.
            if( !fit.converged || !std::isfinite(fit.null_deviance) || !std::isfinite(fit.deviance) ) {
                return std::numeric_limits<double>::quiet_NaN();
            }

            return fit.null_deviance - fit.deviance;
        },
        options.factors.value(),
        []( size_t iteration, size_t max_iterations ){
            LOG_MSG1 << "Iteration " << iteration << " of " << max_iterations;
        }
    );

    write_factor_tree( options, factors, balances.tree );
    write_factor_edges( options, factors, balances.tree );
    write_factor_objective_values( options, factors, balances.tree );
    write_factor_taxa( options, factors, balances.tree );
    write_balances_table( options, factors );
    write_glm_coefficients( options, factors, meta, glm_coeffs );
}
