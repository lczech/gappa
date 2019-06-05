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

#include "commands/edit/split.hpp"

#include "options/global.hpp"

#include "CLI/CLI.hpp"

#include "genesis/placement/formats/jplace_writer.hpp"
#include "genesis/placement/function/functions.hpp"
#include "genesis/placement/function/sample_set.hpp"
#include "genesis/utils/containers/matrix.hpp"
#include "genesis/utils/core/algorithm.hpp"
#include "genesis/utils/formats/csv/reader.hpp"
#include "genesis/utils/io/input_source.hpp"
#include "genesis/utils/io/input_stream.hpp"

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
 * @brief Describes a sample by its name and a list of its pqueries and their abundances.
 */
struct SampleContent
{
    /**
     * @brief Name of the sample.
     */
    std::string name;

    /**
     * @brief Map from a pquery (index in the pquery_names vector) to its abundance.
     *
     * We use this form instead of a full OTU table, because those are often quite spare,
     * and we do not want to waste memory.
     */
    std::unordered_map<size_t, double> pqueries;
};

/**
 * @brief Structure that holds an entiere OTU table. This is the internal format used here.
 */
struct OtuTable
{
    /**
     * @brief Names of the pqueries. Indices in the vector are used for lookup.
     */
    std::vector<std::string> pquery_names;

    /**
     * @brief Keep the content of each sample.
     */
    std::vector<SampleContent> samples;
};

// =================================================================================================
//      Setup
// =================================================================================================

void setup_split( CLI::App& app )
{
    // Create the options and subcommand objects.
    auto options = std::make_shared<SplitOptions>();
    auto sub = app.add_subcommand(
        "split",
        "Split the queries in jplace files into multiple files, "
        "for example, according to an OTU table."
    );

    // -----------------------------------------------------------
    //     Input options
    // -----------------------------------------------------------

    // Jplace input
    options->jplace_input.add_jplace_input_opt_to_app( sub );

    // Split file
    auto split_file_opt = sub->add_option(
        "--split-file",
        options->split_file,
        "File containing a comma-separated mapping of query names to sample names."
    );
    split_file_opt->check( CLI::ExistingFile );
    split_file_opt->group( "Input" );

    // OTU table file
    auto otu_table_file_opt = sub->add_option(
        "--otu-table-file",
        options->otu_table_file,
        "File containing a tab-separated OTU table."
    );
    otu_table_file_opt->check( CLI::ExistingFile );
    otu_table_file_opt->group( "Input" );

    // Make two modes mutually exclusive.
    split_file_opt->excludes( otu_table_file_opt );
    otu_table_file_opt->excludes( split_file_opt );

    // TODO make an option to select the columns, instead of having to prune the table
    // if it contains unwanted columns (which most otu tables do...)

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
    sub->callback( [ options ]() {
        run_split( *options );
    });
}

// =================================================================================================
//      Read Split File
// =================================================================================================

OtuTable read_split_file( SplitOptions const& options )
{
    using namespace genesis;
    assert( ! options.split_file.empty() );
    assert( options.otu_table_file.empty() );

    // Result
    OtuTable result;

    // User output.
    if( global_options.verbosity() >= 1 ) {
        std::cout << "Reading split file.\n";
    }

    // TODO allow to customize the separator char here (and later in the OTU table as well)!

    // Read the file
    size_t line_size = 0;
    auto reader = utils::CsvReader();
    reader.separator_chars( "," );
    auto const table = reader.read( utils::from_file( options.split_file ));

    // The list in the file is not expected to be sorted. Thus, use lookups to find entries.
    std::unordered_map<std::string, size_t> pquery_to_index;
    std::unordered_map<std::string, size_t> sample_to_index;

    // Iterate it once to get all pquery names and sample in a fixed order.
    // Unfortunate, but the price we pay for a smaller data structure.
    for( auto const& line : table ) {

        // Some consistency check. We do not allow to mix line sizes in one file.
        if( line_size == 0 ) {
            line_size = line.size();
        }
        if( ! (( line.size() == 2 && line_size == 2 ) || ( line.size() == 3 && line_size == 3 )) ) {
            throw CLI::ValidationError(
                "--split-file (" + options.split_file +  ")",
                "Invalid split file. Needs to be a comma-separated list of fields, "
                "with either two or three fields per line: A pquery name, a sample name, "
                "and optionally its multiplicity."
            );
        }
        assert( line.size() == 2 || line.size() == 3 );

        // If the pquery name does not already have an index, give it one, and add it to the list.
        auto const& pquery_name = line[0];
        if( pquery_to_index.count( pquery_name ) == 0 ) {
            pquery_to_index[ pquery_name ] = result.pquery_names.size();
            result.pquery_names.push_back( pquery_name );
        }

        // Assert that it worked.
        assert( pquery_to_index.count( pquery_name ) > 0 );
        assert( pquery_to_index.at( pquery_name ) < result.pquery_names.size() );
        assert( result.pquery_names.at( pquery_to_index.at( pquery_name )) == pquery_name );

        // Same for the sample name.
        auto const& sample_name = line[1];
        if( sample_to_index.count( sample_name ) == 0 ) {
            sample_to_index[ sample_name ] = result.samples.size();
            result.samples.push_back({});
            result.samples.back().name = sample_name;
        }
    }

    // Iterate it again, this time to fill the samples.
    for( auto const& line : table ) {
        assert(( line.size() == 2 && line_size == 2 ) || ( line.size() == 3 && line_size == 3 ));
        assert( line.size() == 2 || line.size() == 3 );

        // Get the parts of the line.
        auto const& pquery_name = line[0];
        auto const& sample_name = line[1];
        double      multip = 1.0;

        // Convert the multiplicity, if present.
        if( line.size() == 3 ) {
            try{
                multip = std::stod( line[2] );
            } catch( ... ) {
                throw CLI::ValidationError(
                    "--split-file (" + options.split_file +  ")",
                    "Invalid multiplicity entry, cannot parse as a number: '" + line[2] + "'."
                );
            }
        }

        // Add it to the result.
        assert( sample_to_index.count( sample_name ) > 0 );
        assert( pquery_to_index.count( pquery_name ) > 0 );
        assert( sample_to_index.at( sample_name ) < result.samples.size() );
        auto& sample_entry = result.samples.at( sample_to_index.at( sample_name ));
        auto const pquery_idx = pquery_to_index.at( pquery_name );
        if( sample_entry.pqueries.count( pquery_idx ) > 0 ) {
            std::cout << "Duplicate entry for pquery '" << pquery_name << "' and sample ";
            std::cout << sample_name << ". Adding up their multiplicities.\n";
        }
        sample_entry.pqueries[ pquery_idx ] += multip;
    }

    return result;
}

// =================================================================================================
//      Read OTU Table File
// =================================================================================================

OtuTable read_otu_table_file( SplitOptions const& options )
{
    using namespace genesis;
    assert( ! options.otu_table_file.empty() );
    assert( options.split_file.empty() );

    // Result
    OtuTable result;

    // User output.
    if( global_options.verbosity() >= 1 ) {
        std::cout << "Reading OTU table.\n";
    }

    // TODO allow to customize the separator char here (and later in the OTU table as well)!

    // Prepare to read the file
    auto reader = utils::CsvReader();
    reader.separator_chars( "\t" );
    utils::InputStream otu_is( utils::from_file( options.otu_table_file ) );

    // Get header.
    if( !otu_is ) {
        throw std::runtime_error( "Empty OTU table file." );
    }
    auto const header = reader.parse_line( otu_is );
    if( header.size() < 2 ) {
        throw std::runtime_error( "Invalid OTU table with less than two columns." );
    }

    // Add a sample for each element in the header (except the first, which is the header for
    // the pquery names column).
    for( size_t i = 1; i < header.size(); ++i ) {
        // Check for duplicate sample names.
        for( auto const& sc : result.samples ) {
            if( sc.name == header[i] ) {
                throw std::runtime_error(
                    "Duplicate sample name '" + header[i] + "' in OTU table."
                );
            }
        }

        // Add sample name
        result.samples.push_back({});
        result.samples.back().name = header[i];
        assert( result.samples.size() == i );
    }

    // Read the table and fill the rest of our result.
    while( otu_is ) {
        auto const line = reader.parse_line( otu_is );
        if( line.size() != header.size() ) {
            throw std::runtime_error( "Invalid OTU table with inconsistent number of columns." );
        }
        assert( line.size() >= 2 );

        // Get the pquery name (first column), and check for duplicates.
        auto const& pquery_name = line[0];
        if( utils::contains( result.pquery_names, pquery_name ) ) {
            throw std::runtime_error( "Duplicate pquery name '" + pquery_name + "' in OTU table." );
        }

        // Add pquery name.
        auto const pquery_idx = result.pquery_names.size();
        result.pquery_names.push_back( pquery_name );

        // Add the per-sample entries (other columns).
        for( size_t i = 1; i < line.size(); ++i ) {

            // Get multiplicity.
            double multip = 1.0;
            try{
                multip = std::stod( line[i] );
            } catch( ... ) {
                throw CLI::ValidationError(
                    "--otu-table-file (" + options.otu_table_file +  ")",
                    "Invalid multiplicity entry, cannot parse as a number: '" + line[i] + "'."
                );
            }

            // Only add to the sample if non-zero value!
            // We get the sample at i-1, because this is where it was added
            // when the header line was processed.
            if( std::isfinite(multip) && multip > 0.0 ) {
                result.samples.at( i-1 ).pqueries[ pquery_idx ] = multip;
            }
        }
    }

    return result;
}

// =================================================================================================
//      Run
// =================================================================================================

void run_split( SplitOptions const& options )
{
    using namespace genesis;
    using namespace genesis::placement;

    // Read the split information from the specified file.
    OtuTable otu_table;
    if( ! options.split_file.empty() ) {
        otu_table = read_split_file( options );
    } else if( ! options.otu_table_file.empty() ) {
        otu_table = read_otu_table_file( options );
    } else {
        throw CLI::ValidationError(
            "--split-file, --otu-table-file",
            "Exactly one of the ways to input the split information has to be used."
        );
    }

    // Check if any of the files we are going to produce already exists. If so, fail early.
    std::vector<std::string> check_files;
    for( auto const& sample : otu_table.samples ) {
        check_files.push_back( options.jplace_output.file_prefix() + sample.name + "\\.jplace" );
    }
    options.jplace_output.check_nonexistent_output_files( check_files );

    // Print some user output.
    options.jplace_input.print();

    // User output.
    if( global_options.verbosity() >= 1 ) {
        std::cout << "Reading sample" << ( options.jplace_input.file_count() > 1 ? "s" : "" );
        std::cout << ".\n";
    }

    // Read all jplace files at once. Typically, this command is run with one file anyway.
    auto const sample_set = options.jplace_input.sample_set();
    if( sample_set.size() == 0 ) {
        // The jplace input is required, so we should have at least one sample!
        throw std::runtime_error( "Internal error: Expecting multiple samples." );
    }

    // Get the reference tree from the samples.
    if( ! all_identical_trees( sample_set )) {
        throw std::runtime_error(
            "Cannot process multiple jplace samples if they have different reference trees."
        );
    }
    auto const& ref_tree = sample_set[0].tree();
    // TODO make average branch length?!

    // Create a mapping from pquery names to the pquery that contains the name.
    std::unordered_map<std::string, Pquery const*> name_map;
    for( auto const& sample : sample_set ) {
        for( auto const& pquery : sample ) {
            for( auto const& pname : pquery.names() ) {
                if( name_map.count( pname.name ) > 0 ) {
                    std::cout << "Duplicate pquery name '" << pname.name;
                    std::cout << "' in the input jplace file(s).\n";
                }
                name_map[ pname.name ] = &pquery;
            }
        }
    }

    // User output.
    if( global_options.verbosity() >= 1 ) {
        std::cout << "Writing split samples.\n";
    }

    // Create and write the split target samples.
    size_t file_count = 0;
    #pragma omp parallel for schedule(dynamic)
    for( size_t si = 0; si < otu_table.samples.size(); ++si ) {
        auto const& sample_entry = otu_table.samples[si];
        auto const filename = options.jplace_output.file_prefix() + sample_entry.name + ".jplace";

        // User output.
        if( global_options.verbosity() >= 2 ) {
            #pragma omp critical(GAPPA_SPLIT_PRINT)
            {
                // TODO maybe output the full path including out dir?
                ++file_count;
                std::cout << "Writing file " << file_count << " of " << otu_table.samples.size();
                std::cout << ": " << filename << "\n";
            }
        }

        // Create a new sample and fill it with the needed pqueries.
        auto target = Sample( ref_tree );
        for( auto const& pqry_entry : sample_entry.pqueries ) {
            auto const& pquery_name = otu_table.pquery_names.at( pqry_entry.first );
            auto const& multip = pqry_entry.second;

            // See if we find the pquery in the jplace input files.
            if( name_map.count( pquery_name ) == 0 ) {
                #pragma omp critical(GAPPA_SPLIT_PRINT)
                {
                    std::cout << "Warning: No pquery with name '" << pquery_name;
                    std::cout << "' found in input samples.\n";
                }
                continue;
            }

            // Add to the target sample. We keep it simple here and first copy everything,
            // including names, then delete them again and add just the one that we want.
            // This way, we can use the copy mechanism of Sample.add(), which adjusts all
            // pointers etc for us!
            auto& new_pqry = target.add( *( name_map.at( pquery_name )));
            new_pqry.clear_names();
            new_pqry.add_name( pquery_name, multip );
        }

        // Write the new sample to a file.
        JplaceWriter().to_file( target, options.jplace_output.out_dir() + filename );
    }
}
