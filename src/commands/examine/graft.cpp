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

#include "commands/examine/graft.hpp"

#include "options/global.hpp"
#include "tools/cli_setup.hpp"

#include "CLI/CLI.hpp"

#include "genesis/placement/formats/jplace_reader.hpp"
#include "genesis/placement/function/tree.hpp"
#include "genesis/tree/common_tree/newick_writer.hpp"
#include "genesis/utils/core/fs.hpp"
#include "genesis/utils/io/output_target.hpp"

#ifdef GENESIS_OPENMP
#   include <omp.h>
#endif

// =================================================================================================
//      Setup
// =================================================================================================

void setup_graft( CLI::App& app )
{
    // Create the options and subcommand objects.
    auto opt = std::make_shared<GraftOptions>();
    auto sub = app.add_subcommand(
        "graft",
        "Make a tree with each of the query sequences represented as a pendant edge."
    );

    // Add input options.
    opt->jplace_input.add_jplace_input_opt_to_app( sub );

    // Fill in custom options.
    sub->add_flag(
        "--fully-resolve", opt->fully_resolve,
        "If set, branches that contain multiple pqueries are resolved by creating a new branch "
        "for each of the pqueries individually, placed according to their distal/proximal lengths. "
        "If not set (default), all pqueries at one branch are collected in a subtree "
        "that branches off from the branch."
    )->group( "Settings" );
    sub->add_option(
        "--name-prefix", opt->name_prefix,
        "Specify a prefix to be added to all new leaf nodes, i.e., to the query sequence names.",
        true
    )->group( "Settings" );

    // Add output options.
    opt->file_output.add_default_output_opts_to_app( sub );
    opt->newick_tree_output.add_newick_tree_quote_invalid_chars_opt_to_app( sub, nullptr );

    // Set the run function as callback to be called when this subcommand is issued.
    // Hand over the options by copy, so that their shared ptr stays alive in the lambda.
    sub->callback( gappa_cli_callback(
        sub,
        {},
        [ opt ]() {
            run_graft( *opt );
        }
    ));
}

// =================================================================================================
//      Run
// =================================================================================================

void run_graft( GraftOptions const& options )
{
    using namespace genesis;
    using namespace genesis::placement;

    // Prepare output file names and check if any of them already exists. If so, fail early.
    std::vector<std::pair<std::string, std::string>> out_tree_files;
    for( auto const& bfn : options.jplace_input.base_file_names() ) {
        out_tree_files.push_back({ bfn, "newick" });
    }
    options.file_output.check_output_files_nonexistence( out_tree_files );

    // Print some user output.
    options.jplace_input.print();

    // TODO add support for external trees, e.g., bootstrap trees.
    // for this, make sure that the attribute tree is used so that all values can be captured.
    // that probably means there has to be an option to specify which values go where
    // (edges or nodes). or, if only newick is used, not, because we do not reroot,
    // so the assignment does not change.

    size_t file_counter = 0;
    #pragma omp parallel for schedule(dynamic)
    for( size_t i = 0; i < options.jplace_input.file_count(); ++i ) {

        // User output.
        LOG_MSG2 << "Reading file " << ( ++file_counter ) << " of " << options.jplace_input.file_count()
                 << ": " << options.jplace_input.file_path( i );

        // Read the sample and make the tree.
        auto const sample = options.jplace_input.sample( i );
        auto const tog    = labelled_tree( sample, options.fully_resolve, options.name_prefix );

        // Write output to file.
        options.newick_tree_output.write_tree(
            tog, options.file_output.get_output_target(
                out_tree_files[i].first, out_tree_files[i].second
            )
        );
        // tree::CommonTreeNewickWriter().write(
        //     tog, options.file_output.get_output_target(
        //         out_tree_files[i].first, out_tree_files[i].second
        //     )
        // );
    }
}
