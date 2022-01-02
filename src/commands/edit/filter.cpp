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

#include "commands/edit/filter.hpp"

#include "options/global.hpp"
#include "tools/cli_setup.hpp"

#include "CLI/CLI.hpp"

#include "genesis/placement/formats/jplace_writer.hpp"
#include "genesis/placement/function/functions.hpp"
#include "genesis/placement/function/masses.hpp"
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

void setup_filter( CLI::App& app )
{
    // Create the options and subcommand objects.
    auto options = std::make_shared<FilterOptions>();
    auto sub = app.add_subcommand(
        "filter",
        "Filter jplace files according to some criteria, that is, remove all queries and/or "
        "placement locations that do not pass the provided filter(s)."
    );

    // -----------------------------------------------------------
    //     Input options
    // -----------------------------------------------------------

    // Jplace input
    options->jplace_input.add_jplace_input_opt_to_app( sub );

    // // Merge input
    // options->merge_input = sub->add_flag(
    //     "--merge-input",
    //     options->merge_input.value(),
    //     "If multiple jplace files are provided as input, merge them into one file prior to "
    //     "filtering. This requires all of them to have the same reference tree. If this flag "
    //     "is not set, all files are filtered individually instead."
    // );
    // options->merge_input.option()->group( "Input" );

    // -----------------------------------------------------------
    //     Filter placement options
    // -----------------------------------------------------------

    // Normalize before
    options->normalize_before = sub->add_flag(
        "--normalize-before",
        options->normalize_before.value(),
        "Before filtering placements, normalize the initial placement masses (likelihood "
        "weight ratios) by proportially scaling them so that they sum to one per pquery. "
    );
    options->normalize_before.option()->group( "Placement Filters" );

    // Min accumulated mass
    options->min_accumulated_mass = sub->add_option(
        "--min-accumulated-mass",
        options->min_accumulated_mass.value(),
        "Only keep the most likely placements per query so that their accumulated mass is above "
        "the given minimum value."
    );
    options->min_accumulated_mass.option()->group( "Placement Filters" );
    options->min_accumulated_mass.option()->check( CLI::Range( 0.0, 1.0 ));

    // Min mass threshold
    options->min_mass_threshold = sub->add_option(
        "--min-mass-threshold",
        options->min_mass_threshold.value(),
        "Only keep those placements per query whose mass is above the given minimum threshold."
    );
    options->min_mass_threshold.option()->group( "Placement Filters" );
    options->min_mass_threshold.option()->check( CLI::Range( 0.0, 1.0 ));

    // Min accumulated mass
    options->max_n_placements = sub->add_option(
        "--max-n-placements",
        options->max_n_placements.value(),
        "Only keep the n most likely placements per query."
    );
    options->max_n_placements.option()->group( "Placement Filters" );

    // Remove empty
    options->no_remove_empty = sub->add_flag(
        "--no-remove-empty",
        options->no_remove_empty.value(),
        "After filtering placements, there might be pqueries that do not have any placement locations "
        "remaining. By default, the whole pquery is removed in this case, as it is useless. "
        "However, if this flag is set, they are kept as empty pqueries with just their name."
    );
    options->no_remove_empty.option()->group( "Placement Filters" );

    // Normalize after
    options->normalize_after = sub->add_flag(
        "--normalize-after",
        options->normalize_after.value(),
        "After filtering placements, normalize the remaining placement masses (likelihood "
        "weight ratios) by proportially scaling them so that they sum to one per pquery."
    );
    options->normalize_after.option()->group( "Placement Filters" );

    // -----------------------------------------------------------
    //     Filter names options
    // -----------------------------------------------------------

    // Keep names
    options->keep_names = sub->add_option(
        "--keep-names",
        options->keep_names.value(),
        "Keep queries whose name matches the given names, which can be provided either as a "
        "regular expression (regex), or as a file with one name per line. Remove all others."
    );
    options->keep_names.option()->group( "Name Filters" );

    // Removing names
    options->remove_names = sub->add_option(
        "--remove-names",
        options->remove_names.value(),
        "Remove queries whose name matches the given names, which can be provided either as a "
        "regular expression (regex), or as a file with one name per line. Keep all others."
    );
    options->remove_names.option()->group( "Name Filters" );

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
            run_filter( *options );
        }
    ));
}

// =================================================================================================
//      Run Helpers
// =================================================================================================

void filter_sample( FilterOptions const& options, genesis::placement::Sample& sample )
{
    using namespace genesis;
    using namespace genesis::placement;
    using namespace genesis::utils;

    // Normalize before, if needed
    if( options.normalize_before.value() ) {
        normalize_weight_ratios( sample );
    }

    // Some user output prep
    auto placement_count = total_placement_count( sample );

    // Min accumulated mass
    if( options.min_accumulated_mass ) {
        filter_min_accumulated_weight( sample, options.min_accumulated_mass.value() );

        // User output
        auto new_placement_count = total_placement_count( sample );
        assert( new_placement_count <= placement_count );
        LOG_MSG1 << "Removed " << ( placement_count - new_placement_count ) << " placement locations "
                 << "due to " << options.min_accumulated_mass.option()->get_name() << " filtering.";
        placement_count = new_placement_count;
    }

    // Min mass threshold
    if( options.min_mass_threshold ) {
        filter_min_weight_threshold( sample, options.min_mass_threshold.value() );

        // User output
        auto new_placement_count = total_placement_count( sample );
        assert( new_placement_count <= placement_count );
        LOG_MSG1 << "Removed " << ( placement_count - new_placement_count ) << " placement locations "
                 << "due to " << options.min_mass_threshold.option()->get_name() << " filtering.";
        placement_count = new_placement_count;
    }

    // Max n placements
    if( options.max_n_placements ) {
        if( options.max_n_placements.value() == 0 ) {
            throw CLI::ValidationError(
                "--max-n-placements (" + std::to_string( options.max_n_placements.value() ) +  ")",
                "Invalid value; has to be > 0, as otherwise all placements would be removed "
                "from the query."
            );
        }
        filter_n_max_weight_placements( sample, options.max_n_placements.value() );

        // User output
        auto new_placement_count = total_placement_count( sample );
        assert( new_placement_count <= placement_count );
        LOG_MSG1 << "Removed " << ( placement_count - new_placement_count ) << " placement locations "
                 << "due to " << options.max_n_placements.option()->get_name() << " filtering.";
        placement_count = new_placement_count;
    }

    // Some user output prep
    auto name_count = total_name_count( sample );

    // Finally, remove all pqueries that have no placements left after the above filtering,
    // and normalize if needed. We remove first, so that the normalization does not need
    // to operate on empty pqueries, which currently throws and exception in genesis.
    if( ! options.no_remove_empty.value() ) {
        remove_empty_placement_pqueries( sample );

        // User output
        auto new_name_count = total_name_count( sample );
        assert( new_name_count <= name_count );
        if( new_name_count != name_count ) {
            LOG_MSG1 << "Removed " << ( name_count - new_name_count ) << " placement names / pqueries "
            << "which did not contain any placement locations after placement filtering. Use "
            << options.no_remove_empty.option()->get_name() << " to change this behavior.";
        }
        name_count = new_name_count;
    }
    if( options.normalize_after.value() ) {
        normalize_weight_ratios( sample );
    }

    // Keeping names
    if( options.keep_names ) {
        if( is_file( options.keep_names.value() )) {
            // Bit expensive to do the double conversion here, but for this use case,
            // that should suffice.
            auto const list = file_read_lines( options.keep_names.value() );
            auto const set = std::unordered_set<std::string>( list.begin(), list.end() );
            filter_pqueries_keeping_names( sample, set );
        } else {
            // Assume it's a regex.
            filter_pqueries_keeping_names( sample, options.keep_names.value() );
        }

        // User output
        auto new_name_count = total_name_count( sample );
        assert( new_name_count <= name_count );
        LOG_MSG1 << "Removed " << ( name_count - new_name_count ) << " placement names / pqueries "
                 << "due to " << options.keep_names.option()->get_name() << " filtering.";
        name_count = new_name_count;
    }

    // Removing names
    if( options.remove_names ) {
        if( is_file( options.remove_names.value() )) {
            // Bit expensive to do the double conversion here, but for this use case,
            // that should suffice.
            auto const list = file_read_lines( options.remove_names.value() );
            auto const set = std::unordered_set<std::string>( list.begin(), list.end() );
            filter_pqueries_removing_names( sample, set );
        } else {
            // Assume it's a regex.
            filter_pqueries_removing_names( sample, options.remove_names.value() );
        }

        // User output
        auto new_name_count = total_name_count( sample );
        assert( new_name_count <= name_count );
        LOG_MSG1 << "Removed " << ( name_count - new_name_count ) << " placement names / pqueries "
                 << "due to " << options.remove_names.option()->get_name() << " filtering.";
        name_count = new_name_count;
    }
}

// =================================================================================================
//      Run
// =================================================================================================

void run_filter( FilterOptions const& options )
{
    using namespace genesis;
    using namespace genesis::placement;
    using namespace genesis::utils;

    // Check if any of the files we are going to produce already exists. If so, fail early.
    options.jplace_output.check_output_files_nonexistence( "filter", "jplace" );

    // Print some user output.
    options.jplace_input.print();

    // User output.
    LOG_MSG1 << "Reading " << options.jplace_input.file_count() << " sample"
             << ( options.jplace_input.file_count() > 1 ? "s" : "" ) << ".";

    // Get all queries of all samples. Requires that all have the same ref tree.
    auto sample = options.jplace_input.merged_samples();

    // Run the filtering.
    filter_sample( options, sample );

    // Write the new sample to a file.
    JplaceWriter().write(
        sample,
        options.jplace_output.get_output_target( "filter", "jplace" )
    );
}
