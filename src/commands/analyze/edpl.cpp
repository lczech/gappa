/*
    gappa - Genesis Applications for Phylogenetic Placement Analysis
    Copyright (C) 2017-2018 Lucas Czech and HITS gGmbH

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

#include "commands/analyze/edpl.hpp"

#include "options/global.hpp"

#include "CLI/CLI.hpp"

#include "genesis/placement/function/functions.hpp"
#include "genesis/placement/function/measures.hpp"
#include "genesis/placement/function/operators.hpp"

#include "genesis/tree/common_tree/distances.hpp"
#include "genesis/tree/common_tree/functions.hpp"
#include "genesis/tree/function/distances.hpp"
#include "genesis/tree/function/functions.hpp"

#include "genesis/utils/containers/matrix.hpp"
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

void setup_edpl( CLI::App& app )
{
    // Create the options and subcommand objects.
    auto opt = std::make_shared<EdplOptions>();
    auto sub = app.add_subcommand(
        "edpl",
        "Calcualte the Expected Distance between Placement Locations (EDPL) for all Pqueries."
    );

    // File input
    opt->jplace_input.add_jplace_input_opt_to_app( sub );

    // Number of histogram bins.
    sub->add_option(
        "--histogram-bins",
        opt->histogram_bins,
        "Number of histogram bins for binning the EDPL values.",
        true
    )->group( "Settings" );

    // Histogram max.
    sub->add_option(
        "--histogram-max",
        opt->histogram_max,
        "Maximum value to use in the histogram for binning the EDPL values. "
        "To use the maximal EDPL found in the samples, use a negative value (default).",
        true
    )->group( "Settings" );

    // Output
    opt->file_output.add_output_dir_opt_to_app( sub );
    opt->file_output.add_file_prefix_opt_to_app( sub, "", "edpl_" );

    // Set the run function as callback to be called when this subcommand is issued.
    // Hand over the options by copy, so that their shared ptr stays alive in the lambda.
    sub->set_callback( [ opt ]() {
        run_edpl( *opt );
    });
}

// =================================================================================================
//      Run
// =================================================================================================

void run_edpl( EdplOptions const& options )
{
    using namespace genesis;
    using namespace genesis::placement;
    using namespace genesis::tree;
    using namespace genesis::utils;

    // Prepare output file names and check if any of them already exists. If so, fail early.
    std::vector<std::string> files_to_check;
    files_to_check.push_back( options.file_output.file_prefix() + "list\\.csv" );
    files_to_check.push_back( options.file_output.file_prefix() + "histogram\\.csv" );
    options.file_output.check_nonexistent_output_files( files_to_check );

    // Print some user output.
    options.jplace_input.print();

    // Prepare intermediate data.
    Tree tree;
    utils::Matrix<double> node_distances;
    size_t file_count = 0;

    // Helper for expressiveness and conciseness.
    // Stores an edpl value for a pquery name.
    struct NameEdpl
    {
        std::string name;
        double      edpl;
    };

    // Prepare result. The outer vector is indexed by samples, the inner lists the pquery names
    // and their edpl per pquery. That is, pqueries with multiple names get multiple entries here.
    // We store this first so that the result file is written in the correct order.
    // Not nice, but the data size should be okay. If this ever leads to memory issues,
    // we need to re-think the parallelization scheme...
    auto edpl_values = std::vector<std::vector<NameEdpl>>( options.jplace_input.file_count() );

    // Read all jplace files.
    #pragma omp parallel for schedule(dynamic)
    for( size_t fi = 0; fi < options.jplace_input.file_count(); ++fi ) {

        // User output
        if( global_options.verbosity() >= 2 ) {
            #pragma omp critical(GAPPA_EDPL_PRINT_PROGRESS)
            {
                ++file_count;
                std::cout << "Processing file " << file_count << " of " << options.jplace_input.file_count();
                std::cout << ": " << options.jplace_input.file_path( fi ) << "\n";
            }
        }

        // Read in file.
        auto const sample = options.jplace_input.sample( fi );

        // The main accumulation is single threaded.
        // We could optimize more, but seriously, it is fast enough already.
        #pragma omp critical(GAPPA_EDPL_TREE)
        {
            // Tree
            if( tree.empty() ) {
                assert( node_distances.empty() );
                tree = sample.tree();
                node_distances = node_branch_length_distance_matrix( tree );
            } else if( ! genesis::placement::compatible_trees( tree, sample.tree() ) ) {
                throw std::runtime_error( "Input jplace files have differing reference trees." );
            }
            assert( ! node_distances.empty() );
        }

        // Some safety instead of an assertion.
        if(
            tree.empty() ||
            node_distances.rows() != tree.node_count() ||
            node_distances.cols() != tree.node_count()
        ) {
            throw std::runtime_error( "Internal Error: Distance matrix disagrees with tree." );
        }

        // Calculate the edpl for the sample and store it per oquery name.
        // We reserve entries for each pquery. If there are pqueries with multiple names,
        // this will lead to reallocation, but in the standard case, this is faster.
        // Also, we later copy the entries to the result, to make sure we do not store more data
        // than necessary.
        assert( fi < edpl_values.size() && edpl_values[fi].empty() );
        auto temp = std::vector<NameEdpl>( sample.size() );
        for( auto const& pquery : sample ) {
            auto const edplv = edpl( pquery, node_distances );
            for( auto const& name : pquery.names() ) {
                temp.push_back({ name.name, edplv });
            }
        }
        edpl_values[fi] = temp;
    }

    // User output
    if( global_options.verbosity() >= 1 ) {
        std::cout << "Writing output files.\n";
    }

    // Prepare list file
    auto const list_file_name = options.file_output.out_dir() + options.file_output.file_prefix() + "list.csv";
    std::ofstream list_ofs;
    file_output_stream( list_file_name, list_ofs );
    double max_edpl = - std::numeric_limits<double>::infinity();

    // Write list file. We also use the iteration to find the max edpl,
    // which we need for making the histogram.
    list_ofs << "Sample,Pquery,EDPL\n";
    for( size_t fi = 0; fi < options.jplace_input.file_count(); ++fi ) {
        auto const file_name = options.jplace_input.base_file_name( fi );

        for( auto const& entry : edpl_values[fi] ) {
            list_ofs << file_name << "," << entry.name << "," << entry.edpl << "\n";
            max_edpl = std::max( max_edpl, entry.edpl );
        }
    }
    list_ofs.close();

    // Get the max value to use for the histogram. Use a warning if needed.
    if( options.histogram_max > 0.0 && options.histogram_max < 0.75 * max_edpl ) {
        std::cout << "Warning: The maximum value for the histogram is set to less than 75% of ";
        std::cout << "the maximal value actually found in the samples. Hence, all values in ";
        std::cout << "between will be collected in the highest bin of the histogram. If this is ";
        std::cout << "intentional, you can ignore this warning.\n";
    }
    if( options.histogram_max > 0.0 && options.histogram_max > 1.25 * max_edpl ) {
        std::cout << "Warning: The maximum value for the histogram is set to more than 125% of ";
        std::cout << "the maximal value actually found in the samples. Hence, the highest bins ";
        std::cout << "of the histogram will be empty. If this is intentional, you can ignore this ";
        std::cout << "warning.\n";
    }
    auto const hist_max = options.histogram_max < 0.0 ? max_edpl : options.histogram_max;

    // Make and fill the histogram.
    auto hist = Histogram( options.histogram_bins, 0.0, hist_max );
    for( size_t fi = 0; fi < options.jplace_input.file_count(); ++fi ) {
        for( auto const& entry : edpl_values[fi] ) {
            hist.increment( entry.edpl );
        }
    }

    // Prepare histogram file
    auto const hist_file_name = options.file_output.out_dir() + options.file_output.file_prefix() + "histogram.csv";
    std::ofstream hist_ofs;
    file_output_stream( hist_file_name, hist_ofs );

    // Write histogram
    hist_ofs << "Bin,Start,End,Value,Percentage,\"Accumulated Value\",\"Accumulated Percentage\"\n";
    auto const hist_sum = sum(hist);
    double hist_acc = 0.0;
    for( size_t i = 0; i < hist.bins(); ++i ) {
        hist_acc += hist[i];
        hist_ofs << i << "," << hist.bin_range(i).first << "," << hist.bin_range(i).second << ",";
        hist_ofs << hist[i] << "," << ( hist[i] / hist_sum ) << ",";
        hist_ofs << hist_acc << "," << ( hist_acc / hist_sum ) << "\n";
    }
    hist_ofs.close();
}
