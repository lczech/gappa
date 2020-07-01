/*
    gappa - Genesis Applications for Phylogenetic Placement Analysis
    Copyright (C) 2017-2020 Lucas Czech, Pierre Barbera and HITS gGmbH

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

#include "commands/sequence/cat.hpp"

#include "options/global.hpp"
#include "tools/cli_setup.hpp"
#include "tools/misc.hpp"

#include "CLI/CLI.hpp"

#include "genesis/sequence/formats/fasta_input_iterator.hpp"
#include "genesis/sequence/formats/fasta_output_iterator.hpp"
#include "genesis/sequence/functions/labels.hpp"

#include <cassert>
#include <string>
#include <regex>

// =================================================================================================
//      Setup
// =================================================================================================

void setup_cat( CLI::App& app )
{
    // Create the options and subcommand objects.
    auto opt = std::make_shared<CatOptions>();
    auto sub = app.add_subcommand(
        "cat",
        "Concatenate sequence files into one, add prefix based on filename to sequence labels if desired."
    );


    // -----------------------------------------------------------
    //     Input Data
    // -----------------------------------------------------------

    opt->sequence_input.add_sequence_input_opt_to_app( sub );

    // regex to select the per sequence prefix, based on the file name
    sub->add_option(
        "--prefix-regex",
        opt->prefix_regex,
        "Regex to select part of the filename to use as a prefix to the sequence label. "
        "Uses the Modified ECMAScript regular expression grammar: https://en.cppreference.com/w/cpp/regex/ecmascript",
        false
    )->group( "Settings" );

    // sanitize labels?
    sub->add_flag(
        "--sanitize-labels",
        opt->sanitize_labels,
        "Sanitize the seqeunce labels, replaces characters that may cause errors downstream with underscores."
    )->group("Settings");

    // -----------------------------------------------------------
    //     Output Options
    // -----------------------------------------------------------

    opt->output.add_output_dir_opt_to_app( sub );
    opt->output.add_file_prefix_opt_to_app( sub );

    // -----------------------------------------------------------
    //     Callback
    // -----------------------------------------------------------

    // Set the run function as callback to be called when this subcommand is issued.
    // Hand over the options by copy, so that their shared ptr stays alive in the lambda.
    sub->callback( gappa_cli_callback(
        sub,
        {},
        [ opt ]() {
            run_cat( *opt );
        }
    ));
}

// =================================================================================================
//      Helper Functions
// =================================================================================================


// =================================================================================================
//      Run
// =================================================================================================

void run_cat( CatOptions const& options )
{
    using namespace ::genesis;
    using namespace ::genesis::sequence;
    using namespace ::genesis::utils;

    size_t file_count = 0;
    auto const set_size = options.sequence_input.file_count();
    bool const use_prefix = not options.prefix_regex.empty();

    // set up the regex
    std::regex reg;
    if( use_prefix ) {
        reg = std::regex(   options.prefix_regex );
    }

    // Generate output file name.
    auto const ofn
        = options.output.out_dir()
        + options.output.file_prefix()
        + "merged.fasta"
    ;

    LOG_MSG2 << "Outfile: " << ofn;

    FastaOutputIterator fasta_out{ to_file( ofn ) };

    // read in files one by one, using input iterator, output continuously using output iterator
    for( size_t fi = 0; fi < set_size; ++fi ) {
        auto const& file_path = options.sequence_input.file_path( fi );

        LOG_MSG2 << "Processing file " << ( ++file_count ) << " of " << set_size
                 << ": " << file_path;

        // get the prefix of this filename
        std::string label_prefix; 
        if( use_prefix ) {
            auto file_name = file_filename( file_basename( file_path ) );
            std::smatch match;
            if( std::regex_search(  file_name, match, reg ) ) {
                label_prefix = match.str();
                LOG_MSG2 << "  Using label prefix: \"" << label_prefix << "\"";
            } else {
                throw std::invalid_argument{
                    std::string("Regex did not match for filename: ")
                    + file_name
                };
            }
        }

        auto it = FastaInputIterator(
            from_file( file_path ),
            options.sequence_input.fasta_reader()
        );
        while( it ) {
            auto seq = *it;

            if( not label_prefix.empty() ) {
                seq.label( label_prefix + "_" + seq.label() );
            }

            if( options.sanitize_labels ) {
                sanitize_label( seq );
            }

            fasta_out << seq;

            ++it;
        }
    }
}
