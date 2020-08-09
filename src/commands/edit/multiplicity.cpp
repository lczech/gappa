/*
    gappa - Genesis Applications for Phylogenetic Placement Analysis
    Copyright (C) 2017-2020 Lucas Czech and HITS gGmbH

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

#include "commands/edit/multiplicity.hpp"

#include "options/global.hpp"
#include "tools/cli_setup.hpp"

#include "CLI/CLI.hpp"

#include "genesis/placement/formats/jplace_writer.hpp"
#include "genesis/sequence/formats/fasta_input_iterator.hpp"
#include "genesis/sequence/formats/fasta_reader.hpp"
#include "genesis/sequence/functions/labels.hpp"
#include "genesis/utils/formats/csv/reader.hpp"
#include "genesis/utils/io/output_stream.hpp"
#include "genesis/utils/io/output_target.hpp"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <map>
#include <unordered_map>
#include <utility>

#ifdef GENESIS_OPENMP
#   include <omp.h>
#endif

// =================================================================================================
//      Typedefs
// =================================================================================================

/**
 * @brief Map from jplace base file name to a list of multiplicites per pquery name.
 *
 * A special case is the empty key (no jplace file name), which is used if the user simply
 * provided a list of abundances per pquery name, with no specified sample name.
 */
using MultiplicityMap = std::unordered_map<std::string, std::unordered_map<std::string, double>>;

/**
 * @brief Same as the MultiplicityMap, but keeps its keys sorted.
 */
using SortedMultiplicityMap = std::map<std::string, std::map<std::string, double>>;

// =================================================================================================
//      Setup
// =================================================================================================

void setup_multiplicity( CLI::App& app )
{
    // Create the options and subcommand objects.
    auto options = std::make_shared<MultiplicityOptions>();
    auto sub = app.add_subcommand(
        "multiplicity",
        "Edit the multiplicities of queries in jplace files."
    );

    // -----------------------------------------------------------
    //     Input options
    // -----------------------------------------------------------

    // Jplace input
    options->jplace_input.add_jplace_input_opt_to_app( sub );

    // Multiplicity list file
    auto multiplicity_file_opt = sub->add_option(
        "--multiplicity-file",
        options->multiplicity_file,
        "File containing a tab-separated list of [sample name,] query name, and multiplicity."
    );
    multiplicity_file_opt->check( CLI::ExistingFile );
    multiplicity_file_opt->group( "Input" );

    // Alternatively, fasta file(s) with info in the headers.
    auto fasta_file_opt = options->sequence_input.add_fasta_input_opt_to_app( sub, false );

    // Make two modes for multiplicities mutually exclusive.
    multiplicity_file_opt->excludes( fasta_file_opt );
    fasta_file_opt->excludes( multiplicity_file_opt );

    auto keep_full_label_opt = sub->add_flag(
        "--keep-full-label",
        options->keep_full_label,
        "If fasta files are used, keep their whole label as the name for jplace pqueries, "
        "instead of removing the abundance annotation."
    );
    keep_full_label_opt->group( "Input" );
    keep_full_label_opt->needs( fasta_file_opt );

    // -----------------------------------------------------------
    //     Output options
    // -----------------------------------------------------------

    auto write_multiplicity_file_opt = sub->add_flag(
        "--write-multiplicity-file",
        options->write_multiplicity_file,
        "Do not change the existing multiplicities, but instead produce a file that lists them. "
    );
    write_multiplicity_file_opt->group( "Output" );
    write_multiplicity_file_opt->excludes( fasta_file_opt );
    write_multiplicity_file_opt->excludes( multiplicity_file_opt );

    options->file_output.add_output_dir_opt_to_app( sub );
    options->file_output.add_file_prefix_opt_to_app( sub );

    // -----------------------------------------------------------
    //     Callback
    // -----------------------------------------------------------

    // Set the run function as callback to be called when this subcommand is issued.
    // Hand over the options by copy, so that their shared ptr stays alive in the lambda.
    sub->callback( gappa_cli_callback(
        sub,
        {},
        [ options ]() {
            run_multiplicity( *options );
        }
    ));
}

// =================================================================================================
//      Multiplicity File Reading
// =================================================================================================

std::pair<MultiplicityMap, std::vector<std::string>> get_multiplicities_csv_file(
    MultiplicityOptions const& options
) {
    using namespace genesis;

    // Already checked upon callind this function. Assert again here.
    assert( ! options.multiplicity_file.empty() );
    assert( options.sequence_input.file_count() == 0 );

    // Store the results.
    MultiplicityMap result;
    std::vector<std::string> duplicates;

    // TODO allow to customize the separator char here (and later in the writing as well)!

    // Read the file line by line, that is, pquery by pquery.
    size_t line_size = 0;
    auto reader = utils::CsvReader();
    reader.separator_chars( "\t" );
    auto const table = reader.read( utils::from_file( options.multiplicity_file ));
    for( auto const& line : table ) {

        // Some consistency check. We do not allow to mix line sizes in one file.
        if( line_size == 0 ) {
            line_size = line.size();
        }

        // Get the parts of the line.
        std::string sample;
        std::string pquery;
        std::string multip;
        if( line.size() == 2 && line_size == 2 ) {
            pquery = line[0];
            multip = line[1];
        } else if( line.size() == 3 && line_size == 3 ) {
            sample = line[0];
            pquery = line[1];
            multip = line[2];
        } else {
            throw CLI::ValidationError(
                "--multiplicity-file (" + options.multiplicity_file +  ")",
                "Invalid multiplicity file. Needs to be a tab-separated list of fields, "
                "with either two or three fields per line: An optional jplace sample name, "
                "a pquery name, and its multiplicity."
            );
        }

        // Check if it is a duplicate.
        if( result[ sample ].count( pquery ) > 0 ) {
            duplicates.push_back(
                sample + ( sample.empty() ? "" : " " ) + pquery
            );
        }

        // Convert the multiplicity.
        double value;
        try{
            value = std::stod( multip );
        } catch( ... ) {
            throw CLI::ValidationError(
                "--multiplicity-file (" + options.multiplicity_file +  ")",
                "Invalid multiplicity entry, cannot parse as a number: '" + multip + "'."
            );
        }

        // Store in result.
        result[ sample ][ pquery ] = value;
    }

    return { result, duplicates };
}

std::pair<MultiplicityMap, std::vector<std::string>> get_multiplicities_fasta_files(
    MultiplicityOptions const& options
) {
    using namespace genesis;

    // Already checked upon calling this function. Assert again here.
    assert( options.multiplicity_file.empty() );
    assert( options.sequence_input.file_count() > 0 );

    // Store the results.
    MultiplicityMap result;
    std::vector<std::string> duplicates;

    // Read fasta files in parallel.
    #pragma omp parallel for schedule(dynamic)
    for( size_t file_idx = 0; file_idx < options.sequence_input.file_count(); ++file_idx ) {
        auto const file_path = options.sequence_input.file_path( file_idx );
        auto const sample = options.sequence_input.base_file_name( file_idx );

        // Iterate the file and read all sequence labels.
        auto seq_it = sequence::FastaInputIterator( utils::from_file( file_path ));
        while( seq_it ) {
            std::string pquery;
            double      value;

            // First try to simply use the abundance.
            // This accepts both formats "size=123" and "_123".
            auto const& label = seq_it->label();
            auto const abun = sequence::guess_sequence_abundance( label );
            pquery = abun.first;
            value = abun.second;

            // Also try the more elaborate way of using attributes directly.
            // A bit redundant, but okay for now. Those values might overwrite the previous ones.
            // The try clause catches all kinds of erros: label_attributes might throw,
            // or any of the string to number conversions.
            // In each such case, we simply reset to what we had before.
            try{
                auto const la = sequence::label_attributes( label );
                if( ! la.attributes.empty() ) {
                    pquery = la.label;
                    double mult = 1.0;
                    if( la.attributes.count("size") > 0 ) {
                        mult *= std::stod( la.attributes.at( "size" ));
                    }
                    if( la.attributes.count("weight") > 0 ) {
                        mult *= std::stod( la.attributes.at( "weight" ));
                    }
                    value = mult;
                }
            } catch(...) {
                // Reset to previous values.
                pquery = abun.first;
                value = abun.second;
            }

            // If we keep the full label, reset it.
            if( options.keep_full_label ) {
                pquery = label;
            }

            // Set the value in the result.
            #pragma omp critical(GAPPA_MULTIPLICITY_ADD_MULTIPLICITY)
            {
                // Check if it is a duplicate.
                if( result[ sample ].count( pquery ) > 0 ) {
                    duplicates.push_back(
                        sample + ( sample.empty() ? "" : " " ) + pquery
                    );
                }

                // Store in result.
                result[ sample ][ pquery ] = value;
            }

            // Next sequence;
            ++seq_it;
        }
    }

    return { result, duplicates };
}

MultiplicityMap get_multiplicities( MultiplicityOptions const& options )
{
    // User output.
    LOG_MSG1 << "Reading multiplicities.";

    // Store the results.
    std::pair<MultiplicityMap, std::vector<std::string>> result;

    // If the user specified a csv file, read it.
    if( ! options.multiplicity_file.empty() ) {
        result = get_multiplicities_csv_file( options );

    // If the user specified a set of fasta files, read them.
    } else if( options.sequence_input.file_count() > 0 ) {
        result = get_multiplicities_fasta_files( options );

    // If the user didn't specify any way to input multiplicities, complain about it.
    } else {
        throw CLI::ValidationError(
            "--multiplicity-file, --fasta-path",
            "Exactly one of the ways to input per-query multiplicities has to be used."
        );
    }

    // Some user output for duplicates.
    if( ! result.second.empty() ) {
        std::sort( result.second.begin(),  result.second.end() );
        result.second.erase(
            std::unique( result.second.begin(), result.second.end() ),
            result.second.end()
        );

        LOG_WARN << "Warning: the multiplicity/fasta file(s) contain duplicate entries:";
        for( auto const& dup : result.second ) {
            LOG_WARN << " - " << dup;
        }
    }

    return result.first;
}

// =================================================================================================
//      Change Multiplicities
// =================================================================================================

void change_multiplicities( MultiplicityOptions const& options )
{
    // Check if any of the files we are going to produce already exists. If so, fail early.
    auto const bfns = options.jplace_input.base_file_names();
    std::vector<std::string> check_files;
    for( auto const& bfn : bfns ) {
        check_files.push_back( options.file_output.file_prefix() + bfn + "\\.jplace" );
    }
    options.file_output.check_nonexistent_output_files( check_files );

    // Get all multiplicites. That might need some memory, but for now, easier that way.
    auto multips = get_multiplicities( options );

    // Preparations.
    size_t file_count = 0;
    auto const set_size = options.jplace_input.file_count();
    size_t not_found = 0;

    // Run the loop for each jplace sample.
    #pragma omp parallel for schedule(dynamic)
    for( size_t fi = 0; fi < set_size; ++fi ) {

        // User output.
        LOG_MSG2 << "Processing file " << ( ++file_count ) << " of " << set_size
                 << ": " << options.jplace_input.file_path( fi );

        // Read the sample.
        auto sample = options.jplace_input.sample( fi );
        auto const basename = options.jplace_input.base_file_name( fi );

        // If there is an entry for this sample in the multiplicities, use it.
        // Otherwise, use the default empty one (which might be created if not present).
        auto const& smp_mult = (
            multips.count( basename ) > 0
            ? multips.at( basename )
            : multips[""]
        );

        // Set the new multiplicity for each pquery name.
        for( auto& pquery : sample ) {
            for( auto& pqn : pquery.names() ) {
                if( smp_mult.count( pqn.name ) > 0 ) {
                    pqn.multiplicity = smp_mult.at( pqn.name );
                } else {
                    ++not_found;

                    // User output.
                    LOG_MSG2 << "No multiplicity value found for pquery '" << pqn.name
                             << "' in sample " << basename << " ("
                             << options.jplace_input.file_path( fi ) <<  ").";
                }
            }
        }

        // Write sample back to file.
        auto const ofn = options.file_output.file_prefix() + basename + ".jplace";
        genesis::placement::JplaceWriter().write(
            sample,
            genesis::utils::to_file( options.file_output.out_dir() + ofn )
        );
    }

    if( not_found > 0 ) {
        LOG_MSG1 << "Warning: Could not find " << not_found << " pquery names.";
    }
}

// =================================================================================================
//      Write Multiplicities
// =================================================================================================

void write_multiplicities( MultiplicityOptions const& options )
{
    // Check if the produced file already exists. If so, fail early.
    options.file_output.check_nonexistent_output_files({
        options.file_output.file_prefix() + "multiplicities\\.csv"
    });

    // Store the multiplicites.
    SortedMultiplicityMap multips;

    // Preparations.
    size_t file_count = 0;
    auto const set_size = options.jplace_input.file_count();
    size_t duplicate_sample_cnt = 0;
    size_t duplicate_pquery_cnt = 0;

    // Run the loop for each jplace sample.
    #pragma omp parallel for schedule(dynamic)
    for( size_t fi = 0; fi < set_size; ++fi ) {

        // User output.
        LOG_MSG2 << "Processing file " << ( ++file_count ) << " of " << set_size << ": "
                 << options.jplace_input.file_path( fi );

        // Read the sample (multithreaded).
        auto const sample = options.jplace_input.sample( fi );
        auto const basename = options.jplace_input.base_file_name( fi );

        // Get and store the multiplicity for each pquery name. Single threaded.
        #pragma omp critical(GAPPA_MULTIPLICITY_ADD_MULTIPLICITY)
        {
            if( multips.count( basename ) > 0 ) {
                LOG_WARN << "Warning: Duplicate sample name '" + basename + "'. "
                         << "This will lead to misleading results if not fixed!";
            }

            for( auto& pquery : sample ) {
                for( auto& pqn : pquery.names() ) {

                    if( multips[ basename ].count( pqn.name ) > 0 ) {
                        ++duplicate_pquery_cnt;

                        LOG_MSG1 << "Duplicate pquery name '" << pqn.name
                                 << "' in sample " << basename << " ("
                                 << options.jplace_input.file_path( fi ) <<  ").";
                    }
                    multips[ basename ][ pqn.name ] = pqn.multiplicity;
                }
            }
        }
    }

    // User warnings
    if( duplicate_pquery_cnt > 0 ) {
        LOG_WARN << "Warning: There were " << duplicate_pquery_cnt << " duplicate pquery names.";
    }
    if( duplicate_sample_cnt > 0 ) {
        LOG_WARN << "Warning: There were " << duplicate_sample_cnt << " duplicate sample names.";
    }

    // Prepare file.
    auto const filename
        = options.file_output.out_dir()
        + options.file_output.file_prefix()
        + "multiplicities.csv"
    ;

    // User output
    LOG_MSG1 << "Writing multiplicity file: " << filename;

    // TODO proper escaping! use csv writer class, if we add one to genesis.
    // TODO use the separator char here as well!

    // Write the multiplicity file.
    std::ofstream ofs;
    genesis::utils::file_output_stream( filename, ofs );
    for( auto const& sample : multips ) {
        for( auto const& pquery : sample.second ) {
            ofs << sample.first << "\t" << pquery.first << "\t" << pquery.second << "\n";
        }
    }
    ofs.close();
}

// =================================================================================================
//      Run
// =================================================================================================

void run_multiplicity( MultiplicityOptions const& options )
{
    // Print some user output.
    options.jplace_input.print();

    if( options.write_multiplicity_file ) {
        write_multiplicities( options );
    } else {
        change_multiplicities( options );
    }
}
