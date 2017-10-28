/*
    gappa - Genesis Applications for Phylogenetic Placement Analysis
    Copyright (C) 2017 Lucas Czech

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

#include "commands/tog.hpp"

#include "tools/input.hpp"

#include "CLI/CLI.hpp"

#include "genesis/placement/formats/jplace_reader.hpp"
#include "genesis/placement/function/tree.hpp"
#include "genesis/tree/default/newick_writer.hpp"
#include "genesis/utils/core/fs.hpp"

// =================================================================================================
//      Setup
// =================================================================================================

void setup_tog( CLI::App& app )
{
    // Create the options and subcommand objects.
    auto opt = std::make_shared<TogOptions>();
    auto sub = app.add_subcommand(
        "tog",
        "makes a tree with each of the reads represented as a pendant edge.",
        true
    );

    // Common options.
    sub->add_option(
        "placefiles", opt->jplace_paths,
        "List of jplace files to process."
    )->required()->check(CLI::ExistingFile);
    sub->add_option(
        "--out-dir", opt->out_dir,
        "Specify the directory to write files to.", true
    )->check(CLI::ExistingDirectory);

    // Specific options.
    sub->add_option(
        "--name-prefix", opt->name_prefix,
        "Specify a prefix to be added to all new leaf nodes.",
        true
    );
    sub->add_option(
        "--fully-resolve", opt->fully_resolve,
        "Control in which way multiple placements at one edge are turned into new edges.",
        true
    );

    // TODO add check whether out files do not exist
    // TODO add verbosity levels

    // Set the run function as callback to be called when this subcommand is issued.
    // Hand over the options by copy, so that their shared ptr stays alive in the lambda.
    sub->set_callback( [opt]() {
        run_tog( *opt );
    });
}

// =================================================================================================
//      Run
// =================================================================================================

void run_tog( TogOptions const& options )
{
    using namespace genesis;
    using namespace genesis::placement;

    auto const jplace_files = get_jplace_files( options.jplace_paths );
    auto const file_names   = get_file_names( jplace_files );

    auto reader = JplaceReader();
    // TODO dont report errors in jplace. offer subcommand for that

    // TODO add support for external trees, e.g., bootstrap trees.
    // for this, make sure that the attribute tree is used so that all values can be captured.
    // that probably means there has to be an option to specify which values go where
    // (edges or nodes). or, if only newick is used, not, because we do not reroot,
    // so the assignment does not change.

    // TODO mention in wiki that multiple files are possible and how they are named.

    for( size_t i = 0; i < jplace_files.size(); ++i ) {
        // Read the sample and make the tree.
        auto const sample = reader.from_file( jplace_files[i] );
        auto const tog    = labelled_tree( sample, options.fully_resolve, options.name_prefix );

        // Write output to file.
        auto const out_file = utils::dir_normalize_path( options.out_dir ) + file_names[i];
        tree::DefaultTreeNewickWriter().to_file( tog, out_file );
    }
}
