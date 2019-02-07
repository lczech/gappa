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

#include "commands/edit/multiplicity.hpp"

#include "options/global.hpp"

#include "CLI/CLI.hpp"

#include "genesis/placement/formats/jplace_writer.hpp"
#include "genesis/sequence/formats/fasta_input_iterator.hpp"
#include "genesis/sequence/formats/fasta_reader.hpp"
#include "genesis/sequence/functions/labels.hpp"
#include "genesis/utils/formats/csv/reader.hpp"

#include <algorithm>
#include <cassert>
#include <fstream>
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

    // -----------------------------------------------------------
    //     Output options
    // -----------------------------------------------------------

    options->jplace_output.add_output_dir_opt_to_app( sub );
    options->jplace_output.add_file_prefix_opt_to_app( sub );

    // -----------------------------------------------------------
    //     Callback
    // -----------------------------------------------------------

    // Set the run function as callback to be called when this subcommand is issued.
    // Hand over the options by copy, so that their shared ptr stays alive in the lambda.
    sub->set_callback( [ options ]() {
        run_multiplicity( *options );
    });
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

    // Already checked upon callind this function. Assert again here.
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
    if( global_options.verbosity() >= 1 ) {
        std::cout << "Reading multiplicities.\n";
    }

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
        std::cout << "Warning: the multiplicity/fasta file(s) contain duplicate entries";
        if( global_options.verbosity() <= 1 ) {
            std::cout << ".\n";
        } else {
            std::cout << ":\n";
            std::sort( result.second.begin(),  result.second.end() );
            result.second.erase(
                std::unique( result.second.begin(), result.second.end() ),
                result.second.end()
            );
            for( auto const& dup : result.second ) {
                std::cout << " - " << dup << "\n";
            }
        }
    }

    return result.first;
}

// =================================================================================================
//      Run
// =================================================================================================

void run_multiplicity( MultiplicityOptions const& options )
{
    using namespace genesis;

    // Check if any of the files we are going to produce already exists. If so, fail early.
    auto const bfns = options.jplace_input.base_file_names();
    std::vector<std::string> check_files;
    for( auto const& bfn : bfns ) {
        check_files.push_back( options.jplace_output.file_prefix() + bfn + "\\.jplace" );
    }
    options.jplace_output.check_nonexistent_output_files( check_files );

    // Print some user output.
    options.jplace_input.print();

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
        if( global_options.verbosity() >= 2 ) {
            #pragma omp critical(GAPPA_MULTIPLICITY_PRINT)
            {
                ++file_count;
                std::cout << "Processing file " << file_count << " of " << set_size;
                std::cout << ": " << options.jplace_input.file_path( fi ) << "\n";
            }
        }

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
                    if( global_options.verbosity() >= 2 ) {
                        #pragma omp critical(GAPPA_MULTIPLICITY_PRINT)
                        {
                            std::cout << "No multilicity value found for pquery " << pqn.name;
                            std::cout << " in sample " << basename << " (";
                            std::cout << options.jplace_input.file_path( fi ) <<  ").\n";
                        }
                    }
                }
            }
        }

        // Write sample back to file.
        auto const ofn = options.jplace_output.file_prefix() + basename + ".jplace";
        placement::JplaceWriter().to_file( sample, options.jplace_output.out_dir() + ofn );
    }

    if( global_options.verbosity() <= 1 ) {
        std::cout << "Warning: Could not find " << not_found << " pquery names.\n";
    }
}
