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

#include "commands/pre/unchunkify.hpp"

#include "options/global.hpp"

#include "CLI/CLI.hpp"

#include "genesis/placement/formats/jplace_reader.hpp"
#include "genesis/placement/formats/jplace_writer.hpp"
#include "genesis/placement/sample.hpp"
#include "genesis/utils/containers/mru_cache.hpp"
#include "genesis/utils/formats/json/document.hpp"
#include "genesis/utils/formats/json/iterator.hpp"
#include "genesis/utils/formats/json/reader.hpp"

#ifdef GENESIS_OPENMP
#   include <omp.h>
#endif

#include <algorithm>
#include <cassert>
#include <memory>

// =================================================================================================
//      Typedefs
// =================================================================================================

/**
 * @brief Store a sample, along with a map from sequence hash names to the pquery index in the sample.
 *
 * Not all modes of the command use the map, it can thus be empty if not needed.
 */
struct MappedSample
{
    genesis::placement::Sample sample;
    std::unordered_map<std::string, size_t> hash_to_index;
};

/**
 * @brief Cache for chunk jplace files.
 */
using ChunkCache = genesis::utils::MruCache<size_t, std::shared_ptr<MappedSample>>;

/**
 * @brief Store a sample index and a pquery index that tells where a particular hash can be found.
 */
struct SamplePqueryIndices
{
    size_t sample_index;
    size_t pquery_index;
};

using HashToIndexMap = std::unordered_map<std::string, SamplePqueryIndices>;

// =================================================================================================
//      Setup
// =================================================================================================

void setup_unchunkify( CLI::App& app )
{
    // Create the options and subcommand objects.
    auto opt = std::make_shared<UnchunkifyOptions>();
    auto sub = app.add_subcommand(
        "unchunkify",
        "Unchunkify a set of jplace files using abundace map files and create per-sample jplace files."
    );

    // -----------------------------------------------------------
    //     Add common options
    // -----------------------------------------------------------

    opt->jplace_input.add_jplace_input_opt_to_app( sub, false );
    opt->abundance_map_input.add_multi_file_input_opt_to_app( sub, "abundances", "json" );
    opt->file_output.add_output_dir_opt_to_app( sub );

    // -----------------------------------------------------------
    //     Fill in custom options
    // -----------------------------------------------------------

    // Chunk List file.
    auto chunk_list_file_opt = sub->add_option(
        "--chunk-list-file",
        opt->chunk_list_file,
        "If provided, needs to contain a list of chunk file paths in the numerical order that was "
        "produced by the chunkify command."
    );

    // Chunk List file.
    auto chunk_file_expression_opt = sub->add_option(
        "--chunk-file-expression",
        opt->chunk_file_expression,
        "If provided, ..."
    );

    // Cache size
    sub->add_option(
        "--jplace-cache-size",
        opt->jplace_cache_size,
        "Cache size to determine how many jplace files are kept in memory. Default (0) means all. "
        "Use this if the command runs out of memory. It however comes at the cost of longer runtime. "
        "In order to check how large the cache size can be, you can run the command with -vv, "
        "which will report the used cache size until it crashes. Then, set the cache size to "
        "something below that.",
        true
    );

    // Make the three input modes mutually exclusive.
    chunk_list_file_opt->excludes( opt->jplace_input.option() );
    chunk_list_file_opt->excludes( chunk_file_expression_opt );
    chunk_file_expression_opt->excludes( opt->jplace_input.option() );
    chunk_file_expression_opt->excludes( chunk_list_file_opt );
    opt->jplace_input.option()->excludes( chunk_list_file_opt );
    opt->jplace_input.option()->excludes( chunk_file_expression_opt );

    // -----------------------------------------------------------
    //     Callback
    // -----------------------------------------------------------

    // Set the run function as callback to be called when this subcommand is issued.
    // Hand over the options by copy, so that their shared ptr stays alive in the lambda.
    sub->set_callback( [ opt ]() {
        run_unchunkify( *opt );
    });
}

// =================================================================================================
//      Helpers
// =================================================================================================

enum class UnchunkifyMode
{
    kNone,
    kChunkListFile,
    kChunkFileExpression,
    kJplaceInput
};

/**
 * @brief Check which of the three modes was selected by the user, and return it.
 */
UnchunkifyMode get_unchunkify_mode( UnchunkifyOptions const& options )
{
    UnchunkifyMode mode;
    size_t mode_cnt = 0;

    if( *( options.jplace_input.option() ) && options.jplace_input.file_count() > 0 ) {
        mode = UnchunkifyMode::kJplaceInput;
        ++mode_cnt;

        if( global_options.verbosity() >= 1 ) {
            std::cout << "Selected mode: Jplace Input.\n";
        }
    }
    if( ! options.chunk_list_file.empty() ) {
        mode = UnchunkifyMode::kChunkListFile;
        ++mode_cnt;

        if( global_options.verbosity() >= 1 ) {
            std::cout << "Selected mode: Chunk List File.\n";
        }
    }
    if( ! options.chunk_file_expression.empty() ) {
        mode = UnchunkifyMode::kChunkFileExpression;
        ++mode_cnt;

        if( global_options.verbosity() >= 1 ) {
            std::cout << "Selected mode: Chunk File Expression.\n";
        }
    }
    if( mode_cnt != 1 ) {
        throw CLI::ValidationError(
            "--jplace-path, --chunk-list-file, --chunk-file-expression",
            "Exactly one of the three input modes has to be provided."
        );
    }

    return mode;
}

/**
 * @brief If Jplace Files mode was selected, build the hash map. If not, return an empty map.
 */
HashToIndexMap get_hash_to_indices_map(
    UnchunkifyOptions const& options,
    ChunkCache&              chunk_cache,
    UnchunkifyMode           mode
) {
    HashToIndexMap hash_map;

    // If we are not in that mode, just return empty.
    if( mode != UnchunkifyMode::kJplaceInput ) {
        return hash_map;
    }

    // Print user output.
    if( global_options.verbosity() >= 2 ) {
        std::cout << "Preparing chunk hash list.\n";
    }

    // Load all (!) chunk files once (possibly removing the earlier ones from the cache while
    // doing so), and for each pquery, store its names and indices for later lookup.
    // As the actual storage is critical, this loop does not scale well.
    // But at least, the file loading is done in parallel... Could optimize if needed.
    #pragma omp parallel for schedule(dynamic)
    for( size_t sample_idx = 0; sample_idx < options.jplace_input.file_count(); ++sample_idx ) {
        auto const chunk = chunk_cache.fetch_copy( sample_idx );

        for( size_t pquery_idx = 0; pquery_idx < chunk->sample.size(); ++pquery_idx ) {
            auto const& pquery = chunk->sample.at( pquery_idx );

            for( auto const& name : pquery.names() ) {
                if( hash_map.count( name.name ) > 0 ) {
                    auto const file1 = options.jplace_input.file_path( hash_map[ name.name ].sample_index );
                    auto const file2 = options.jplace_input.file_path( sample_idx );
                    throw std::runtime_error(
                        "Pquery with hash name '" + name.name + "' exists in multiple files: " +
                        file1 + " and " + file2
                    );
                }

                #pragma omp critical(GAPPA_UNCHUNKIFY_FILL_HASH_INDICES_MAP)
                {
                    hash_map[ name.name ] = { sample_idx, pquery_idx };
                }
            }
        }
    }

    // Print user output.
    if( global_options.verbosity() >= 2 ) {
        std::cout << "Prepared chunk hash list.\n";
    }

    return hash_map;
}

// =================================================================================================
//      Run
// =================================================================================================

void run_unchunkify( UnchunkifyOptions const& options )
{
    using namespace genesis::placement;
    using namespace genesis::utils;

    // -----------------------------------------------------------
    //     Options Check
    // -----------------------------------------------------------

    // Do this first, as it validates that provided options, so we fail early.
    auto const mode = get_unchunkify_mode( options );

    // -----------------------------------------------------------
    //     Input Output File Preparations
    // -----------------------------------------------------------

    // Check if any of the files we are going to produce already exists. If so, fail early.
    options.file_output.check_nonexistent_output_files({ ".*\\.jplace" });

    // Print some user output.
    options.jplace_input.print_files();
    options.abundance_map_input.print_files();

    auto jplace_writer = JplaceWriter();

    // -----------------------------------------------------------
    //     Prepare Helper Data
    // -----------------------------------------------------------

    // Depending on the mode, we need different helper data structures. Prepare them.

    // Make a cache for storing the jplace chunk files.
    // We load a file given its index in the file list. This makes it flexible for the different
    // modes to decide how they turn their input into a sample index.
    ChunkCache chunk_cache( options.jplace_cache_size );
    chunk_cache.load_function = [&]( size_t const index ){

        // Report cache size on every load, that is, whenever the cache actually changes.
        if( global_options.verbosity() >= 3 ) {
            #pragma omp critical(GAPPA_UNCHUNKIFY_PRINT)
            {
                std::cout << "Current jplace cache size: " << chunk_cache.size() << "\n";
            }
        }

        // Create the result and load the sample.
        auto mapped_sample = MappedSample();
        mapped_sample.sample = options.jplace_input.sample( index );

        // If we are in a mode that needs per-sample indicies, create the map from hashes to indices.
        if( mode == UnchunkifyMode::kChunkFileExpression || mode == UnchunkifyMode::kChunkListFile ) {
            for( size_t pquery_idx = 0; pquery_idx < mapped_sample.sample.size(); ++pquery_idx ) {
                auto const& pquery = mapped_sample.sample.at( pquery_idx );

                for( auto const& name : pquery.names() ) {
                    if( mapped_sample.hash_to_index.count( name.name ) > 0 ) {
                        auto const file = options.jplace_input.file_path( index );
                        throw std::runtime_error(
                            "Pquery with hash name '" + name.name +
                            "' exists in multiple times in file: " + file
                        );
                    }

                    mapped_sample.hash_to_index[ name.name ] = pquery_idx;
                }
            }
        }

        return std::make_shared<MappedSample>( mapped_sample );
    };

    // Mode Jplace Input. We don't have any chunk nums, so instead we just prepare a full
    // lookup from hashes to sample index. This can get big...
    // It is only filled if the mode is actually jplace input.
    auto const hash_to_indices = get_hash_to_indices_map( options, chunk_cache, mode );

    // -----------------------------------------------------------
    //     Run
    // -----------------------------------------------------------

    // Some statistics for user output.
    size_t file_count = 0;
    size_t seq_count = 0;
    size_t not_found_count = 0;

    // Iterate map files
    #pragma omp parallel for schedule(dynamic)
    for( size_t fi = 0; fi < options.abundance_map_input.file_count(); ++fi ) {
        auto const& map_filename = options.abundance_map_input.file_path( fi );

        // User output
        if( global_options.verbosity() >= 2 ) {
            #pragma omp critical(GAPPA_UNCHUNKIFY_PRINT)
            {
                ++file_count;
                std::cout << "Processing file " << file_count << " of " << options.abundance_map_input.file_count();
                std::cout << ": " << map_filename << "\n";

                // if( global_options.verbosity() >= 3 ) {
                //     std::cout << "Current jplace cache size: " << chunk_cache.size() << "\n";
                // }
            }
        }

        // Read map file.
        auto doc = JsonReader().from_file( map_filename );
        if( ! doc.is_object() ) {
            throw std::runtime_error( "Invalid abundance map: " + map_filename );
        }
        auto abun_it = doc.find( "abundances" );
        if( abun_it == doc.end() || ! abun_it->is_array() ) {
            throw std::runtime_error( "Invalid abundance map: " + map_filename );
        }

        // Sort mapped sequences by chunk id, in order to minimize loading.
        auto sort_by_chunk_id = [&]( JsonDocument const& lhs, JsonDocument const& rhs )
        {
            // Caution.
            if(
                ! lhs.is_array() || lhs.size() != 3 || ! lhs[1].is_number_unsigned() ||
                ! rhs.is_array() || rhs.size() != 3 || ! rhs[1].is_number_unsigned()
            ) {
                throw std::runtime_error( "Invalid abundance map: " + map_filename );
            }

            return lhs.get_array()[1].get_number_unsigned() < rhs.get_array()[1].get_number_unsigned();
        };
        std::sort( abun_it->get_array().begin(), abun_it->get_array().end(), sort_by_chunk_id );

        // Create empty sample.
        Sample sample;

        // Loop over mapped sequences and add them to the sample.
        for( auto seq_entry_it = abun_it->begin(); seq_entry_it != abun_it->end(); ++seq_entry_it ) {
            if(
                ! seq_entry_it->is_array() ||
                seq_entry_it->size() != 3  ||
                ! (*seq_entry_it)[0].is_string() ||
                ! (*seq_entry_it)[1].is_number_unsigned() ||
                ! (*seq_entry_it)[2].is_object()
            ) {
                throw std::runtime_error( "Invalid abundance map: " + map_filename );
            }
            ++seq_count;

            auto const& seq_hash = (*seq_entry_it)[0].get_string();

            // Get sample index depending on mode.
            size_t sample_idx;
            size_t pquery_idx;
            if( mode == UnchunkifyMode::kJplaceInput ) {
                if( hash_to_indices.count( seq_hash ) == 0 ) {
                    ++not_found_count;
                    continue;
                }
                auto const& indices = hash_to_indices.at( seq_hash );
                sample_idx = indices.sample_index;
                pquery_idx = indices.pquery_index;
            } else {
                assert( mode == UnchunkifyMode::kChunkFileExpression || mode == UnchunkifyMode::kChunkListFile );

                // TODO
                // auto const chunk_num = (*seq_entry_it)[1].get_number_unsigned();
                sample_idx = 0;
            }

            // Load the chunk.
            auto const chunk = chunk_cache.fetch_copy( sample_idx );

            // For two modes, we need to get the pquery index from the sample.
            if( mode == UnchunkifyMode::kChunkFileExpression || mode == UnchunkifyMode::kChunkListFile ) {

                if( chunk->hash_to_index.count( seq_hash ) == 0 ) {
                    ++not_found_count;
                    continue;
                }
                pquery_idx = chunk->hash_to_index.at( seq_hash );
            }

            // New sample: give it a tree!
            if( sample.empty() ) {
                sample = Sample( chunk->sample.tree() );
            }

            // Fill in the sequence.
            auto& pquery = sample.add( chunk->sample.at( pquery_idx ));

            // Remove the hash name, and add the actual sequence names and abundances.
            pquery.clear_names();
            auto const& mult_arr = (*seq_entry_it)[2].get_object();
            for( auto const& mult_obj : mult_arr ) {
                auto const& label = mult_obj.first;
                if( ! mult_obj.second.is_number_unsigned() ) {
                    throw std::runtime_error( "Invalid abundance map: " + map_filename );
                }
                auto const mult = mult_obj.second.get_number_unsigned();

                pquery.add_name( label, mult );
            }

            // std::cout << "seq_hash " << seq_hash << "\t";
            // std::cout << "sample_idx " << sample_idx << "\t";
            // std::cout << "chunk_num " << chunk_num << "\t";
            // std::cout << "pquery_idx " << pquery_idx << "\t";
            // std::cout << "mult_arr.size() " << mult_arr.size() << "\n";
        }

        // Get sample name.
        auto sample_name_it = doc.find( "sample" );
        if( sample_name_it == doc.end() || ! sample_name_it->is_string() ) {
            throw std::runtime_error( "Invalid abundance map: " + map_filename );
        }
        auto const& sample_name = sample_name_it->get_string();

        // We are done with the map/sample. Write it.
        jplace_writer.to_file( sample, options.file_output.out_dir() + sample_name + ".jplace" );
    }

    if( global_options.verbosity() >= 1 ) {
        std::cout << "Processed " << seq_count << " unique sequences in the chunks.\n";
        std::cout << "Could not find " << not_found_count << " sequence hashes.\n";
    }
}
