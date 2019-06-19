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

#include "commands/examine/info.hpp"

#include "options/global.hpp"
#include "tools/cli_setup.hpp"

#include "CLI/CLI.hpp"

#include "genesis/tree/function/functions.hpp"

#include <algorithm>
#include <cassert>
#include <ios>
#include <iomanip>

#ifdef GENESIS_OPENMP
#   include <omp.h>
#endif

// =================================================================================================
//      Setup
// =================================================================================================

void setup_info( CLI::App& app )
{
    // Create the options and subcommand objects.
    auto opt = std::make_shared<InfoOptions>();
    auto sub = app.add_subcommand(
        "info",
        "Print basic information about placement files."
    );

    // File input
    opt->jplace_input.add_jplace_input_opt_to_app( sub );

    // Set the run function as callback to be called when this subcommand is issued.
    // Hand over the options by copy, so that their shared ptr stays alive in the lambda.
    sub->callback( gappa_cli_callback(
        sub,
        {},
        [ opt ]() {
            run_info( *opt );
        }
    ));
}

// =================================================================================================
//      Run
// =================================================================================================

void run_info( InfoOptions const& options )
{
    using namespace genesis;
    using namespace genesis::placement;
    using namespace genesis::tree;
    using namespace genesis::utils;

    // Print some user output.
    options.jplace_input.print();

    // Helper for expressiveness and conciseness.
    struct SampleInfo
    {
        std::string name;
        size_t      branches;
        size_t      leaves;
        size_t      pqueries;
    };

    // Prepare result. The vector is indexed by samples.
    auto sample_infos = std::vector<SampleInfo>( options.jplace_input.file_count() );
    size_t name_width = 0;

    // Read all jplace files.
    size_t file_count = 0;
    #pragma omp parallel for schedule(dynamic)
    for( size_t fi = 0; fi < options.jplace_input.file_count(); ++fi ) {

        // User output
        LOG_MSG2 << "Processing file " << ( ++file_count ) << " of " << options.jplace_input.file_count()
                 << ": " << options.jplace_input.file_path( fi );

        // Read in file.
        auto sample = options.jplace_input.sample( fi );

        // Store result.
        assert( fi < sample_infos.size() );
        sample_infos[fi].name = options.jplace_input.base_file_name( fi );
        sample_infos[fi].branches = sample.tree().edge_count();
        sample_infos[fi].leaves = genesis::tree::leaf_node_count( sample.tree() );
        sample_infos[fi].pqueries = sample.size();

        name_width = std::max( name_width, sample_infos[fi].name.size() );
    }

    LOG_BOLD;
    LOG_MSG1 << std::left << std::setw( name_width + 1 ) << "Sample"
             << "    Branches      Leaves    Pqueries";
    for( auto const& info : sample_infos ) {
        LOG_MSG1 << std::left << std::setw( name_width + 1 ) << info.name
                 << std::right << std::setw( 12 ) << info.branches
                 << std::right << std::setw( 12 ) << info.leaves
                 << std::right << std::setw( 12 ) << info.pqueries;
    }
}
