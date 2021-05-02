/*
    gappa - Genesis Applications for Phylogenetic Placement Analysis
    Copyright (C) 2017-2021 Lucas Czech

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

#include "commands/simulate/random_tree.hpp"

#include "options/global.hpp"
#include "tools/cli_setup.hpp"
#include "tools/misc.hpp"

#include "CLI/CLI.hpp"

#include "genesis/tree/common_tree/newick_writer.hpp"
#include "genesis/tree/common_tree/tree.hpp"
#include "genesis/tree/function/manipulation.hpp"
#include "genesis/tree/tree.hpp"
#include "genesis/utils/io/output_target.hpp"

#include <cassert>
#include <cctype>
#include <string>
#include <stdexcept>
#include <vector>

// =================================================================================================
//      Setup
// =================================================================================================

void setup_random_tree( CLI::App& app )
{
    // Create the options and subcommand objects.
    auto opt = std::make_shared<RandomTreeOptions>();
    auto sub = app.add_subcommand(
        "random-tree",
        "Create a random tree with a given numer of leaf nodes."
    );

    // -----------------------------------------------------------
    //     Input Data
    // -----------------------------------------------------------

    // Leaf count
    auto leaf_count_opt = sub->add_option(
        "--leaf-count",
        opt->num_leaves,
        "Number of leaf nodes (taxa) to create."
    );
    leaf_count_opt->group( "Input" );
    leaf_count_opt->required();

    // -----------------------------------------------------------
    //     Output Options
    // -----------------------------------------------------------

    opt->file_output.add_default_output_opts_to_app( sub );
    opt->file_output.add_file_compress_opt_to_app( sub );

    // -----------------------------------------------------------
    //     Callback
    // -----------------------------------------------------------

    // Set the run function as callback to be called when this subcommand is issued.
    // Hand over the options by copy, so that their shared ptr stays alive in the lambda.
    sub->callback( gappa_cli_callback(
        sub,
        {},
        [ opt ]() {
            run_random_tree( *opt );
        }
    ));
}

// =================================================================================================
//      Run
// =================================================================================================

void run_random_tree( RandomTreeOptions const& options )
{
    using namespace ::genesis;
    using namespace ::genesis::tree;

    // Init randomness.
    std::srand( std::time( nullptr ));

    // Check that at least one of the options is set.
    if( options.num_leaves < 3 ) {
        throw CLI::ValidationError(
            "--leaf-count",
            "Leaf count has to be at least 3."
        );
    }

    // Check if the output file name already exists. If so, fail early.
    options.file_output.check_output_files_nonexistence( "random-tree", "newick" );

    // Make a minimal tree with common data types.
    auto tree = minimal_tree();
    if( ! validate_topology( tree )) {
        throw std::runtime_error( "Internal error: Invalid tree created." );
    }

    // The minimal tree already has 2 leaves. Create the remaining ones.
    for( size_t i = 2; i < options.num_leaves; ++i ) {
        // Pick a random edge to attach the leaf to.
        auto& rand_edge = tree.edge_at( std::rand() % tree.edge_count() );
        add_new_leaf_node( tree, rand_edge );
    }
    if( ! validate_topology( tree )) {
        throw std::runtime_error( "Internal error: Invalid tree created." );
    }

    // Give random names to the leaf nodes.
    size_t leaf_node_cnt = 0;
    for( auto& node : tree.nodes() ) {
        if( is_leaf( node )) {
            assert( leaf_node_cnt < options.num_leaves );
            node.data<CommonNodeData>().name = random_indexed_name( leaf_node_cnt, options.num_leaves );
            ++leaf_node_cnt;
        }
    }

    // Give random edge lengths in [ 0.0, 1.0 ] to all edges.
    for( auto& edge : tree.edges() ) {
        auto const r = static_cast<double>( std::rand() ) / static_cast<double>( RAND_MAX );
        edge.data<CommonEdgeData>().branch_length = r;
    }

    // Reroot on random inner node.
    size_t rand_node_index = 0;
    while( is_leaf( tree.node_at( rand_node_index ))) {
        rand_node_index = std::rand() % tree.node_count();
    }
    change_rooting( tree, tree.node_at( rand_node_index ));

    // Create a newick tree from it.
    auto nw = CommonTreeNewickWriter();
    nw.write( tree, options.file_output.get_output_target( "random-tree", "newick" ));
}
