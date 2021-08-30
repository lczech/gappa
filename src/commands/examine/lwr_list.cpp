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
    Lucas Czech <lczech@carnegiescience.edu>
    Department of Plant Biology, Carnegie Institution For Science
    260 Panama Street, Stanford, CA 94305, USA
*/

#include "commands/examine/lwr_list.hpp"

#include "options/global.hpp"
#include "tools/cli_setup.hpp"

#include "CLI/CLI.hpp"

#include "genesis/placement/function/functions.hpp"
#include "genesis/placement/function/masses.hpp"
#include "genesis/placement/function/operators.hpp"

#include "genesis/utils/io/output_stream.hpp"
#include "genesis/utils/math/histogram.hpp"
#include "genesis/utils/math/histogram/stats.hpp"

#include <cassert>
#include <fstream>

#ifdef GENESIS_OPENMP
#   include <omp.h>
#endif

// =================================================================================================
//      Setup
// =================================================================================================

void setup_lwr_list( CLI::App& app )
{
    // Create the options and subcommand objects.
    auto opt = std::make_shared<LwrListOptions>();
    auto sub = app.add_subcommand(
        "lwr-list",
        "Print a list of all pqueries with their likelihood weight ratios (LWRs)."
    );

    // File input
    opt->jplace_input.add_jplace_input_opt_to_app( sub );

    // How many lwrs to output
    sub->add_option(
        "--num-lwrs",
        opt->num_lwrs,
        "Number of LWR columns to print. That is, how many of the LWRs per pquery to output "
        "(most likely, second most likely, etc). If set to 0, all LWRs of each pquery are printed; "
        "as that can differ between pqueries though, the output won't be a proper table any more.",
        true
    )->group( "Settings" );

    // // Offer to ignore the check for tree compatibility
    // sub->add_flag(
    //     "--no-compatibility-check",
    //     opt->no_compat_check,
    //     "If set, disables the check for tree compatibility when multiple jplace files are provided. "
    //     "Useful if comparing results across differing reference trees."
    // )->group( "Settings" );

    // Output
    opt->file_output.add_default_output_opts_to_app( sub );

    // Set the run function as callback to be called when this subcommand is issued.
    // Hand over the options by copy, so that their shared ptr stays alive in the lambda.
    sub->callback( gappa_cli_callback(
        sub,
        {},
        [ opt ]() {
            run_lwr_list( *opt );
        }
    ));
}

// =================================================================================================
//      Run
// =================================================================================================

void run_lwr_list( LwrListOptions const& options )
{
    using namespace genesis;
    using namespace genesis::placement;
    using namespace genesis::tree;
    using namespace genesis::utils;

    // Prepare output file names and check if any of them already exists. If so, fail early.
    options.file_output.check_output_files_nonexistence( "lwr-list", "csv" );

    // Print some user output.
    options.jplace_input.print();

    // Prepare intermediate data.
    Tree tree;
    size_t file_count = 0;
    size_t pquery_count = 0;
    size_t name_count = 0;

    // Prepare list file.
    auto list_ofs = options.file_output.get_output_target( "lwr-list", "csv" );

    // Write list file header.
    (*list_ofs) << "Sample,PqueryName,Multiplicity";
    if( options.num_lwrs == 0 ) {
        (*list_ofs) << ",LWRs...";
    } else {
        for( size_t i = 0; i < options.num_lwrs; ++i ) {
            (*list_ofs) << ",LWR." << (i+1);
        }
        (*list_ofs) << ",Remainder";
    }
    (*list_ofs) << "\n";

    // Read all jplace files.
    // Here, we do one at a time, in order to get the correct output order of table rows.
    for( size_t fi = 0; fi < options.jplace_input.file_count(); ++fi ) {
        // User output
        LOG_MSG2 << "Processing file " << ( ++file_count ) << " of " << options.jplace_input.file_count()
                 << ": " << options.jplace_input.file_path( fi );

        // Read in file.
        auto sample = options.jplace_input.sample( fi );
        sort_placements_by_weight( sample );

        // Check whether the tree is the same. This is totally not needed for the calculation,
        // but the case where we want different trees to be summarized sounds more like an error.
        if( ! options.no_compat_check ) {
            // Tree
            if( tree.empty() ) {
                tree = sample.tree();
            } else if( ! genesis::placement::compatible_trees( tree, sample.tree() ) ) {
                throw std::runtime_error(
                    "Input jplace files have differing reference trees. "
                    // "(Disable this check using --no-compat-check)"
                );
            }
        }

        // Go through all pqueries and their names that are in the current file.
        for( auto const& pquery : sample ) {
            ++pquery_count;
            for( auto const& name : pquery.names() ) {
                ++name_count;
                auto const file_name = options.jplace_input.base_file_name( fi );
                (*list_ofs) << file_name << "," << name.name << "," << name.multiplicity;

                // Print the LWRs as needed, and potentially the remainder.
                if( options.num_lwrs == 0 ) {
                    // Special case: Print all LWRs - not a table any more.
                    for( size_t i = 0; i < pquery.placement_size(); ++i ) {
                        (*list_ofs) << "," << pquery.placement_at( i ).like_weight_ratio;
                    }
                } else if( options.num_lwrs < pquery.placement_size() ) {
                    // More placements than we want to print - accumuate the rest into the remainder.
                    for( size_t i = 0; i < options.num_lwrs; ++i ) {
                        (*list_ofs) << "," << pquery.placement_at( i ).like_weight_ratio;
                    }
                    double remainder = 0.0;
                    for( size_t i = options.num_lwrs; i < pquery.placement_size(); ++i ) {
                        remainder += pquery.placement_at( i ).like_weight_ratio;
                    }
                    (*list_ofs) << "," << remainder;
                } else {
                    // Fewer placements than we want to print - fill the rest of the row
                    // up with zeros, and print a zero remainder.
                    for( size_t i = 0; i < pquery.placement_size(); ++i ) {
                        (*list_ofs) << "," << pquery.placement_at( i ).like_weight_ratio;
                    }
                    for( size_t i = pquery.placement_size(); i < options.num_lwrs; ++i ) {
                        (*list_ofs) << ",0.0";
                    }
                    (*list_ofs) << ",0.0";
                }
                (*list_ofs) << "\n";
            }
        }
    }

    LOG_MSG << "Wrote " << pquery_count << " pqueries with " << name_count << " names";
}
