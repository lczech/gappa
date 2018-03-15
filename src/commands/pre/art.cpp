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

#include "commands/pre/art.hpp"

#include "options/global.hpp"

#include "CLI/CLI.hpp"

#include "genesis/sequence/counts.hpp"
#include "genesis/sequence/formats/fasta_input_iterator.hpp"
#include "genesis/sequence/formats/fasta_writer.hpp"
#include "genesis/sequence/functions/consensus.hpp"
#include "genesis/sequence/functions/entropy.hpp"
#include "genesis/sequence/functions/labels.hpp"
#include "genesis/sequence/sequence_set.hpp"
#include "genesis/sequence/sequence.hpp"

#include "genesis/taxonomy/formats/taxonomy_reader.hpp"
#include "genesis/taxonomy/formats/taxopath_generator.hpp"
#include "genesis/taxonomy/formats/taxopath_parser.hpp"
#include "genesis/taxonomy/functions/entropy_data.hpp"
#include "genesis/taxonomy/functions/entropy.hpp"
#include "genesis/taxonomy/functions/taxonomy.hpp"
#include "genesis/taxonomy/functions/taxopath.hpp"
#include "genesis/taxonomy/iterator/preorder.hpp"
#include "genesis/taxonomy/taxon.hpp"
#include "genesis/taxonomy/taxonomy.hpp"
#include "genesis/taxonomy/taxopath.hpp"

#include "genesis/utils/core/std.hpp"
#include "genesis/utils/io/output_stream.hpp"
#include "genesis/utils/text/string.hpp"

#ifdef GENESIS_OPENMP
#   include <omp.h>
#endif

#include <cctype>
#include <map>
#include <fstream>

// =================================================================================================
//      Typedefs
// =================================================================================================

// =================================================================================================
//      Setup
// =================================================================================================

void setup_art( CLI::App& app )
{
    // Create the options and subcommand objects.
    auto opt = std::make_shared<ArtOptions>();
    auto sub = app.add_subcommand(
        "art",
        "Generate consensus sequences from a sequence database according to the ART method."
    );

    // -----------------------------------------------------------
    //     Add common options
    // -----------------------------------------------------------

    // opt->sequence_input.add_fasta_input_opt_to_app( sub );

    opt->output.add_output_dir_opt_to_app( sub );

    // -----------------------------------------------------------
    //     Input Data
    // -----------------------------------------------------------

    // Taxonomy file
    auto tax_file_opt = sub->add_option(
        "--taxonomy-file",
        opt->taxonomy_file,
        "File that lists the taxa of the database."
    );
    tax_file_opt->required();
    tax_file_opt->check( CLI::ExistingFile );

    // Sequence file
    auto seq_file_opt = sub->add_option(
        "--sequence-file",
        opt->sequence_file,
        "Fasta file containing the sequences of the database."
    );
    seq_file_opt->required();
    seq_file_opt->check( CLI::ExistingFile );

    // -----------------------------------------------------------
    //     Entropy pruning option
    // -----------------------------------------------------------

    // Target size
    auto target_size_opt = sub->add_option(
        "--target-size",
        opt->target_taxonomy_size,
        "Target size of how many taxa to selecte for building consensus sequences."
    );
    target_size_opt->required();

    // Min subclade size
    // auto min_subclade_size_opt =
    sub->add_option(
        "--min-subclade-size",
        opt->min_subclade_size,
        "Minimal size of sub-clades. Everything below is expanded."
    );

    // Max subclade size
    // auto max_subclade_size_opt = sub->add_option(
    //     "--max-subclade-size",
    //     opt->max_subclade_size,
    //     "Minimal size of sub-clades. Everything below is expanded."
    // );

    // Min tax level
    // auto min_tax_level_opt =
    sub->add_option(
        "--min-tax-level",
        opt->min_tax_level,
        "Minimal taxonomic level. Taxa below this level are always expanded."
    );

    // Allow approximation
    // auto allow_approx_opt =
    sub->add_flag(
        "--allow-approximation",
        opt->allow_approximation,
        "Allow to expand taxa that help getting closer to the --target-size, even if they are not "
        "the ones with the highest entropy."
    );

    // Write info files
    sub->add_flag(
        "--write-info-files",
        opt->write_info_files,
        "If set, two additional info files are written, containing the new pruned taxonomy, "
        "as well as the entropy of all clades of the original taxonomy."
    );

    // -----------------------------------------------------------
    //     Consensus options
    // -----------------------------------------------------------

    // Consensus Method
    auto cons_meth_opt = sub->add_set_ignore_case(
        "--consensus-method",
        opt->consensus_method,
        { "majorities", "cavener", "threshold" },
        "Consensus method to use for combining sequences.",
        true
    );

    // Consensus Treshold
    auto cons_thresh_opt = sub->add_option(
        "--consensus-threshold",
        opt->consensus_threshold,
        "Threshold value to use with --consensus-method threshold. Has to be in [ 0.0, 1.0 ].",
        true
    );
    cons_thresh_opt->requires( cons_meth_opt );
    cons_thresh_opt->check( CLI::Range( 0.0, 1.0 ));

    // -----------------------------------------------------------
    //     Callback
    // -----------------------------------------------------------

    // Set the run function as callback to be called when this subcommand is issued.
    // Hand over the options by copy, so that their shared ptr stays alive in the lambda.
    sub->set_callback( [ opt ]() {
        run_art( *opt );
    });
}

// =================================================================================================
//      Read Taxonomy
// =================================================================================================

genesis::taxonomy::Taxonomy read_taxonomy( ArtOptions const& options )
{
    using namespace genesis::sequence;
    using namespace genesis::taxonomy;

    // User output.
    if( global_options.verbosity() >= 1 ) {
        std::cout << "Reading taxonomy and preparing entropy calculations.\n";
    }

    // Get alignment length.
    auto it = FastaInputIterator();
    it.from_file( options.sequence_file );
    auto const seq_len = it->size();

    // Read from file.
    auto tax = Taxonomy();
    TaxonomyReader().from_file( options.taxonomy_file, tax );
    sort_by_name( tax );

    // TODO only allocate for the sub tax

    // Create a Sequence Count objeect for each taxon.
    // This might allocate quite a lot of memory!
    auto add_sequence_counts_to_taxonomy = [&]( Taxon& taxon ){
        taxon.reset_data( EntropyTaxonData::create() );
        taxon.data<EntropyTaxonData>().counts = SiteCounts( "ACGT", seq_len );
    };
    preorder_for_each( tax, add_sequence_counts_to_taxonomy );

    // User output.
    if( global_options.verbosity() >= 1 ) {
        std::cout << "Taxonomy contains a total of " << total_taxa_count( tax ) << " taxa, ";
        std::cout << "with " << taxa_count_lowest_levels( tax ) << " taxa at the lowest level.\n";
    }

    return tax;
}

// =================================================================================================
//      Fill Site Counts
// =================================================================================================

void fill_site_counts( ArtOptions const& options, genesis::taxonomy::Taxonomy& tax )
{
    using namespace genesis::sequence;
    using namespace genesis::taxonomy;

    // User output.
    if( global_options.verbosity() >= 1 ) {
        std::cout << "Reading sequences.\n";
    }

    // User output prep.
    std::map<char, size_t> char_counts;
    size_t total_seqs_count = 0;
    size_t no_tax_seqs_count = 0;

    // Prepare helpers.
    auto taxopath_parser = TaxopathParser();
    auto fasta_reader = FastaReader();
    fasta_reader.site_casing( FastaReader::SiteCasing::kToUpper );

    // Iterate sequences.
    auto it = FastaInputIterator( fasta_reader );
    for( it.from_file( options.sequence_file ); it; ++it ) {

        // User output.
        // If we have verbose output, count characters.
        if( global_options.verbosity() >= 2 ) {
            for( auto const& s : *it ) {
                ++char_counts[s];
            }

            // Also, output a progress. Could be done nicer in the future.
            if( total_seqs_count % 100000 == 0 ) {
                std::cout << "At sequence " << total_seqs_count << "\n";
            }
        }
        ++total_seqs_count;

        // Get taxo path of the sequence.
        // We offer two versions: the sequence label is just the path, or it starts at the first space.
        std::string taxopath_str;
        auto const delim = it->label().find_first_of( " \t" );
        if( delim == std::string::npos ) {
            taxopath_str = it->label();
        } else {
            taxopath_str = it->label().substr( delim + 1 );
        }

        // TODO distinguish between not found at all and not in the specified sub taxonomy

        // Parse the taxo path and find it in the taxonomy.
        // If the first attempt fails, remove the last element (assumed to be species level),
        // and try again. If we fail again, we cannot use this sequence.
        auto taxopath = taxopath_parser( taxopath_str );
        auto taxp = find_taxon_by_taxopath( tax, taxopath );
        if( taxp == nullptr ) {
            taxopath.pop_back();
            taxp = find_taxon_by_taxopath( tax, taxopath );
        }
        if( taxp == nullptr ) {
            ++no_tax_seqs_count;
            if( global_options.verbosity() >= 3 ) {
                std::cout << "Sequence " << it->label() << " not found in the taxonomy!\n";
            }
            continue;
        }

        // Accummulate counts for all taxonomic ranks.
        auto cur_tax = taxp;
        do {
            cur_tax->data<EntropyTaxonData>().counts.add_sequence( *it );
            cur_tax = cur_tax->parent();
        } while( cur_tax != nullptr );
    }

    // User output.
    if( global_options.verbosity() >= 1 || no_tax_seqs_count > 0 ) {
        std::cout << "Processed " << total_seqs_count << " sequences.\n";
        if( no_tax_seqs_count > 0 ) {
            std::cout << "Thereof, " << no_tax_seqs_count << " sequences were not found in the taxonomy.\n";
        }
    }
    if( global_options.verbosity() >= 2 ) {
        std::cout << "Character counts in the sequences:\n";
        for( auto const& count : char_counts ) {
            std::cout << "    " << count.first << ": " << count.second << "\n";
        }
    }
    if( char_counts['U'] > char_counts['T'] ) {
        // The above condition might add U or T to the char counts map.
        // We don't use it any more afterwards, so that's okay.
        std::cout << "Warning: There are more 'U' sites in the sequences than 'T' sites. ";
        std::cout << "Are you sure that the sites are properly converted to 'T'?\n";
    }
}

// =================================================================================================
//      Calculate Entropy
// =================================================================================================

void calculate_entropy( ArtOptions const& options, genesis::taxonomy::Taxonomy& tax )
{
    using namespace genesis::sequence;
    using namespace genesis::taxonomy;

    // Currently, we do not need any options for this step.
    // Could change later if we want to offer entropy settings, see below.
    (void) options;

    // User output.
    if( global_options.verbosity() >= 1 ) {
        std::cout << "Calculating entropy.\n";
    }

    // Set the default options.
    // auto opt = SiteEntropyOptions::kWeighted;
    auto opt = SiteEntropyOptions::kIncludeGaps;

    // Calculate!
    auto calc_entropies = [&]( Taxon& t ) {
        auto const& counts = t.data<EntropyTaxonData>().counts;
        t.data<EntropyTaxonData>().entropy = averaged_entropy( counts, false, opt );
    };
    preorder_for_each( tax, calc_entropies );
}

// =================================================================================================
//      Select Taxa
// =================================================================================================

void select_taxa( ArtOptions const& options, genesis::taxonomy::Taxonomy& tax )
{
    using namespace genesis::sequence;
    using namespace genesis::taxonomy;

    // User output.
    if( global_options.verbosity() >= 1 ) {
        std::cout << "Selecting taxa based on entropy.\n";
    }

    // Get algo settings.
    PruneByEntropySettings prune_settings;
    prune_settings.min_subtaxonomy_size = options.min_subclade_size;
    prune_settings.max_subtaxonomy_size = options.max_subclade_size;
    prune_settings.min_border_level     = options.min_tax_level;
    prune_settings.allow_approximation  = options.allow_approximation;

    // Run Forrest, run!
    prune_by_entropy( tax, options.target_taxonomy_size, prune_settings );
    if( ! validate_pruned_taxonomy( tax ) ) {
        throw std::runtime_error( "Something went wrong, the selected taxa are inconsistent.\n" );
    }

    // User output.
    if( global_options.verbosity() >= 1 ) {
        auto const border_cnt = count_taxa_with_prune_status(
            tax, EntropyTaxonData::PruneStatus::kBorder
        );
        std::cout << "Selected " << border_cnt << " taxa for which to build consensus sequences.\n";
    }
}

// =================================================================================================
//      Generate Consensus Sequences
// =================================================================================================

void generate_consensus_sequences( ArtOptions const& options, genesis::taxonomy::Taxonomy const& tax )
{
    using namespace genesis::sequence;
    using namespace genesis::taxonomy;

    // User output.
    if( global_options.verbosity() >= 1 ) {
        std::cout << "Generating consensus sequences.\n";
    }

    // Helper function.
    auto write_fasta_sequence = [] ( std::ofstream& out, std::string const& name, std::string const& sites ) {
        out << ">" << name << "\n";
        for (size_t i = 0; i < sites.length(); i += 80) {
            out << sites.substr(i, 80) << "\n";
        }
    };

    // Prepare output.
    // TODO check with file overwrite settings
    std::ofstream cons_seq_os;
    auto const fn = options.output.out_dir() + options.consensus_sequence_file;
    genesis::utils::file_output_stream( fn, cons_seq_os );
    auto const tax_gen = TaxopathGenerator();

    // Calculate consensus sequences and write them.
    auto make_consensus_sequences = [&]( Taxon const& t ) {

        // Only interested in the border taxa.
        if( t.data<EntropyTaxonData>().status != EntropyTaxonData::PruneStatus::kBorder ) {
            return;
        }

        // Prep.
        auto const name = sanitize_label( tax_gen(t) );
        auto const& counts = t.data<EntropyTaxonData>().counts;

        // Consensus sequence.
        std::string sites;
        if( options.consensus_method == "majorities" ) {
            sites = consensus_sequence_with_majorities( counts );
        } else if( options.consensus_method == "cavener" ) {
            sites = consensus_sequence_cavener( counts );
        } else if( options.consensus_method == "threshold" ) {
            sites = consensus_sequence_with_threshold( counts, options.consensus_threshold );
        } else {
            throw CLI::ConversionError( "Unknown consensus method: " + options.consensus_method );
        }
        write_fasta_sequence( cons_seq_os, name, sites );

    };
    preorder_for_each( tax, make_consensus_sequences );
    cons_seq_os.close();
}

// =================================================================================================
//      Write Taxonomy Info
// =================================================================================================

void write_info_files( ArtOptions const& options, genesis::taxonomy::Taxonomy const& tax )
{
    using namespace genesis::taxonomy;

    if( ! options.write_info_files ) {
        return;
    }

    // TODO check with file overwrite settings

    // Prepare entropy output.
    std::ofstream entropy_os;
    auto const entropy_fn = options.output.out_dir() + options.entropy_info_file;
    genesis::utils::file_output_stream( entropy_fn, entropy_os );
    entropy_os << "Taxon\tStatus\tChild_Taxa\tTotal_Taxa\tLowest_Level_Taxa\tSequences\tEntropy\n";

    // Prepare taxonomy output.
    std::ofstream taxonomy_os;
    auto const taxonomy_fn = options.output.out_dir() + options.taxonomy_info_file;
    genesis::utils::file_output_stream( taxonomy_fn, taxonomy_os );
    taxonomy_os << "Taxon\tChild_Taxa\tTotal_Taxa\tLowest_Level_Taxa\n";

    // Write to files.
    auto const gen = TaxopathGenerator();
    auto print_taxon_info = [&]( Taxon const& t )
    {
        // Calculate values.
        auto const name = gen( t );
        auto const total_chldrn = total_taxa_count( t );
        auto const lowest_chldrn = taxa_count_lowest_levels( t );
        auto const added_seqs = t.data<EntropyTaxonData>().counts.added_sequences_count();
        auto const entr = t.data<EntropyTaxonData>().entropy;

        // Status: was the taxon selected or not.
        std::string status = "-";
        if( t.data<EntropyTaxonData>().status == EntropyTaxonData::PruneStatus::kOutside ) {
            status = "Outside";
        }
        if( t.data<EntropyTaxonData>().status == EntropyTaxonData::PruneStatus::kBorder ) {
            status = "Selected";
        }
        if( t.data<EntropyTaxonData>().status == EntropyTaxonData::PruneStatus::kInside ) {
            status = "Inside";
        }

        // For all taxa, write out entropy info.
        entropy_os << name;
        entropy_os << "\t" << status;
        entropy_os << "\t" << t.size();
        entropy_os << "\t" << total_chldrn;
        entropy_os << "\t" << lowest_chldrn;
        entropy_os << "\t" << added_seqs;
        entropy_os << "\t" << entr;
        entropy_os << "\n";

        // Write all inner and border taxa to the taxnomy file.
        if( t.data<EntropyTaxonData>().status != EntropyTaxonData::PruneStatus::kOutside ) {
            taxonomy_os << name;
            taxonomy_os << "\t" << t.size();
            taxonomy_os << "\t" << total_chldrn;
            taxonomy_os << "\t" << lowest_chldrn;
            taxonomy_os << "\n";
        }
    };
    preorder_for_each( tax, print_taxon_info );

    // Wrap up.
    entropy_os.close();
    taxonomy_os.close();
}

// =================================================================================================
//      Run
// =================================================================================================

void run_art( ArtOptions const& options )
{
    using namespace genesis::utils;

    // TODO check output files
    options.output.check_nonexistent_output_files({ options.consensus_sequence_file });

    // TODO option for selecting sub taxonomy only!

    // Run the whole thing!
    auto taxonomy = read_taxonomy( options );
    fill_site_counts( options, taxonomy );
    calculate_entropy( options, taxonomy );
    select_taxa( options, taxonomy );
    generate_consensus_sequences( options, taxonomy );
    write_info_files( options, taxonomy );

}
