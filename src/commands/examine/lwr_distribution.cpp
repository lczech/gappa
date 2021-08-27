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

#include "commands/examine/lwr_distribution.hpp"

#include "options/global.hpp"
#include "tools/cli_setup.hpp"

#include "CLI/CLI.hpp"

#include "genesis/placement/function/functions.hpp"
#include "genesis/placement/function/masses.hpp"
#include "genesis/placement/function/operators.hpp"

#include "genesis/utils/io/output_stream.hpp"
#include "genesis/utils/math/histogram.hpp"
#include "genesis/utils/math/histogram/stats.hpp"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <utility>
#include <vector>

#ifdef GENESIS_OPENMP
#   include <omp.h>
#endif

// =================================================================================================
//      Setup
// =================================================================================================

void setup_lwr_distribution( CLI::App& app )
{
    // Create the options and subcommand objects.
    auto opt = std::make_shared<LwrDistributionOptions>();
    auto sub = app.add_subcommand(
        "lwr-distribution",
        "Print a summary table that represents the distribution "
        "of the likelihood weight ratios (LWRs) of all pqueries."
    );

    // File input
    opt->jplace_input.add_jplace_input_opt_to_app( sub );

    // Multiplicities. Not taken into account.
    // opt->jplace_input.add_ignore_multiplicities_opt_to_app( sub );

    // Number of histogram bins.
    sub->add_option(
        "--num-entries",
        opt->num_entries,
        "Number of entries representing the pqueries. This is the length of the output table, "
        "representing the pquery LWR distribution. If set to 0, or if the input has fewer "
        "pqueries that the given number, the output table will contain all pqueries.",
        true
    )->group( "Settings" );

    // How many lwrs to output
    sub->add_option(
        "--num-lwrs",
        opt->num_lwrs,
        "Number of LWRs per pquery to output (the most likely, second most likely, etc); "
        "all remaining LWRs are accumulated into the Remainder column. "
        "This is the number of LWR columns of the output table.",
        true
    )->group( "Settings" );

    // How to sort the per-pquery LWRs to create the output table.
    sub->add_flag(
        "--numerical-sort",
        opt->numerical_sort,
        "By default, we sort the entries in the output table using a weighted sum of the LWRs "
        "of each pquery, with weight 1 for the most likely LWR, "
        "weight 1/2 for the second most likely LWR, weight 1/3 for the third most likely, etc."
        "If this option is set however, the entries in the output table are sorted by "
        "the most likely LWR first, then sorting identical entries by the second most "
        "likely LWR, and so forth."
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
            run_lwr_distribution( *opt );
        }
    ));
}

// =================================================================================================
//      Run
// =================================================================================================

void run_lwr_distribution( LwrDistributionOptions const& options )
{
    using namespace genesis;
    using namespace genesis::placement;
    using namespace genesis::tree;
    using namespace genesis::utils;

    // Prepare output file names and check if any of them already exists. If so, fail early.
    options.file_output.check_output_files_nonexistence( "lwr-distribution", "csv" );

    // Print some user output.
    options.jplace_input.print();

    // Prepare intermediate data.
    Tree tree;
    size_t file_count = 0;

    // Prepare collection of all LWRs, for each pquery.
    struct LwrEntry
    {
        // We only store the index to the name,
        // in order to not have repeated string allocations all over the place.
        size_t              sample_index;
        std::string         pquery_name;

        // Store the weighted sum sort value. Only used for sorting if numerical_sort is false.
        double              sort_value;

        // The actual list of LWRs of this pquery, containing the n most likely LWRs and the
        // accumulated remainder of all LWRs above n as an additional last entry.
        std::vector<double> lwrs;
    };
    auto collection = std::vector<LwrEntry>();

    // Read all jplace files.
    #pragma omp parallel for schedule(dynamic)
    for( size_t fi = 0; fi < options.jplace_input.file_count(); ++fi ) {

        // User output
        ++file_count;
        LOG_MSG2 << "Processing file " << ( file_count ) << " of "
                 << options.jplace_input.file_count()
                 << ": " << options.jplace_input.file_path( fi );

        // Read in file.
        auto sample = options.jplace_input.sample( fi );
        sort_placements_by_weight( sample );

        // Check whether the tree is the same. This is totally not needed for the calculation,
        // but the case where we want different trees to be summarized sounds more like and error.
        if( ! options.no_compat_check ) {
            #pragma omp critical(GAPPA_LWR_DIST_TREE)
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

        // Reserve space for the sample. In the standard case of having one input file,
        // this saves time for repeated reallocations. For multiple files, this still should
        // yield some speedup. We use the current capacity as a proxy for the current size
        // (we cannot use actual size, due to the parallel loop). This might cause the vector
        // to overshoot the needed size, but we can live with that.
        auto const needed_extra_cap = total_name_count( sample );
        #pragma omp critical(GAPPA_LWR_DIST_COLLECTION)
        {
            collection.reserve( collection.capacity() + needed_extra_cap );
        }

        // The main accumulation is single threaded.
        // We could optimize more, but seriously, it is fast enough already.
        for( auto& pquery : sample ) {

            // Prepare the vector with all top n LWRs, and the remainder.
            // Also, compute the sort value for our default sort order.
            auto lwrs = std::vector<double>( options.num_lwrs + 1, 0.0 );
            double sort_value = 0.0;
            auto const max_n = std::min( options.num_lwrs, pquery.placement_size() );
            for( size_t n = 0; n < max_n; ++n ) {
                auto const lwr = pquery.placement_at( n ).like_weight_ratio;
                lwrs[n] = lwr;
                sort_value += lwr / static_cast<double>( n + 1 );
            }
            for( size_t n = max_n; n < pquery.placement_size(); ++n ) {
                auto const lwr = pquery.placement_at( n ).like_weight_ratio;
                lwrs.back() += lwr;
                sort_value += lwr / static_cast<double>( n + 1 );
            }

            // Now add the LWRs to the collection.
            #pragma omp critical(GAPPA_LWR_DIST_COLLECTION)
            {
                // Add the values as often as the pquery has names,
                // as each of them represents a different pquery.
                LwrEntry entry;
                entry.sample_index = fi;
                entry.sort_value = sort_value;
                entry.lwrs = std::move(lwrs);
                for( auto const& name : pquery.names() ) {
                    entry.pquery_name = name.name;
                    collection.emplace_back( entry );
                }
            }

            // We are done with this pquery, and will never need it again.
            // Let's free its memory, because we just did more or less a full copy of its memory
            // footprint, so let's save that!
            pquery.clear_placements();
            pquery.clear_names();
        }
    }
    LOG_MSG1 << "Found " << collection.size() << " pqueries";

    // Sort according to needs
    LOG_MSG1 << "Sorting pqueries by LWR";
    if( options.numerical_sort ) {
        std::sort(
            collection.begin(), collection.end(),
            []( LwrEntry const& lhs, LwrEntry const& rhs ){
                assert( lhs.lwrs.size() == rhs.lwrs.size() );

                // Sort by LWR, starting with the most likely one.
                // If those are identical, compare the second most likely, and so forth,
                // until we find one that differs and which then gives the sort order.
                // We sort by largest LWR first.
                for( size_t i = 0; i < lhs.lwrs.size(); ++i ) {
                    if( lhs.lwrs[i] != rhs.lwrs[i] ) {
                        return lhs.lwrs[i] > rhs.lwrs[i];
                    }
                }
                return false;
            }
        );
    } else {
        std::sort(
            collection.begin(), collection.end(),
            []( LwrEntry const& lhs, LwrEntry const& rhs ){
                // Simply sort by the pre-computed sort value.
                // We sort by largest LWR sort value first.
                return lhs.sort_value > rhs.sort_value;
            }
        );
    }

    // Get the number of rows to write. In the normal case, we write `num_entries` many rows,
    // picking them at the respective relative position in the sorted collection.
    // If there are not that many pqueries, we only write as many as there are;
    // if the number of output entries set by the user is 0, we also write all that there are.
    auto const max_e
        = ( options.num_entries == 0 || collection.size() < options.num_entries )
        ? collection.size()
        : options.num_entries
    ;

    // User output
    if( max_e == collection.size() ) {
        LOG_MSG1 << "Writing output table with all " << max_e << " pqueries";
    } else {
        LOG_MSG1 << "Writing output table with " << max_e << " representative pqueries";
    }

    // Prepare file
    auto ofs = options.file_output.get_output_target( "lwr-distribution", "csv" );

    // Write header, using an R-friendly format with dots as delimiters in column names.
    (*ofs) << "Index,Sample,PqueryName";
    for( size_t i = 0; i < options.num_lwrs; ++i ) {
        (*ofs) << ",LWR." << (i+1);
    }
    (*ofs) << ",Remainder\n";

    // Write data rows.
    for( size_t e = 0; e < max_e; ++e ) {

        // Get the index of the pquery that sits at the e/max_e position in the collection.
        // We compute it so that the first and the last entry of the collection are always
        // part of the output.
        auto const index = static_cast<size_t>(
            static_cast<double>( collection.size() - 1 ) *
            static_cast<double>( e ) /
            static_cast<double>( max_e - 1 )
        );
        assert( index < collection.size() );
        assert( collection[index].lwrs.size() == options.num_lwrs + 1 );

        // Print the entry with all its LWRs.
        (*ofs) << (index + 1);
        (*ofs) << "," << options.jplace_input.base_file_name( collection[index].sample_index );
        (*ofs) << "," << collection[index].pquery_name;
        for( auto const& v : collection[index].lwrs ) {
            (*ofs) << "," << v;
        }
        (*ofs) << "\n";
    }
}
