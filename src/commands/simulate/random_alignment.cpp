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

#include "commands/simulate/random_alignment.hpp"

#include "options/global.hpp"
#include "tools/cli_setup.hpp"
#include "tools/misc.hpp"

#include "CLI/CLI.hpp"

#include "genesis/utils/io/output_stream.hpp"

#include <cassert>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

// =================================================================================================
//      Setup
// =================================================================================================

void setup_random_alignment( CLI::App& app )
{
    // Create the options and subcommand objects.
    auto opt = std::make_shared<RandomAlignmentOptions>();
    auto sub = app.add_subcommand(
        "random-alignment",
        "Create a random alignment with a given numer of sequences of a given length."
    );

    // -----------------------------------------------------------
    //     Input Data
    // -----------------------------------------------------------

    // Sequence count
    auto num_sequences_opt = sub->add_option(
        "--sequence-count",
        opt->num_sequences,
        "Number of sequences to create."
    );
    num_sequences_opt->group( "Input" );
    num_sequences_opt->required();

    // Sequence length
    auto len_sequences_opt = sub->add_option(
        "--sequence-length",
        opt->len_sequences,
        "Length of the sequences to create."
    );
    len_sequences_opt->group( "Input" );
    len_sequences_opt->required();

    // Character set
    auto characters_opt = sub->add_option(
        "--characters",
        opt->characters,
        "Set of characters to use for the sequences.",
        true
    );
    characters_opt->group( "Input" );
    // characters_opt->required();

    // -----------------------------------------------------------
    //     Output Options
    // -----------------------------------------------------------

    opt->file_output.add_default_output_opts_to_app( sub );
    opt->file_output.add_file_compress_opt_to_app( sub );

    // File type fasta.
    auto write_fasta_opt = sub->add_flag(
        "--write-fasta",
        opt->write_fasta,
        "Write sequences to a fasta file."
    );
    write_fasta_opt->group( "Output" );

    // File type phylip.
    auto write_strict_phylip_opt = sub->add_flag(
        "--write-strict-phylip",
        opt->write_strict_phylip,
        "Write sequences to a strict phylip file."
    );
    write_strict_phylip_opt->group( "Output" );
    auto write_relaxed_phylip_opt = sub->add_flag(
        "--write-relaxed-phylip",
        opt->write_relaxed_phylip,
        "Write sequences to a relaxed phylip file."
    );
    write_relaxed_phylip_opt->group( "Output" );
    write_relaxed_phylip_opt->excludes( write_strict_phylip_opt );
    write_strict_phylip_opt->excludes( write_relaxed_phylip_opt );

    // -----------------------------------------------------------
    //     Callback
    // -----------------------------------------------------------

    // Set the run function as callback to be called when this subcommand is issued.
    // Hand over the options by copy, so that their shared ptr stays alive in the lambda.
    sub->callback( gappa_cli_callback(
        sub,
        {},
        [ opt ]() {
            run_random_alignment( *opt );
        }
    ));
}

// =================================================================================================
//      Run
// =================================================================================================

void run_random_alignment( RandomAlignmentOptions const& options )
{
    using namespace ::genesis;

    // Init randomness.
    std::srand( std::time( nullptr ));

    // Check that at least one of the options is set.
    if( ! options.write_fasta && ! options.write_strict_phylip && ! options.write_relaxed_phylip ) {
        throw CLI::ValidationError(
            "--write-fasta, --write-strict-phylip, --write-relaxed-phylip",
            "At least one output format has to be specified."
        );
    }
    if( options.num_sequences == 0 ) {
        throw CLI::ValidationError(
            "--sequence-count",
            "Sequence count has to be greater than zero."
        );
    }
    if( options.len_sequences == 0 ) {
        throw CLI::ValidationError(
            "--sequence-length",
            "Sequence length has to be greater than zero."
        );
    }

    // Open streams as needed. This fails if the files already exist.
    std::shared_ptr<genesis::utils::BaseOutputTarget> fasta_os;
    if( options.write_fasta ) {
        fasta_os = options.file_output.get_output_target( "random-alignment", "fasta" );
    }
    std::shared_ptr<genesis::utils::BaseOutputTarget> phylip_os;
    if( options.write_strict_phylip || options.write_relaxed_phylip ) {
        phylip_os = options.file_output.get_output_target( "random-alignment", "phylip" );

        // Write phylip header.
        (*phylip_os) << options.num_sequences << " " << options.len_sequences << "\n";
    }

    // Write sequences. We do not use our sequence classes of the fasta or phylip writers,
    // as the file formats are simple and we do not want to store all those sequences in memory.
    for( size_t s = 0; s < options.num_sequences; ++s ) {

        // Write sequence name header.
        auto const name = random_indexed_name( s, options.num_sequences );
        if( options.write_fasta ) {
            (*fasta_os) << ">" << name << "\n";
        }
        if( options.write_strict_phylip ) {
            if( name.size() > 10 ) {
                // Should never happen. This would be equlivalent to 26^10 sequences.
                // Still, let's check.
                throw std::runtime_error(
                    "Cannot handle this many sequences in strict phylip format."
                );
            }
            (*phylip_os) << name << std::string( 10 - name.size(), ' ' );
        }
        if( options.write_relaxed_phylip ) {
            (*phylip_os) << name << " ";
        }

        // Write content.
        for( size_t l = 0; l < options.len_sequences; ++l ) {

            // Write new line ever so often, except for the stupid strict phylip format.
            if( l > 0 && l % 80 == 0 ) {
                if( options.write_fasta ) {
                    (*fasta_os) << "\n";
                }
                if( options.write_relaxed_phylip ) {
                    (*phylip_os) << "\n";
                }
            }

            // Get a random char from the set and write it.
            auto const c = options.characters[ std::rand() % options.characters.size() ];
            if( options.write_fasta ) {
                (*fasta_os) << c;
            }
            if( options.write_strict_phylip || options.write_relaxed_phylip ) {
                (*phylip_os) << c;
            }
        }

        // Write final new line.
        if( options.write_fasta ) {
            (*fasta_os) << "\n";
        }
        if( options.write_strict_phylip || options.write_relaxed_phylip ) {
            (*phylip_os) << "\n";
        }
    }
}
