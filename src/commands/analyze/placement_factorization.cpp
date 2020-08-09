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
#include "genesis/utils/tools/color.hpp"
#include "genesis/utils/tools/color.hpp"
#include "genesis/utils/tools/color/list_sequential.hpp"
#include "genesis/utils/tools/color/list_sequential.hpp"

#include <fstream>
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

    sub->add_option(
        "--factors",
        opt->factors.value,
        "Number of phylogenetic factors to compute.",
        true
    )->group( "Settings" );

    // Taxon weights
    sub->add_option(
        "--taxon-weight-tendency",
        opt->taxon_weight_tendency.value,
        "Tendency term to use for calculating taxon weights.",
        true
    )->group( "Settings" )
    ->transform(
        CLI::IsMember({ "geometric-mean", "arithmetic-mean", "median", "none" }, CLI::ignore_case )
    );
    sub->add_option(
        "--taxon-weight-norm",
        opt->taxon_weight_norm.value,
        "Norm term to use for calculating taxon weights.",
        true
    )->group( "Settings" )
    ->transform(
        CLI::IsMember({ "manhattan", "euclidean", "maximum", "aitchison", "none" }, CLI::ignore_case )
    );

    // Pseudo counts
    sub->add_option(
        "--pseudo-count-summand-all",
        opt->pseudo_count_summand_all.value,
        "Constant that is added to all taxon masses to avoid zero counts.",
        true
    )->group( "Settings" );
    sub->add_option(
        "--pseudo-count-summand-zeros",
        opt->pseudo_count_summand_zeros.value,
        "Constant that is added to taxon masses that are zero, to avoid zero counts.",
        true
    )->group( "Settings" );

    // -----------------------------------------------------------
    //     Output options
    // -----------------------------------------------------------

    // Color.
    // opt->color_map.add_color_list_opt_to_app( sub, "viridis" );
    // opt->color_norm.add_log_scaling_opt_to_app( sub );

    opt->file_output.add_output_dir_opt_to_app( sub );
    // opt->file_output.add_file_prefix_opt_to_app( sub, "tree", "tree_" );
    opt->tree_output.add_tree_output_opts_to_app( sub );

    // sub->add_option(
    //     "--factor-tree-file",
    //     opt->factor_tree_file.value,
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

genesis::utils::Matrix<double> read_meta_data( PlacementFactorizationOptions const& options )
{
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
    LOG_MSG1 << report;

    // TODO use glm_convert_dataframe instead?!

    // Copy the meta data in the correct sample order.
    auto const jplace_count = options.jplace_input.file_count();
    auto result = genesis::utils::Matrix<double>( jplace_count, meta.cols() );
    assert( meta.rows() == jplace_count );
    for( size_t i = 0; i < jplace_count; ++i ) {
        for( size_t c = 0; c < meta.cols(); ++c ) {
            result( i, c ) = meta[ c ].as<double>()[ options.jplace_input.base_file_name(i) ];
        }
    }

    return result;
}

genesis::tree::BalanceSettings get_balance_settings( PlacementFactorizationOptions const& options )
{
    using namespace genesis::tree;
    BalanceSettings result;

    // Tendency
    if( options.taxon_weight_tendency.value == "geometric-mean" ) {
        result.tendency = BalanceSettings::WeightTendency::kGeometricMean;
    } else if( options.taxon_weight_tendency.value == "arithmetic-mean" ) {
        result.tendency = BalanceSettings::WeightTendency::kArithmeticMean;
    } else if( options.taxon_weight_tendency.value == "median" ) {
        result.tendency = BalanceSettings::WeightTendency::kMedian;
    } else if( options.taxon_weight_tendency.value == "none" ) {
        result.tendency = BalanceSettings::WeightTendency::kNone;
    } else {
        throw CLI::ValidationError(
            "--taxon-weight-tendency (" + options.taxon_weight_tendency.value + ")",
            "Invalid option selected for taxon weight tendency term."
        );
    }

    // Norm
    if( options.taxon_weight_norm.value == "manhattan" ) {
        result.norm = BalanceSettings::WeightNorm::kManhattan;
    } else if( options.taxon_weight_norm.value == "euclidean" ) {
        result.norm = BalanceSettings::WeightNorm::kEuclidean;
    } else if( options.taxon_weight_norm.value == "maximum" ) {
        result.norm = BalanceSettings::WeightNorm::kMaximum;
    } else if( options.taxon_weight_norm.value == "aitchison" ) {
        result.norm = BalanceSettings::WeightNorm::kAitchison;
    } else if( options.taxon_weight_norm.value == "none" ) {
        result.norm = BalanceSettings::WeightNorm::kNone;
    } else {
        throw CLI::ValidationError(
            "--taxon-weight-norm (" + options.taxon_weight_norm.value + ")",
            "Invalid option selected for taxon weight norm term."
        );
    }

    // Pseudo Counts
    result.pseudo_count_summand_all   = options.pseudo_count_summand_all.value;
    result.pseudo_count_summand_zeros = options.pseudo_count_summand_zeros.value;

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

// =================================================================================================
//      Output Writing
// =================================================================================================

void write_factor_tree(
    PlacementFactorizationOptions const& options,
    std::vector<genesis::tree::PhyloFactor> const& factors,
    genesis::tree::Tree const& tree
) {
    using namespace genesis::tree;

    // if( options.factor_tree_file.value.empty() ) {
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
        tree, all_edge_cols, options.file_output.out_dir() + "factors_tree"
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
            tree, edge_cols, options.file_output.out_dir() + "factor_edges_" + std::to_string( i+1 )
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
            options.file_output.out_dir() + "objective_values_" + std::to_string( i+1 )
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

    std::ofstream factor_taxa_of;
    file_output_stream( options.file_output.out_dir() + "factor_taxa.txt", factor_taxa_of );

    auto write_taxa_list = [&]( std::unordered_set<size_t> indices ) {
        std::unordered_set<std::string> edge_names;
        for( auto const ei : indices ) {
            auto const& ed = tree.edge_at( ei ).secondary_link().node().data<CommonNodeData>();
            if( ! ed.name.empty() ) {
                edge_names.insert( ed.name );
            }
        }
        for( auto const& en : edge_names ) {
            factor_taxa_of << en << "\n";
        }
        factor_taxa_of << "\n";
    };

    for( size_t i = 0; i < factors.size(); ++i ) {
        auto const& factor = factors[i];

        factor_taxa_of << "Factor " << (i+1) << ", root side:\n";
        write_taxa_list( factor.edge_indices_primary );

        factor_taxa_of << "Factor " << (i+1) << ", non-root side:\n";
        write_taxa_list( factor.edge_indices_secondary );
    }
    factor_taxa_of.close();
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
    MatrixWriter<double>().write(
        balances, genesis::utils::to_file( options.file_output.out_dir() + "factor_balances.csv" ),
        options.jplace_input.base_file_names(), col_names, "Sample"
    );
}

// =================================================================================================
//      Run
// =================================================================================================

void run_placement_factorization( PlacementFactorizationOptions const& options )
{
    using namespace genesis;

    // -------------------------------------------------------------------------
    //     Preparations
    // -------------------------------------------------------------------------

    // Check if any of the files we are going to produce already exists. If so, fail early.
    // std::vector<std::string> files_to_check;
    // files_to_check.push_back( "cluster\\.newick" );
    // for( auto const& e : options.tree_output.get_extensions() ) {
    //     files_to_check.push_back(
    //         options.file_output.file_prefix() + "[0-9]*\\." + e
    //     );
    // }
    // options.file_output.check_nonexistent_output_files( files_to_check );

    // Print some user output.
    options.jplace_input.print();

    // -------------------------------------------------------------------------
    //     Read Data
    // -------------------------------------------------------------------------

    auto const meta = read_meta_data( options );
    auto const balances = read_balance_data( options );

    // -------------------------------------------------------------------------
    //     Calculations and Output
    // -------------------------------------------------------------------------

    auto const factors = tree::phylogenetic_factorization(
        balances,
        [&]( std::vector<double> const& balances ){
            auto const fit = glm_fit( meta, balances, utils::glm_family_gaussian() );

            // TODO return nan in the follwing cases. but make sure that this works with
            // the phylofactor implementation first!

            // if( !fit.converged ) {
            //     LOG_DBG << "did not converge";
            // }
            // if( ! std::isfinite(fit.null_deviance) ) {
            //     LOG_DBG << "fit.null_deviance " << fit.null_deviance;
            // }
            // if( ! std::isfinite(fit.deviance) ) {
            //     LOG_DBG << "fit.deviance " << fit.deviance;
            // }
            // if( ! std::isfinite(fit.null_deviance) || ! std::isfinite(fit.deviance) ) {
            //     LOG_DBG << "data.meta_vals_mat " << join( data.meta_vals_mat );
            //     LOG_DBG << "balances " << join( balances );
            // }

            return fit.null_deviance - fit.deviance;
        },
        options.factors.value,
        []( size_t iteration, size_t max_iterations ){
            LOG_MSG1 << "Iteration " << iteration << " of " << max_iterations;
        }
    );

    write_factor_tree( options, factors, balances.tree );
    write_factor_edges( options, factors, balances.tree );
    write_factor_objective_values( options, factors, balances.tree );
    write_factor_taxa( options, factors, balances.tree );
    write_balances_table( options, factors );
}
