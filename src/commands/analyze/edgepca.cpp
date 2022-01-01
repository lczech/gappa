/*
    gappa - Genesis Applications for Phylogenetic Placement Analysis
    Copyright (C) 2017-2022 Lucas Czech

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

#include "commands/analyze/edgepca.hpp"

#include "options/global.hpp"
#include "tools/cli_setup.hpp"

#include "CLI/CLI.hpp"

#include "genesis/placement/function/epca.hpp"
#include "genesis/utils/io/output_stream.hpp"

#include <fstream>

// =================================================================================================
//      Setup
// =================================================================================================

void setup_edgepca( CLI::App& app )
{
    // Create the options and subcommand objects.
    auto opt = std::make_shared<EdgepcaOptions>();
    auto sub = app.add_subcommand(
        "edgepca",
        "Perform Edge PCA (Principal Component Analysis) for a set of samples."
    );

    // Add jplace input options.
    opt->jplace_input.add_jplace_input_opt_to_app( sub );

    // Kappa
    sub->add_option(
        "--kappa",
        opt->kappa,
        "Exponent for scaling between weighted and unweighted splitification.",
        true
    )->group( "Settings" );

    // Epsilon
    sub->add_option(
        "--epsilon",
        opt->epsilon,
        "Epsilon to use to determine if a split matrix’s column is constant for filtering. "
        "Set to a negative value to deavtivate constant columnn filtering.",
        true
    )->group( "Settings" );

    // Components`
    sub->add_option(
        "--components",
        opt->components,
        "Number of principal coordinates to calculate. Use 0 to calculate all possible coordinates.",
        true
    )->group( "Settings" );


    // TODO scaling/normalization


    // Other jplace settings
    opt->jplace_input.add_point_mass_opt_to_app( sub );
    opt->jplace_input.add_ignore_multiplicities_opt_to_app( sub );

    // Color.
    opt->color_map.add_color_list_opt_to_app( sub, "spectral" );
    opt->color_map.add_mask_opt_to_app( sub, "#dfdfdf" );

    // Output options for general files.
    // opt->file_output.setup();
    opt->file_output.add_default_output_opts_to_app( sub );

    // Tree output options.
    opt->tree_output.add_tree_output_opts_to_app( sub );

    // Set the run function as callback to be called when this subcommand is issued.
    // Hand over the options by copy, so that their shared ptr stays alive in the lambda.
    sub->callback( gappa_cli_callback(
        sub,
        {
            "Matsen2011-edgepca-and-squash-clustering"
        },
        [ opt ]() {
            run_edgepca( *opt );
        }
    ));
}

// =================================================================================================
//      Run
// =================================================================================================

void run_edgepca( EdgepcaOptions const& options )
{
    using namespace genesis;
    using namespace genesis::placement;
    using namespace genesis::utils;

    // Check if any of the general files we are going to produce already exists. If so, fail early.
    options.file_output.check_output_files_nonexistence({
        { "projection", "csv" }, { "transformation", "csv" }
    });

    // Same for tree files.
    std::vector<std::pair<std::string, std::string>> tree_infixes_and_extensions;
    for( auto const& e : options.tree_output.get_extensions() ) {
        tree_infixes_and_extensions.emplace_back( "tree_*", e );
    }
    options.file_output.check_output_files_nonexistence( tree_infixes_and_extensions );

    // User is warned when not using any tree outputs.
    options.tree_output.check_tree_formats();

    // Print some user output.
    options.jplace_input.print();

    // Base check
    if( options.jplace_input.file_count() < 2 ) {
        throw std::runtime_error( "Cannot run Edge PCA with fewer than 2 samples." );
    }

    // Read samples
    auto const sample_set = options.jplace_input.sample_set();
    assert( sample_set.size() >= 2 );

    // TODO check kappa and epsilon ranges!
    // TODO edge pca technically only needs the imbalance matrix. could refactor this to save mem!

    // Run, Forrest, run!
    LOG_MSG1 << "Running Edge PCA";
    auto const epca_data = epca( sample_set, options.kappa, options.epsilon, options.components );

    LOG_MSG1 << "Writing result files";

    // Write out projection
    auto proj_target = options.file_output.get_output_target( "projection", "csv" );
    auto& proj_os = proj_target->ostream();
    for( size_t r = 0; r < epca_data.projection.rows(); ++r ) {
        proj_os << options.jplace_input.base_file_name( r );
        for( size_t c = 0; c < epca_data.projection.cols(); ++c ) {
            proj_os << "," << epca_data.projection( r, c );
        }
        proj_os << "\n";
    }

    // Eigenvalues and Eigenvectors
    auto const trans_target = options.file_output.get_output_target( "transformation", "csv" );
    auto& trans_os = trans_target->ostream();
    for( size_t r = 0; r < epca_data.eigenvalues.size(); ++r ) {
        trans_os << epca_data.eigenvalues[r];
        for( size_t e = 0; e < epca_data.eigenvectors.rows(); ++e ) {
            trans_os << "," << epca_data.eigenvectors( r, e );
        }
        trans_os << "\n";
    }

    // Trees
    auto const& tree = sample_set.at(0).tree();
    for( size_t c = 0; c < epca_data.projection.cols(); ++c ) {
        LOG_BOLD;
        LOG_MSG1 << "Writing tree for component " << c;

        auto color_map = options.color_map.color_map();
        auto color_norm = options.color_norm.get_diverging_norm();

        color_norm.autoscale( epca_data.eigenvectors.col( c ));
        color_norm.make_centric();

        // Get the colors for the column we are interested in.
        auto const eigen_color_vector = color_map( color_norm, epca_data.eigenvectors.col( c ));

        // Init colors with the mask color, signifying that these edges do not have a value.
        std::vector<utils::Color> color_vector( tree.edge_count(), color_map.mask_color() );
        // std::vector<utils::Color> color_vector( tree.edge_count(), color_map( color_norm, 0.0 ));

        // For each edge that has an eigenvector, get its color and store it.
        // We need to do this because the filtering of const columns might have removed some.
        for( size_t i = 0; i < epca_data.edge_indices.size(); ++i ) {
            auto const edge_index = epca_data.edge_indices[i];
            color_vector[ edge_index ] = eigen_color_vector[i];
        }

        // Write tree
        auto const tree_infix = "tree_" + std::to_string( c );
        options.tree_output.write_tree_to_files(
            tree,
            color_vector,
            color_map,
            color_norm,
            options.file_output,
            tree_infix
        );
    }
}
