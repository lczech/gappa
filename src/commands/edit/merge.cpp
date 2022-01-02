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

#include "commands/edit/merge.hpp"

#include "options/global.hpp"
#include "tools/cli_setup.hpp"

#include "CLI/CLI.hpp"

#include "genesis/placement/formats/jplace_writer.hpp"
#include "genesis/placement/function/functions.hpp"
#include "genesis/tree/function/functions.hpp"
#include "genesis/utils/core/fs.hpp"
#include "genesis/utils/io/output_target.hpp"

#include <cassert>
#include <utility>

#ifdef GENESIS_OPENMP
#   include <omp.h>
#endif

// =================================================================================================
//      Setup
// =================================================================================================

void setup_merge( CLI::App& app )
{
    // Create the options and subcommand objects.
    auto options = std::make_shared<MergeOptions>();
    auto sub = app.add_subcommand(
        "merge",
        "Merge jplace files by combining their pqueries into one file."
    );

    // -----------------------------------------------------------
    //     Input options
    // -----------------------------------------------------------

    // Jplace input
    options->jplace_input.add_jplace_input_opt_to_app( sub );

    // // Reference tree. Not implemented as of now, as there does not seem much need for this.
    // options->reference_tree = sub->add_option(
    //     "--reference tree",
    //     options->reference_tree.value(),
    //     "If provided, use this reference tree instead of the ones given in the input jplace files. "
    //     "This allows to use a tree that is different from the input jplace file reference trees, "
    //     "as long as all bipartitions are compatible."
    // );
    // options->reference_tree.option()->check( CLI::ExistingFile );
    // options->reference_tree.option()->group( "Input" );

    // -----------------------------------------------------------
    //     Output options
    // -----------------------------------------------------------

    options->jplace_output.add_default_output_opts_to_app( sub );
    options->jplace_output.add_file_compress_opt_to_app( sub );

    // -----------------------------------------------------------
    //     Callback
    // -----------------------------------------------------------

    // Set the run function as callback to be called when this subcommand is issued.
    // Hand over the options by copy, so that their shared ptr stays alive in the lambda.
    sub->callback( gappa_cli_callback(
        sub,
        {},
        [ options ]() {
            run_merge( *options );
        }
    ));
}

// =================================================================================================
//      Run
// =================================================================================================

void run_merge( MergeOptions const& options )
{
    using namespace genesis;
    using namespace genesis::placement;
    using namespace genesis::utils;

    // Check if any of the files we are going to produce already exists. If so, fail early.
    options.jplace_output.check_output_files_nonexistence( "merge", "jplace" );

    // Print some user output.
    options.jplace_input.print();

    // User output.
    LOG_MSG1 << "Reading " << options.jplace_input.file_count() << " sample"
             << ( options.jplace_input.file_count() > 1 ? "s" : "" ) << ".";

    // Get all queries of all samples. Requires that all have the same ref tree.
    auto sample = options.jplace_input.merged_samples();

    // Write the new sample to a file.
    JplaceWriter().write(
        sample,
        options.jplace_output.get_output_target( "merge", "jplace" )
    );
}
