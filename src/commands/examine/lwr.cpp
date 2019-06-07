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

#include "commands/examine/lwr.hpp"

#include "options/global.hpp"

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

void setup_lwr( CLI::App& app )
{
    // Create the options and subcommand objects.
    auto opt = std::make_shared<LwrOptions>();
    auto sub = app.add_subcommand(
        "lwr",
        "Print histograms of the likelihood weight ratios (LWRs) of all pqueries."
    );

    // File input
    opt->jplace_input.add_jplace_input_opt_to_app( sub );

    // Multiplicities.
    opt->jplace_input.add_ignore_multiplicities_opt_to_app( sub );

    // Number of histogram bins.
    sub->add_option(
        "--histogram-bins",
        opt->histogram_bins,
        "Number of histogram bins for binning the LWR values.",
        true
    )->group( "Settings" );

    // How many lwrs to output
    sub->add_option(
        "--num-lwrs",
        opt->num_lwrs,
        "Number of histograms to print. That is, how many of the LWRs per pquery to output "
        "(most likely, second most likely, etc), or in other words, how many LWRs columns "
        "the output table should have.",
        true
    )->group( "Settings" );

    // Offer to skip list file
    sub->add_flag(
        "--no-list-file",
        opt->no_list_file,
        "If set, do not write out the LWRs per pquery, but just the histogram file. "
        "As the list needs to keep all pquery names in memory (to get the correct order), "
        "the memory requirements might be too large. In that case, this option can help."
    )->group( "Settings" );

    // Output
    opt->file_output.add_output_dir_opt_to_app( sub );
    opt->file_output.add_file_prefix_opt_to_app( sub, "", "lwr_" );

    // Set the run function as callback to be called when this subcommand is issued.
    // Hand over the options by copy, so that their shared ptr stays alive in the lambda.
    sub->callback( [ opt ]() {
        run_lwr( *opt );
    });
}

// =================================================================================================
//      Run
// =================================================================================================

void run_lwr( LwrOptions const& options )
{
    using namespace genesis;
    using namespace genesis::placement;
    using namespace genesis::tree;
    using namespace genesis::utils;

    // TODO does also fail if the list is not written.
    // Prepare output file names and check if any of them already exists. If so, fail early.
    std::vector<std::string> files_to_check;
    files_to_check.push_back( options.file_output.file_prefix() + "list\\.csv" );
    files_to_check.push_back( options.file_output.file_prefix() + "histogram\\.csv" );
    options.file_output.check_nonexistent_output_files( files_to_check );

    // Print some user output.
    options.jplace_input.print();

    // Prepare intermediate data.
    Tree tree;
    size_t file_count = 0;
    auto hists = std::vector<Histogram>( options.num_lwrs, { options.histogram_bins, 0.0, 1.0 });

    // Helper for expressiveness and conciseness.
    // Stores LWR values for a pquery name.
    struct NameLWRs
    {
        std::string         name;
        double              mult;
        std::vector<double> lwr;
    };

    // Prepare result. The outer vector is indexed by samples, the inner lists the pquery names
    // and their edpl per pquery. That is, pqueries with multiple names get multiple entries here.
    // We store this first so that the result file is written in the correct order.
    // Not nice, but the data size should be okay. If this ever leads to memory issues,
    // we need to re-think the parallelization scheme...
    auto lwrs_values = std::vector<std::vector<NameLWRs>>( options.jplace_input.file_count() );

    // Read all jplace files.
    #pragma omp parallel for schedule(dynamic)
    for( size_t fi = 0; fi < options.jplace_input.file_count(); ++fi ) {

        // User output
        if( global_options.verbosity() >= 2 ) {
            #pragma omp critical(GAPPA_LWR_PRINT_PROGRESS)
            {
                ++file_count;
                std::cout << "Processing file " << file_count << " of " << options.jplace_input.file_count();
                std::cout << ": " << options.jplace_input.file_path( fi ) << "\n";
            }
        }

        // Read in file.
        auto sample = options.jplace_input.sample( fi );
        sort_placements_by_weight( sample );

        // Check whether the tree is the same. This is totally not needed for the calculation,
        // but the case where we want different trees to be summarized sounds more like and error.
        #pragma omp critical(GAPPA_LWR_TREE)
        {
            // Tree
            if( tree.empty() ) {
                tree = sample.tree();
            } else if( ! genesis::placement::compatible_trees( tree, sample.tree() ) ) {
                throw std::runtime_error( "Input jplace files have differing reference trees." );
            }
        }

        // The main accumulation is single threaded.
        // We could optimize more, but seriously, it is fast enough already.
        #pragma omp critical(GAPPA_LWR_HIST_ACC)
        {
            for( auto const& pquery : sample ) {
                auto const mult = total_multiplicity( pquery );
                auto const max_n = std::min( options.num_lwrs, pquery.placement_size() );
                for( size_t n = 0; n < max_n; ++n ) {
                    hists[n].accumulate( pquery.placement_at( n ).like_weight_ratio, mult );
                }
            }
        }

        // Store the LWRs for the sample and store it per pquery name.
        // We reserve entries for each pquery. If there are pqueries with multiple names,
        // this will lead to reallocation, but in the standard case, this is faster.
        // Also, we later copy the entries to the result, to make sure we do not store more data
        // than necessary.
        if( ! options.no_list_file ) {
            auto temp = std::vector<NameLWRs>();
            temp.reserve( sample.size() );

            for( auto const& pquery : sample ) {
                for( auto const& name : pquery.names() ) {
                    temp.push_back({ name.name, name.multiplicity, {} });
                    for( size_t n = 0; n < options.num_lwrs; ++n ) {

                        // We always write a value, so that the number of columns is constant.
                        // Uses extra space... Could be optimized.
                        if( n < pquery.placement_size() ) {
                            temp.back().lwr.push_back( pquery.placement_at( n ).like_weight_ratio );
                        } else {
                            temp.back().lwr.push_back( 0.0 );
                        }
                    }
                }
            }

            // Store result.
            assert( fi < lwrs_values.size() && lwrs_values[fi].empty() );
            lwrs_values[fi] = temp;
        }
    }

    // User output
    if( global_options.verbosity() >= 1 ) {
        std::cout << "Writing output files.\n";
    }

    if( ! options.no_list_file ) {
        // Prepare list file
        auto const list_file_name = options.file_output.out_dir() + options.file_output.file_prefix() + "list.csv";
        std::ofstream list_ofs;
        file_output_stream( list_file_name, list_ofs );

        // Write list file.
        list_ofs << "Sample,Pquery,Multiplicity";
        for( size_t i = 0; i < options.num_lwrs; ++i ) {
            list_ofs << ",\"LWR " << (i+1) << "\"";
        }
        list_ofs << "\n";

        for( size_t fi = 0; fi < options.jplace_input.file_count(); ++fi ) {
            auto const file_name = options.jplace_input.base_file_name( fi );

            for( auto const& entry : lwrs_values[fi] ) {
                list_ofs << file_name << "," << entry.name << "," << entry.mult;
                for( size_t i = 0; i < options.num_lwrs; ++i ) {
                    list_ofs << "," << entry.lwr[i] ;
                }
                list_ofs << "\n";
            }
        }
        list_ofs.close();
    }

    // Prepare histogram file
    auto const hist_file_name = options.file_output.out_dir() + options.file_output.file_prefix() + "histogram.csv";
    std::ofstream hist_ofs;
    file_output_stream( hist_file_name, hist_ofs );

    // Write histogram header, and get sum for each of them.
    hist_ofs << "Bin,Start,End,Range";
    auto hist_sums = std::vector<double>( options.num_lwrs, 0.0 );
    auto hist_accs = std::vector<double>( options.num_lwrs, 0.0 );
    for( size_t i = 0; i < options.num_lwrs; ++i ) {
        hist_ofs << ",\"Value " << (i+1) << "\"";
        hist_sums[i] = sum( hists[i] );
    }
    for( size_t i = 0; i < options.num_lwrs; ++i ) {
        hist_ofs << ",\"Percentage " << (i+1) << "\"";
    }
    for( size_t i = 0; i < options.num_lwrs; ++i ) {
        hist_ofs << ",\"Accumulated Value " << (i+1) << "\"";
    }
    for( size_t i = 0; i < options.num_lwrs; ++i ) {
        hist_ofs << ",\"Accumulated Percentage " << (i+1) << "\"";
    }
    hist_ofs << "\n";

    // Write histogram header
    for( size_t b = 0; b < options.histogram_bins; ++b ) {
        hist_ofs << b << ",";
        hist_ofs << hists[0].bin_range(b).first << "," << hists[0].bin_range(b).second;
        hist_ofs << ",\"[" << hists[0].bin_range(b).first << ", " << hists[0].bin_range(b).second << ")\"";

        for( size_t n = 0; n < options.num_lwrs; ++n ) {
            hist_accs[n] += hists[n][b];
            hist_ofs << "," << hists[n][b];
        }
        for( size_t n = 0; n < options.num_lwrs; ++n ) {
            hist_ofs << "," << ( hists[n][b] / hist_sums[n] );
        }
        for( size_t n = 0; n < options.num_lwrs; ++n ) {
            hist_ofs << "," << hist_accs[n];
        }
        for( size_t n = 0; n < options.num_lwrs; ++n ) {
            hist_ofs << "," << ( hist_accs[n] / hist_sums[n] );
        }

        hist_ofs << "\n";
    }
    hist_ofs.close();
}
