/*
    gappa - Genesis Applications for Phylogenetic Placement Analysis
    Copyright (C) 2017-2019 Lucas Czech and HITS gGmbH

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

#include "commands/analyze/edgepca.hpp"

#include "options/global.hpp"

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
        "Perform Edge PCA for a set of samples."
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
    opt->color_map.add_mask_color_opt_to_app( sub, "#dfdfdf" );

    // Output options
    opt->file_output.add_output_dir_opt_to_app( sub );
    opt->file_output.add_file_prefix_opt_to_app( sub, "", "edgepca_" );
    opt->tree_output.add_tree_output_opts_to_app( sub );

    // Set the run function as callback to be called when this subcommand is issued.
    // Hand over the options by copy, so that their shared ptr stays alive in the lambda.
    sub->callback( [ opt ]() {
        run_edgepca( *opt );
    });
}

// =================================================================================================
//      Run
// =================================================================================================

void run_edgepca( EdgepcaOptions const& options )
{
    using namespace genesis;
    using namespace genesis::placement;
    using namespace genesis::utils;

    // Check if any of the files we are going to produce already exists. If so, fail early.
    std::vector<std::string> files_to_check;
    files_to_check.push_back( options.file_output.file_prefix() + "projection.csv" );
    files_to_check.push_back( options.file_output.file_prefix() + "transformation.csv" );
    for( auto const& e : options.tree_output.get_extensions() ) {
        files_to_check.push_back(
            options.file_output.file_prefix() + "tree_[0-9]*\\." + e
        );
    }
    options.file_output.check_nonexistent_output_files( files_to_check );

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
    auto const epca_data = epca( sample_set, options.kappa, options.epsilon, options.components );

    // Write out projection
    auto const proj_fn = options.file_output.out_dir() + options.file_output.file_prefix() + "projection.csv";
    std::ofstream proj_os;
    genesis::utils::file_output_stream( proj_fn, proj_os );
    for( size_t r = 0; r < epca_data.projection.rows(); ++r ) {
        proj_os << options.jplace_input.base_file_name( r );
        for( size_t c = 0; c < epca_data.projection.cols(); ++c ) {
            proj_os << "," << epca_data.projection( r, c );
        }
        proj_os << "\n";
    }
    proj_os.close();

    // Eigenvalues and Eigenvectors
    auto const trans_fn = options.file_output.out_dir() + options.file_output.file_prefix() + "transformation.csv";
    std::ofstream trans_os;
    genesis::utils::file_output_stream( trans_fn, trans_os );
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
        auto const tree_fn = options.file_output.out_dir() + options.file_output.file_prefix() + "tree_" + std::to_string( c );
        options.tree_output.write_tree_to_files(
            tree,
            color_vector,
            color_map,
            color_norm,
            tree_fn
        );
    }
}
