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

#include "genesis/utils/core/std.hpp"
#include "genesis/utils/io/input_source.hpp"
#include "genesis/utils/io/input_stream.hpp"
#include "genesis/utils/io/output_stream.hpp"
#include "genesis/utils/text/string.hpp"
#include "genesis/utils/tools/sha256.hpp"

#include <cstdio>
#include <fstream>
#include <regex>
#include <unordered_map>
#include <unordered_set>

// =================================================================================================
//      Setup
// =================================================================================================

void setup_chunkify( CLI::App& app )
{
    // Create the options and subcommand objects.
    auto opt = std::make_shared<ChunkifyOptions>();
    auto sub = app.add_subcommand(
        "chunkify",
        "Chunkify a set of fasta files and create an abundance map."
    );

    // Add common options.
    opt->add_fasta_input_options( sub );
    opt->add_output_dir_options( sub );

    // Fill in custom options.
    auto opt_amf = sub->add_option(
        "--abundance-map-file",
        opt->abundance_map_file,
        "File path for the abundance map",
        true
    );
    opt_amf->group( opt->output_files_group_name() );

    auto opt_cfp = sub->add_option(
        "--chunk-file-prefix",
        opt->chunk_file_prefix,
        "File path prefix for the fasta chunks",
        true
    );
    opt_cfp->group( opt->output_files_group_name() );

    auto opt_cs = sub->add_option(
        "--chunk-size",
        opt->chunk_size,
        "Number of sequences per chunk file",
        true
    );
    opt_cs->group( opt->output_files_group_name() );

    // Set the run function as callback to be called when this subcommand is issued.
    // Hand over the options by copy, so that their shared ptr stays alive in the lambda.
    sub->set_callback( [ opt ]() {
        run_chunkify( *opt );
    });
}

// =================================================================================================
//      Helpers
// =================================================================================================

namespace std
{
    /**
     * @brief Hash function for SHA256 digestes.
     *
     * Basically, we re-hash from 256 bit to 64 bit. This is ugly, but currently faster to implement
     * than a custom container that uses the full hash width. Might work on this in the future.
     */
    template<>
    struct hash<genesis::utils::SHA256::DigestType>
    {
        using argument_type = genesis::utils::SHA256::DigestType;
        using result_type   = std::size_t;

        result_type operator()( argument_type const& s ) const {
            result_type hash = 0;
            hash ^= s[0] ^ ( static_cast<result_type>( s[1] ) << 32 );
            hash ^= s[2] ^ ( static_cast<result_type>( s[3] ) << 32 );
            hash ^= s[4] ^ ( static_cast<result_type>( s[5] ) << 32 );
            hash ^= s[6] ^ ( static_cast<result_type>( s[7] ) << 32 );
            return hash;
        }
    };
}

size_t guess_sequence_abundance( genesis::sequence::Sequence const& seq )
{
    // Prepare static regex (no need to re-compile it on every function call).
    // Matches either ";size=123;" or "_123"
    static const std::string expr = "(?:[;]?size=([0-9]+)[;]?)|(?:_([0-9]+)$)";
    static std::regex pattern( expr );

    // Run the expression.
    std::smatch matches;
    if( std::regex_search( seq.label(), matches, pattern )) {
        size_t res;
        std::string const num = ( matches[1].str().empty() ? matches[2].str() : matches[1].str() );
        sscanf( num.c_str(), "%zu", &res );
        return res;
    } else {
        return 1;
    }
}

// =================================================================================================
//      Run
// =================================================================================================

void run_chunkify( ChunkifyOptions const& options )
{
    using namespace genesis::utils;
    using namespace genesis::sequence;

    // Check if any of the files we are going to produce already exists. If so, fail early.
    options.check_nonexistent_output_files({
        options.abundance_map_file,
        options.chunk_file_prefix + "[0-9]+.fasta"
    });

    // Print some user output.
    options.input_files_print();

    // Prepare abundance output.
    std::ofstream abundance_map_file;
    file_output_stream( options.out_dir() + options.abundance_map_file, abundance_map_file );
    abundance_map_file << "{\n";

    // Sequences hashes, mapping to the chunk number where we store them.
    std::unordered_map< SHA256::DigestType, size_t > hash_to_chunk;

    // Collect sequences for the current chunk here.
    SequenceSet chunk;
    size_t chunk_count = 0;
    size_t seqs_count = 0;

    // Prepare fata writer.
    auto writer = FastaWriter();
    writer.enable_metadata(false);
    writer.line_length( 0 );

    // Helper function to write a chunk.
    auto flush_chunk = [ &writer, &options ]( SequenceSet const& chunk, size_t chunk_count ){
        auto const ofn
            = options.out_dir()
            + options.chunk_file_prefix
            + to_string( chunk_count )
            + ".fasta"
        ;

        writer.to_file( chunk, ofn );
    };

    // Iterate fasta files
    for( size_t fi = 0; fi < options.input_file_count(); ++fi ) {
        auto const& fasta_filename = options.input_file_path( fi );

        if( global_options.verbosity() >= 2 ) {
            std::cout << "Processing file " << ( fi + 1 ) << " of " << options.input_file_count();
            std::cout << ": " << fasta_filename << "\n";
        }

        // Count identical sequences, accesses via their hash.
        std::unordered_map< SHA256::DigestType, size_t > seq_freqs;

        // Iterate sequences
        InputStream instr( make_unique< FileInputSource >( fasta_filename ));
        auto it = FastaInputIterator( instr, options.fasta_reader() );
        while( it ) {

            // Get hash.
            auto const hash_digest = SHA256::from_string_digest( it->sites() );

            // Increment seq counters.
            seq_freqs[ hash_digest ] += guess_sequence_abundance( *it );
            ++seqs_count;

            // We saw that sequence before. Don't need to add it to the chunk.
            if( hash_to_chunk.count( hash_digest ) > 0 ) {
                ++it;
                continue;
            }

            // New sequence: never saw that hash before. Add it to the chunk, store chunk num.
            hash_to_chunk[ hash_digest ] = chunk_count;
            chunk.add( Sequence( SHA256::digest_to_hex( hash_digest ), it->sites() ));

            // If a chunk is full, flush it.
            if( chunk.size() == options.chunk_size ) {
                flush_chunk( chunk, chunk_count );
                ++chunk_count;
                chunk.clear();
            }

            // Next sequence.
            ++it;
        }

        // Write abundance information for this file.
        abundance_map_file << "  \"" << options.input_files_base_file_name( fi ) << "\": {\n";
        for( auto seq_it = seq_freqs.begin(); seq_it != seq_freqs.end(); ++seq_it ) {
            abundance_map_file << "    \"" << SHA256::digest_to_hex( seq_it->first ) << "\": [ ";
            abundance_map_file << hash_to_chunk.at(seq_it->first) << ", " << seq_it->second << " ]";
            if( std::next(seq_it) != seq_freqs.end() ) {
                abundance_map_file << ",";
            }
            abundance_map_file << "\n";
        }
        abundance_map_file << "  }";
        if( fi < options.input_file_count() - 1 ) {
            abundance_map_file << ",";
        }
        abundance_map_file << "\n";
    }

    // Flush the remaining chunk.
    flush_chunk( chunk, chunk_count );

    // Finish abundance output.
    abundance_map_file << "}\n";
    abundance_map_file.close();

    if( global_options.verbosity() >= 1 ) {
        std::cout << "Processed " << seqs_count << " sequences, thereof ";
        std::cout << hash_to_chunk.size() << " unique.\n";
        std::cout << "Produced " << ( chunk_count + 1 ) << " fasta chunk files.";
    }
}
