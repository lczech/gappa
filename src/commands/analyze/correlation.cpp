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

#include "commands/analyze/correlation.hpp"

#include "options/global.hpp"
#include "tools/cli_setup.hpp"

#include "CLI/CLI.hpp"

#include "genesis/placement/function/epca.hpp"
#include "genesis/placement/function/functions.hpp"
#include "genesis/placement/function/helper.hpp"
#include "genesis/placement/function/masses.hpp"
#include "genesis/placement/function/sample_set.hpp"
#include "genesis/utils/containers/dataframe.hpp"
#include "genesis/utils/containers/matrix.hpp"
#include "genesis/utils/core/algorithm.hpp"
#include "genesis/utils/core/fs.hpp"
#include "genesis/utils/math/correlation.hpp"
#include "genesis/utils/math/matrix.hpp"
#include "genesis/utils/math/statistics.hpp"
#include "genesis/utils/text/string.hpp"

#include <algorithm>
#include <cassert>
#include <limits>
#include <string>
#include <unordered_set>

#ifdef GENESIS_OPENMP
#   include <omp.h>
#endif

// =================================================================================================
//      Internal Helper Classes
// =================================================================================================

/**
 * @brief Helper struct that stores one of the variants of the correlation method and its properties.
 *
 * In the run function, we create a list of these, according to which options the user specified.
 * This list is then iterated to produce the resulting coloured trees for each variant.
 */
struct CorrelationVariant
{
    enum EdgeValues
    {
        kMasses,
        kImbalances
    };

    enum CorrelationMethod
    {
        kPearson,
        kSpearman,
        kKendall
    };

    CorrelationVariant( std::string const& n, EdgeValues m, CorrelationMethod d )
        : name(n)
        , edge_values(m)
        , correlation_value(d)
    {}

    std::string       name;
    EdgeValues        edge_values;
    CorrelationMethod correlation_value;
};

// =================================================================================================
//      Setup
// =================================================================================================

void setup_correlation( CLI::App& app )
{
    // Create the options and subcommand objects.
    auto options = std::make_shared<CorrelationOptions>();
    auto sub = app.add_subcommand(
        "correlation",
        "Calculate the Edge Correlation of samples and metadata features."
    );

    // Jplace input
    options->jplace_input.add_jplace_input_opt_to_app( sub );
    options->jplace_input.add_mass_norm_opt_to_app( sub, true );
    options->jplace_input.add_point_mass_opt_to_app( sub );
    options->jplace_input.add_ignore_multiplicities_opt_to_app( sub );

    // Metadata table input.
    options->metadata_input.add_table_input_opt_to_app( sub, true );
    options->metadata_input.add_separator_char_opt_to_app( sub );
    options->metadata_input.add_column_selection_opts_to_app( sub );

    // Edge value representation
    sub->add_option(
        "--edge-values",
        options->edge_values,
        "Values per edge used to calculate the correlation.",
        true
    )->group( "Settings" )
    ->transform(
        CLI::IsMember({ "both", "imbalances", "masses" }, CLI::ignore_case )
    );

    // Correlation method
    sub->add_option(
        "--method",
        options->method,
        "Method of correlation.",
        true
    )->group( "Settings" )
    ->transform(
        CLI::IsMember({ "all", "pearson", "spearman", "kendall" }, CLI::ignore_case )
    );

    // Color. We allow max, but not min, as this is always 0.
    options->color_map.add_color_list_opt_to_app( sub, "spectral" );
    options->color_map.add_mask_opt_to_app( sub, "#dfdfdf" );

    // Output files.
    // options->file_output.set_optionname( "tree" );
    options->file_output.add_default_output_opts_to_app( sub );
    // options->file_output.add_default_output_opts_to_app( sub, ".", "correlation_" );
    options->tree_output.add_tree_output_opts_to_app( sub );

    // Set the run function as callback to be called when this subcommand is issued.
    // Hand over the options by copy, so that their shared ptr stays alive in the lambda.
    sub->callback( gappa_cli_callback(
        sub,
        {
            "Czech2019-analyzing-and-visualizing-samples"
        },
        [ options ]() {
            run_correlation( *options );
        }
    ));
}

// =================================================================================================
//      Helper Functions
// =================================================================================================

/**
 * @brief Activate variants according to options being set.
 */
std::vector<CorrelationVariant> get_variants( CorrelationOptions const& options )
{
    std::vector<CorrelationVariant> variants;

    if(( options.edge_values == "both" ) || ( options.edge_values == "masses" )) {
        if(( options.method == "all" ) || ( options.method == "pearson" )) {
            variants.push_back({
                "masses_pearson", CorrelationVariant::kMasses, CorrelationVariant::kPearson
            });
        }
        if(( options.method == "all" ) || ( options.method == "spearman" )) {
            variants.push_back({
                "masses_spearman", CorrelationVariant::kMasses, CorrelationVariant::kSpearman
            });
        }
        if(( options.method == "all" ) || ( options.method == "kendall" )) {
            variants.push_back({
                "masses_kendall", CorrelationVariant::kMasses, CorrelationVariant::kKendall
            });
        }
    }
    if(( options.edge_values == "both" ) || ( options.edge_values == "imbalances" )) {
        if(( options.method == "all" ) || ( options.method == "pearson" )) {
            variants.push_back({
                "imbalances_pearson", CorrelationVariant::kImbalances, CorrelationVariant::kPearson
            });
        }
        if(( options.method == "all" ) || ( options.method == "spearman" )) {
            variants.push_back({
                "imbalances_spearman", CorrelationVariant::kImbalances, CorrelationVariant::kSpearman
            });
        }
        if(( options.method == "all" ) || ( options.method == "kendall" )) {
            variants.push_back({
                "imbalances_kendall", CorrelationVariant::kImbalances, CorrelationVariant::kKendall
            });
        }
    }

    return variants;
}

/**
 * @brief Get the metadata table sorted and checked against the input jplace files.
 */
genesis::utils::Dataframe get_metadata( CorrelationOptions const& options )
{
    // Get the metadata.
    // options.metadata_input.print();
    auto const df = options.metadata_input.read_double_dataframe();

    // Check if the sorting actually fits.
    if( ! options.metadata_input.check_row_names( df, options.jplace_input.base_file_names() )) {
        LOG_ERR << "Metadata row names: " << genesis::utils::join( df.row_names(), ", " );
        LOG_ERR << "Jplace file names: " << genesis::utils::join(
            options.jplace_input.base_file_names(), ", "
        );
        throw std::runtime_error(
            "The first column of the metadata file contains different row names "
            "than the input jplace file names. There needs to be exaclty one metadata line per "
            "input jplace file, using the file name (without the extension .jplace[.gz]) as identifier."
        );
    }

    // If everything fits, return it sorted in the order of the input jplace files.
    return options.metadata_input.sort_rows(
        df,
        options.jplace_input.base_file_names()
    );
}

/**
 * @brief Check whether the inpule files have unique names.
 */
void check_jplace_input( CorrelationOptions const& options )
{
    auto fns = options.jplace_input.base_file_names();
    std::sort( fns.begin(), fns.end() );

    if( std::adjacent_find( fns.begin(), fns.end() ) != fns.end() ) {
        throw std::runtime_error(
            "The file names of the input jplace files (without the extension .jplace[.gz]) are not "
            "unique and can thus not used as identifiers for metadata rows. "
            "Make sure that you use unique sample names."
        );
    }
}

// =================================================================================================
//      Make Color Tree
// =================================================================================================

void make_correlation_color_tree(
    CorrelationOptions const&  options,
    std::vector<double> const& values,
    genesis::tree::Tree const& tree,
    std::string const&         infix
) {
    using namespace genesis::utils;

    // Just in case...
    if( values.size() != tree.edge_count() ) {
        throw std::runtime_error( "Internal error: Trees and matrices do not fit to each other." );
    }

    // Get color norm and map.
    auto color_map = options.color_map.color_map();
    auto color_norm = ColorNormalizationDiverging( -1.0, 1.0 );

    // Now, make a color vector and write to files.
    auto const colors = color_map( color_norm, values );
    options.tree_output.write_tree_to_files(
        tree,
        colors,
        color_map,
        color_norm,
        options.file_output,
        infix
    );
}

// =================================================================================================
//      Run with Matrix
// =================================================================================================

/**
 * @brief Run with either the masses or the imbalances matrix.
 */
void run_with_matrix(
    CorrelationOptions const&                options,
    std::vector<CorrelationVariant> const&   variants,
    genesis::utils::Matrix<double> const&    edge_values,
    genesis::utils::Dataframe const&         df,
    CorrelationVariant::EdgeValues           edge_value_type,
    genesis::tree::Tree const&               tree
) {
    using namespace genesis;
    using namespace genesis::utils;

    if( edge_values.cols() != tree.edge_count() ) {
        throw std::runtime_error( "Internal Error: Edge values does not have corrent length." );
    }
    if( edge_values.rows() != df.rows() ) {
        throw std::runtime_error( "Internal Error: Jplace files and Dataframe have differing lengths." );
    }

    // Loop over all variants that have been set.
    for( auto const& variant : variants ) {

        // Only process the variants that have the current input metrix.
        // This is ugly, I know. But the distinction has to be made somewhere...
        if( variant.edge_values != edge_value_type ) {
            continue;
        }

        // Calculate correlation for each metadata field.
        for( auto const& meta_col : df ) {
            auto const& meta_dbl = meta_col.as<double>();

            // User output
            std::string corrname;
            switch( variant.correlation_value ) {
                case CorrelationVariant::kPearson: {
                    corrname = "Pearson";
                    break;
                }
                case CorrelationVariant::kSpearman: {
                    corrname = "Spearman";
                    break;
                }
                case CorrelationVariant::kKendall: {
                    corrname = "Kendall";
                    break;
                }
                default: {
                    throw std::runtime_error( "Internal Error: Invalid correlation variant." );
                }
            }
            LOG_MSG1 << "Writing " << corrname << " correlation with meta-data column "
                     << meta_col.name() << ".";

            // Prepare a vector for the correlation coefficients of all edges.
            auto corr_vec = std::vector<double>( tree.edge_count() );

            // Fill the vector by calculating correlation of each edge of the tree.
            #pragma omp parallel for
            for( size_t e = 0; e < tree.edge_count(); ++e ) {
                switch( variant.correlation_value ) {
                    case CorrelationVariant::kPearson: {
                        corr_vec[e] = pearson_correlation_coefficient(
                            meta_dbl.begin(), meta_dbl.end(),
                            edge_values.col( e ).begin(), edge_values.col( e ).end()
                        );
                        break;
                    }
                    case CorrelationVariant::kSpearman: {
                        corr_vec[e] = spearmans_rank_correlation_coefficient(
                            meta_dbl.begin(), meta_dbl.end(),
                            edge_values.col( e ).begin(), edge_values.col( e ).end()
                        );
                        break;
                    }
                    case CorrelationVariant::kKendall: {
                        corr_vec[e] = kendalls_tau_correlation_coefficient(
                            meta_dbl.begin(), meta_dbl.end(),
                            edge_values.col( e ).begin(), edge_values.col( e ).end()
                        );
                        break;
                    }
                    default: {
                        throw std::runtime_error( "Internal Error: Invalid correlation variant." );
                    }
                }
            }

            // Make a tree using the data vector and name of the variant and field.
            make_correlation_color_tree(
                options, corr_vec, tree,  meta_col.name() + "_" + variant.name
            );
        }
    }
}

// =================================================================================================
//      Run
// =================================================================================================

void run_correlation( CorrelationOptions const& options )
{
    using namespace genesis;
    using namespace genesis::placement;
    using namespace genesis::tree;
    using namespace genesis::utils;

    // -------------------------------------------------------------------------
    //     Checks and Preparation
    // -------------------------------------------------------------------------

    // User output for jplace input.
    options.jplace_input.print();

    // First check for unique jplace file names. If this fails, all the rest cannot work properly,
    // as we use the file names for identifying metadata rows.
    check_jplace_input( options );
    options.tree_output.check_tree_formats();

    // Read in metadata. We do this before reading in the samples, because this is faster,
    // and if it fails, the user does not have to wait that long only to then find it failing,
    // cf https://youtu.be/tcGQpjCztgA
    auto const df = get_metadata( options );

    // Get which variants of the method to run.
    auto const variants = get_variants( options );

    // Check for existing output files.
    std::vector<std::pair<std::string, std::string>> infixes_and_extensions;
    for( auto const& m : variants ) {
        for( auto const& f : df.col_names() ) {
            for( auto const& e : options.tree_output.get_extensions() ) {
                infixes_and_extensions.emplace_back( f + "_" + m.name, e );
            }
        }
    }
    options.file_output.check_output_files_nonexistence( infixes_and_extensions );

    // -------------------------------------------------------------------------
    //     Calculations and Output
    // -------------------------------------------------------------------------

    // Get the data. Read all samples and calcualte the matrices.
    auto const profile = options.jplace_input.placement_profile();

    LOG_MSG1 << "Calculating correlations and writing files.";

    // Calculate things as needed.
    if(( options.edge_values == "both" ) || ( options.edge_values == "masses" )) {
        LOG_BOLD;
        LOG_MSG1 << "Calculating corrlation with masses.";
        run_with_matrix(
            options, variants, profile.edge_masses, df, CorrelationVariant::kMasses, profile.tree
        );
    }
    if(( options.edge_values == "both" ) || ( options.edge_values == "imbalances" )) {
        LOG_BOLD;
        LOG_MSG1 << "Calculating corrlation with imbalances.";
        run_with_matrix(
            options, variants, profile.edge_imbalances, df, CorrelationVariant::kImbalances, profile.tree
        );
    }
}
