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

#include "commands/pre/chunkify.hpp"

#include "options/global.hpp"

#include "CLI/CLI.hpp"

#include "genesis/sequence/sequence.hpp"
#include "genesis/sequence/sequence_set.hpp"
#include "genesis/sequence/formats/fasta_input_iterator.hpp"
#include "genesis/sequence/formats/fasta_writer.hpp"
#include "genesis/sequence/functions/labels.hpp"

#include "genesis/utils/core/std.hpp"
#include "genesis/utils/io/input_source.hpp"
#include "genesis/utils/io/input_stream.hpp"
#include "genesis/utils/io/output_stream.hpp"
#include "genesis/utils/text/string.hpp"
#include "genesis/utils/tools/sha256.hpp"

#include <sparsepp/spp.h>

#include <cstdio>
#include <fstream>
#include <unordered_map>
#include <unordered_set>

// =================================================================================================
//      Typedefs
// =================================================================================================

/**
 * @brief Hash to use.
 */
using HashFunction = genesis::utils::SHA256;

/**
 * @brief Data type for storing a hash map from digests to chunk numbers.
 */
// using ChunkHashMap = std::unordered_map< HashFunction::DigestType, size_t >;
using ChunkHashMap = spp::sparse_hash_map< HashFunction::DigestType, size_t >;

/**
 * @brief Data type for storing per input file frequencies.
 */
using AbundancesHashMap = std::unordered_map< HashFunction::DigestType, size_t >;

// =================================================================================================
//      Setup
// =================================================================================================

void setup_chunkify( CLI::App& app )
{
    // Create the options and subcommand objects.
    auto opt = std::make_shared<ChunkifyOptions>();
    auto sub = app.add_subcommand(
        "chunkify",
        "Chunkify a set of fasta files and create abundance maps."
    );

    // -----------------------------------------------------------
    //     Add common options
    // -----------------------------------------------------------

    opt->add_fasta_input_options( sub );
    opt->add_output_dir_options( sub, {
        { "chunks", "Directory to write chunk fasta files to.", "chunks" },
        { "abundances", "Directory to write abundance map files to.", "abundances" }
    });

    // -----------------------------------------------------------
    //     Fill in custom options
    // -----------------------------------------------------------

    // Abundance File Prefix
    auto opt_afp = sub->add_option(
        "--abundance-file-prefix",
        opt->abundance_file_prefix,
        "File path prefix for the abundance maps.",
        true
    );
    opt_afp->group( opt->output_files_group_name() );

    // Chunk File Prefix
    auto opt_cfp = sub->add_option(
        "--chunk-file-prefix",
        opt->chunk_file_prefix,
        "File path prefix for the fasta chunks.",
        true
    );
    opt_cfp->group( opt->output_files_group_name() );

    // Chunk Size
    auto opt_cs = sub->add_option(
        "--chunk-size",
        opt->chunk_size,
        "Number of sequences per chunk file.",
        true
    );
    opt_cs->group( opt->output_files_group_name() );

    // Minimum Abundance
    auto opt_ma = sub->add_option(
        "--min-abundance",
        opt->min_abundance,
        "Minimum abundance of a sequence per file. Sequences below are filtered out.",
        true
    );
    opt_ma->group( opt->output_files_group_name() );

    // -----------------------------------------------------------
    //     Callback
    // -----------------------------------------------------------

    // Set the run function as callback to be called when this subcommand is issued.
    // Hand over the options by copy, so that their shared ptr stays alive in the lambda.
    sub->set_callback( [ opt ]() {
        run_chunkify( *opt );
    });
}

// =================================================================================================
//      Helpers
// =================================================================================================

void write_chunk_file(
    ChunkifyOptions const& options,
    genesis::sequence::SequenceSet const& chunk,
    size_t chunk_count
) {
    using namespace genesis::sequence;

    // Prepare fata writer.
    auto writer = FastaWriter();
    writer.enable_metadata(false);
    writer.line_length( 0 );

    // Generate output file name.
    auto const ofn
        = options.out_dir( "chunks" )
        + options.chunk_file_prefix
        + std::to_string( chunk_count )
        + ".fasta"
    ;

    // Write
    writer.to_file( chunk, ofn );
}

void write_abundance_map_file(
    ChunkifyOptions const& options,
    ChunkHashMap const& hash_to_chunk,
    AbundancesHashMap const& seq_abundances,
    size_t input_file_counter
) {
    using namespace genesis::utils;

    // Base name of the current input file
    auto const base_fn = options.input_files_base_file_name( input_file_counter );

    // Pprepare a new abundance file
    auto const fn
        = options.out_dir( "abundances" )
        + options.abundance_file_prefix
        + base_fn
        + ".json"
    ;
    std::ofstream ofs;
    file_output_stream( fn, ofs );
    ofs << "{\n";

    // Write name of the input file for later identification.
    ofs << "  \"" << base_fn << "\": {";

    // TODO write meta data: gappa version, invocation, which hash function, which fields are written

    // Write abundance information for this file.
    bool is_first = true;
    for( auto seq_it = seq_abundances.begin(); seq_it != seq_abundances.end(); ++seq_it ) {

        // Print comma for all but the first entry.
        if( ! is_first ) {
            ofs << ",";
        }
        is_first = false;
        ofs << "\n";

        // Print sequence data.
        ofs << "    \"" << HashFunction::digest_to_hex( seq_it->first ) << "\": [ ";
        ofs << hash_to_chunk.at(seq_it->first) << ", " << seq_it->second << " ]";
    }

    // Finish the file.
    ofs << "\n  }\n";
    ofs << "}\n";
    ofs.close();
}

// =================================================================================================
//      Run
// =================================================================================================

void run_chunkify( ChunkifyOptions const& options )
{
    using namespace genesis::utils;
    using namespace genesis::sequence;

    // -----------------------------------------------------------
    //     Input File Preparations
    // -----------------------------------------------------------

    // Check if any of the files we are going to produce already exists. If so, fail early.
    options.check_nonexistent_output_files(
        { options.abundance_file_prefix + ".*\\.json" },
        "abundances"
    );
    options.check_nonexistent_output_files(
        { options.chunk_file_prefix + "[0-9]+\\.fasta" },
        "chunks"
    );

    // Print some user output.
    options.input_files_print();

    // -----------------------------------------------------------
    //     Iterate Input Files
    // -----------------------------------------------------------

    // Sequences hashes, mapping to the chunk number where they are stored,
    // i.e. where they first occured.
    ChunkHashMap hash_to_chunk;

    // Collect sequences for the current chunk here.
    SequenceSet current_chunk;
    size_t chunk_count = 0;
    size_t total_seqs_count = 0;
    size_t min_abun_count = 0;

    // Iterate fasta files
    for( size_t fi = 0; fi < options.input_file_count(); ++fi ) {
        auto const& fasta_filename = options.input_file_path( fi );

        // User output
        if( global_options.verbosity() >= 2 ) {
            std::cout << "Processing file " << ( fi + 1 ) << " of " << options.input_file_count();
            std::cout << ": " << fasta_filename << "\n";
        }

        // Count identical sequences of this fasta file, accesses via their hash.
        AbundancesHashMap seq_abundances;

        // Iterate sequences
        InputStream instr( make_unique< FileInputSource >( fasta_filename ));
        for( auto it = FastaInputIterator( instr, options.fasta_reader() ); it; ++it ) {
            ++total_seqs_count;

            // Check for min abundance.
            auto const abundance = guess_sequence_abundance( *it );
            if( abundance < options.min_abundance ) {
                continue;
            }
            ++min_abun_count;

            // Increment seq abundance for this file.
            auto const hash_digest = HashFunction::from_string_digest( it->sites() );
            seq_abundances[ hash_digest ] += abundance;

            // We saw that sequence before. Don't need to add it to the chunk.
            if( hash_to_chunk.count( hash_digest ) > 0 ) {
                continue;
            }

            // New sequence: never saw that hash before. Add it to the chunk, store chunk num.
            current_chunk.add( Sequence( HashFunction::digest_to_hex( hash_digest ), it->sites() ));
            hash_to_chunk[ hash_digest ] = chunk_count;

            // If a chunk is full, flush it.
            if( current_chunk.size() >= options.chunk_size ) {
                write_chunk_file( options, current_chunk, chunk_count );
                ++chunk_count;
                current_chunk.clear();
            }
        }

        // Finished a fasta file. Write its abundances.
        write_abundance_map_file( options, hash_to_chunk, seq_abundances, fi );
    }

    // -----------------------------------------------------------
    //     Finish
    // -----------------------------------------------------------

    // Write the remaining chunk.
    write_chunk_file( options, current_chunk, chunk_count );

    if( global_options.verbosity() >= 1 ) {
        std::cout << "Processed " << total_seqs_count << " sequences, thereof ";
        std::cout << (total_seqs_count - min_abun_count) << " (";
        std::cout << ( 100 * (total_seqs_count - min_abun_count) / total_seqs_count );
        std::cout << "%) filtered due to low abundance.\n";
        std::cout << "Wrote " << hash_to_chunk.size() << " unique sequences ";
        std::cout << "in " << ( chunk_count + 1 ) << " fasta chunk files.\n";
    }
}
