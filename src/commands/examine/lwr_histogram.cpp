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

#include "commands/examine/lwr_histogram.hpp"

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

void setup_lwr_histogram( CLI::App& app )
{
    // Create the options and subcommand objects.
    auto opt = std::make_shared<LwrHistogramOptions>();
    auto sub = app.add_subcommand(
        "lwr-histogram",
        "Print a table with histograms of the likelihood weight ratios (LWRs) of all pqueries."
    );

    // File input
    opt->jplace_input.add_jplace_input_opt_to_app( sub );

    // Multiplicities.
    opt->jplace_input.add_ignore_multiplicities_opt_to_app( sub );

    // Number of histogram bins.
    sub->add_option(
        "--histogram-bins",
        opt->histogram_bins,
        "Number of histogram bins for binning the LWR values. "
        "This is the number of rows of the output table.",
        true
    )->group( "Settings" );

    // How many lwrs to output
    sub->add_option(
        "--num-lwrs",
        opt->num_lwrs,
        "Number of histograms to print. That is, how many of the LWRs per pquery to output "
        "(most likely, second most likely, etc), or in other words, how many LWR columns "
        "the output table should have.",
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
            run_lwr_histogram( *opt );
        }
    ));
}

// =================================================================================================
//      Run
// =================================================================================================

void run_lwr_histogram( LwrHistogramOptions const& options )
{
    using namespace genesis;
    using namespace genesis::placement;
    using namespace genesis::tree;
    using namespace genesis::utils;

    // Prepare output file names and check if any of them already exists. If so, fail early.
    options.file_output.check_output_files_nonexistence( "lwr-histogram", "csv" );

    // Print some user output.
    options.jplace_input.print();

    // Prepare intermediate data.
    Tree tree;
    size_t file_count = 0;
    size_t pquery_count = 0;
    size_t name_count = 0;
    auto hists = std::vector<Histogram>( options.num_lwrs + 1, { options.histogram_bins, 0.0, 1.0 });

    // Read all jplace files.
    #pragma omp parallel for schedule(dynamic)
    for( size_t fi = 0; fi < options.jplace_input.file_count(); ++fi ) {

        // User output
        LOG_MSG2 << "Processing file " << ( ++file_count ) << " of " << options.jplace_input.file_count()
                 << ": " << options.jplace_input.file_path( fi );

        // Read in file.
        auto sample = options.jplace_input.sample( fi );
        sort_placements_by_weight( sample );

        // Check whether the tree is the same. This is totally not needed for the calculation,
        // but the case where we want different trees to be summarized sounds more like and error.
        if( ! options.no_compat_check ) {
            #pragma omp critical(GAPPA_LWR_TREE)
            {
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
        }

        // The main accumulation is single threaded.
        // We could optimize more, but seriously, it is fast enough already.
        #pragma omp critical(GAPPA_LWR_HIST_ACC)
        {
            for( auto const& pquery : sample ) {
                ++pquery_count;
                name_count += pquery.name_size();
                auto const mult = total_multiplicity( pquery );
                auto const max_n = std::min( options.num_lwrs, pquery.placement_size() );
                for( size_t n = 0; n < max_n; ++n ) {
                    hists[n].accumulate( pquery.placement_at( n ).like_weight_ratio, mult );
                }
                for( size_t n = max_n; n < pquery.placement_size(); ++n ) {
                    hists.back().accumulate( pquery.placement_at( n ).like_weight_ratio, mult );
                }
            }
        }
    }

    // User output
    LOG_MSG1 << "Writing output table.";

    // Prepare histogram file
    auto hist_ofs = options.file_output.get_output_target( "lwr-histogram", "csv" );

    // Write histogram header, and get sum for each of them
    // so that we can later use them for normalization.
    (*hist_ofs) << "Bin,Start,End,Range";
    auto hist_sums = std::vector<double>( options.num_lwrs + 1, 0.0 );
    for( size_t i = 0; i < options.num_lwrs; ++i ) {
        (*hist_ofs) << ",Value." << (i+1);
        hist_sums[i] = sum( hists[i] );
    }
    (*hist_ofs) << ",Value.Remainder";
    hist_sums.back() = sum( hists.back() );
    for( size_t i = 0; i < options.num_lwrs; ++i ) {
        (*hist_ofs) << ",Percentage." << (i+1);
    }
    (*hist_ofs) << ",Percentage.Remainder";
    for( size_t i = 0; i < options.num_lwrs; ++i ) {
        (*hist_ofs) << ",AccumulatedValue." << (i+1);
    }
    (*hist_ofs) << ",AccumulatedValue.Remainder";
    for( size_t i = 0; i < options.num_lwrs; ++i ) {
        (*hist_ofs) << ",AccumulatedPercentage." << (i+1);
    }
    (*hist_ofs) << ",AccumulatedPercentage.Remainder";
    (*hist_ofs) << "\n";

    // Write histogram header
    auto hist_accs = std::vector<double>( options.num_lwrs + 1, 0.0 );
    for( size_t b = 0; b < options.histogram_bins; ++b ) {
        (*hist_ofs) << b << ",";
        (*hist_ofs) << hists[0].bin_range(b).first << "," << hists[0].bin_range(b).second;
        (*hist_ofs) << ",\"[" << hists[0].bin_range(b).first << ", ";
        (*hist_ofs) << hists[0].bin_range(b).second << ")\"";

        for( size_t n = 0; n < options.num_lwrs + 1; ++n ) {
            hist_accs[n] += hists[n][b];
            (*hist_ofs) << "," << hists[n][b];
        }
        for( size_t n = 0; n < options.num_lwrs + 1; ++n ) {
            (*hist_ofs) << "," << ( hists[n][b] / hist_sums[n] );
        }
        for( size_t n = 0; n < options.num_lwrs + 1; ++n ) {
            (*hist_ofs) << "," << hist_accs[n];
        }
        for( size_t n = 0; n < options.num_lwrs + 1; ++n ) {
            (*hist_ofs) << "," << ( hist_accs[n] / hist_sums[n] );
        }
        (*hist_ofs) << "\n";
    }

    LOG_MSG << "Wrote " << pquery_count << " pqueries with " << name_count << " names";
}
