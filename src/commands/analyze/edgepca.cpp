/*
    gappa - Genesis Applications for Phylogenetic Placement Analysis
    Copyright (C) 2017-2023 Lucas Czech

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
#include "tools/misc.hpp"

#include "CLI/CLI.hpp"

#include "genesis/placement/function/epca.hpp"
#include "genesis/tree/formats/newick/simple_reader.hpp"
#include "genesis/tree/formats/newick/simple_tree.hpp"
#include "genesis/tree/formats/newick/simple_writer.hpp"
#include "genesis/tree/function/functions.hpp"
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

    // -------------------------------------------------------------
    //     Checks and preparation
    // -------------------------------------------------------------

    // Check if any of the general files we are going to produce already exists. If so, fail early.
    options.file_output.check_output_files_nonexistence({
        { "projection", "csv" }, { "transformation", "csv" },
        { "eigenvalues", "csv" }, { "eigenvectors", "csv" },
        { "edge_indices", "newick" }, { "eigenvector_", "newick" }
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

    // -------------------------------------------------------------
    //     Processing
    // -------------------------------------------------------------

    // Read samples
    auto const sample_set = options.jplace_input.sample_set();
    if(  sample_set.size() < 2 ) {
        throw std::runtime_error("Need at least two input jplace files to compute EdgePCA");
    }

    // TODO check kappa and epsilon ranges!
    // TODO edge pca technically only needs the imbalance matrix. could refactor this to save mem!

    // Run, Forrest, run!
    LOG_MSG1 << "Running Edge PCA";
    auto const epca_data = epca( sample_set, options.kappa, options.epsilon, options.components );

    // Some checks
    internal_check(
        epca_data.eigenvalues.size()  == options.components,
        "Edge PCA data invalid. epca_data.eigenvalues.size() != options.components"
    );
    internal_check(
        epca_data.eigenvectors.rows() == epca_data.edge_indices.size(),
        "Edge PCA data invalid. epca_data.eigenvectors.rows() != epca_data.edge_indices.size()"
    );
    internal_check(
        epca_data.eigenvectors.cols() == options.components,
        "Edge PCA data invalid. epca_data.eigenvectors.cols() != options.components"
    );
    internal_check(
        epca_data.projection.rows()   == sample_set.size(),
        "Edge PCA data invalid. epca_data.projection.rows() != sample_set.size()"
    );
    internal_check(
        epca_data.projection.cols()   == options.components,
        "Edge PCA data invalid. epca_data.projection.cols() != options.components"
    );

    // -------------------------------------------------------------
    //     Output and File Writing
    // -------------------------------------------------------------

    // Some helpful user output.
    auto const& tree = sample_set.at(0).tree();
    LOG_BOLD;
    LOG_MSG1 << "Tree contains a total of " << tree.edge_count() << " edges, thereof "
             << genesis::tree::inner_edge_count( tree ) << " inner edges (not leading to a leaf). "
             << "Out of these, " << epca_data.edge_indices.size() << " have been used for computing "
             << "the Edge PCA; the remaining ones were filtered out, as they only contained "
             << "constant edge imbalance values, which are not useful for running a PCA. "
             << "The `eigenvectors.newick` tree file contains node labels at the outer nodes of "
             << "those edges that show the edge indices, corresponding to the first column of "
             << "the `eigenvectors.csv` table.";
    LOG_BOLD;

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

    // Eigenvalues and Eigenvectors, transformation as in guppy
    auto const trans_target = options.file_output.get_output_target( "transformation", "csv" );
    auto& trans_os = trans_target->ostream();
    for( size_t r = 0; r < epca_data.eigenvalues.size(); ++r ) {
        trans_os << epca_data.eigenvalues[r];
        for( size_t e = 0; e < epca_data.eigenvectors.rows(); ++e ) {
            trans_os << "," << epca_data.eigenvectors.at( e, r );
        }
        trans_os << "\n";
    }

    // Also write out eigenvalues as individual file, for user convenience
    auto const eigenvalues_target = options.file_output.get_output_target( "eigenvalues", "csv" );
    auto& eigenvalues_os = eigenvalues_target->ostream();
    for( size_t r = 0; r < epca_data.eigenvalues.size(); ++r ) {
        eigenvalues_os << epca_data.eigenvalues[r] << "\n";
    }

    // Same for eigenvectors, including their indices
    auto const eigenvectors_target = options.file_output.get_output_target( "eigenvectors", "csv" );
    auto& eigenvectors_os = eigenvectors_target->ostream();
    eigenvectors_os << "edge_index";
    for( size_t c = 0; c < epca_data.eigenvectors.cols(); ++c ) {
        eigenvectors_os << ",component_" << c;
    }
    eigenvectors_os << "\n";
    for( size_t r = 0; r < epca_data.eigenvectors.rows(); ++r ) {
        eigenvectors_os << epca_data.edge_indices[r];
        for( size_t c = 0; c < epca_data.eigenvectors.cols(); ++c ) {
            eigenvectors_os << "," << epca_data.eigenvectors.at( r, c );
        }
        eigenvectors_os << "\n";
    }

    // Also, write a newick tree with the inner edge indices
    auto edge_index_tree = sample_set[0].tree();
    for( size_t i = 0; i < tree.edge_count(); ++i ) {
        using genesis::tree::CommonNodeData;
        if( genesis::tree::is_leaf(tree.edge_at(i)) ) {
            continue;
        }
        internal_check( tree.edge_at(i).index() == i, "wrong tree edge indices" );
        auto& name = edge_index_tree.edge_at( i ).secondary_node().data<CommonNodeData>().name;
        name = std::to_string( tree.edge_at(i).index() );
    }
    auto nw = genesis::tree::CommonTreeNewickWriter();
    nw.write( edge_index_tree, options.file_output.get_output_target( "edge_indices", "newick" ));

    // Trees
    for( size_t c = 0; c < epca_data.projection.cols(); ++c ) {
        // LOG_BOLD;
        LOG_MSG1 << "Writing tree for component " << c;

        // Prepare a list of all eigenvector componentes, for the whole tree, using 0 when that
        // edge has not been used in the PCA (filtered out, or leaf edge).
        auto eigenvector_comps = std::vector<double>( tree.edge_count(), 0.0);
        for( size_t r = 0; r < epca_data.edge_indices.size(); ++r ) {
            eigenvector_comps.at( epca_data.edge_indices[r] ) = epca_data.eigenvectors.at( r, c );
        }

        // Write a tree with those values annotated in NHX-style at the edges.
        auto nw = genesis::tree::CommonTreeNewickWriter();
        nw.edge_to_element_plugins.push_back(
            [&]( genesis::tree::TreeEdge const& edge, genesis::tree::NewickBrokerElement& element ){
                element.comments.push_back(
                    "&&NHX:eigen=" + std::to_string( eigenvector_comps[ edge.index() ])
                );
            }
        );
        nw.write( tree, options.file_output.get_output_target(
            "eigenvector_" + std::to_string( c ), "newick"
        ));

        // Prepare the color trees
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
