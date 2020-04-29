/*
    gappa - Genesis Applications for Phylogenetic Placement Analysis
    Copyright (C) 2017-2019 Pierre Barbera, Lucas Czech and HITS gGmbH

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

#include "commands/examine/assign.hpp"

#include "options/global.hpp"
#include "tools/cli_setup.hpp"

#include "CLI/CLI.hpp"

#include "genesis/taxonomy/formats/taxopath_parser.hpp"
#include "genesis/taxonomy/formats/taxopath_generator.hpp"
#include "genesis/taxonomy/formats/taxonomy_reader.hpp"
#include "genesis/taxonomy/functions/taxopath.hpp"
#include "genesis/taxonomy/functions/taxonomy.hpp"
#include "genesis/taxonomy/iterator/preorder.hpp"
#include "genesis/taxonomy/iterator/postorder.hpp"
#include "genesis/taxonomy/taxopath.hpp"
#include "genesis/taxonomy/taxonomy.hpp"

#include "genesis/utils/formats/csv/reader.hpp"
#include "genesis/utils/io/input_source.hpp"
#include "genesis/utils/io/input_stream.hpp"
#include "genesis/utils/io/output_stream.hpp"
#include "genesis/utils/text/string.hpp"

#include "genesis/tree/common_tree/functions.hpp"
#include "genesis/tree/common_tree/tree.hpp"
#include "genesis/tree/common_tree/newick_writer.hpp"
#include "genesis/tree/iterator/postorder.hpp"
#include "genesis/tree/function/functions.hpp"
#include "genesis/tree/bipartition/functions.hpp"
#include "genesis/tree/tree/edge.hpp"

#include "genesis/placement/sample.hpp"
#include "genesis/placement/pquery.hpp"
#include "genesis/placement/pquery/placement.hpp"
#include "genesis/placement/function/manipulation.hpp"

#include <fstream>

#ifdef GENESIS_OPENMP
#   include <omp.h>
#endif

// =================================================================================================
//      Setup
// =================================================================================================

void setup_assign( CLI::App& app )
{
    // Create the options and subcommand objects.
    auto opt = std::make_shared<AssignOptions>();
    auto sub = app.add_subcommand(
        "assign",
        "Taxonomically assign placed query sequences and output tabulated summarization."
    );

    // Input
    opt->jplace_input.add_jplace_input_opt_to_app( sub )->required();

    sub->add_option(
        "--taxon-file",
        opt->taxon_map_file,
        "File containing a tab-separated list of reference taxon to taxonomic string assignments."
    )->check(CLI::ExistingFile)->required()->group("Input");

    sub->add_option(
        "--root-outgroup",
        opt->outgroup_file,
        "Root the tree by the outgroup taxa defined in the specified file."
    )->check(CLI::ExistingFile)->group("Input");

    auto taxonomy_option = sub->add_option(
        "--taxonomy",
        opt->taxonomy_file,
        "EXPERIMENTAL: File containing a tab-separated list defining the taxonomy."
        " If mapping is incomplete (for example if the output taxonomy shall be NCBI,"
        " but SILVA was used as the basis in the --taxon-file) a best-effort mapping is attempted."
    )->check(CLI::ExistingFile)->group("Input");

    sub->add_option(
        "--ranks-string",
        opt->rank_constraint,
        "String specifying the rank names, in order, to which the taxonomy adheres. Required when using "
        "the CAMI output format. Assignments not adhereing to this constrained will be collapsed to the "
        "last valid mapping\n"
        "EXAMPLE: superkingdom|phylum|class|order|family|genus|species"
    )->group("Input");

    // Add specific options.
    sub->add_option(
        "--sub-taxopath",
        opt->sub_taxopath,
        "Taxopath (example: Eukaryota;Animalia;Chordata) by which the high level summary should be filtered. "
        "Doesn't affect intermediate results, and an unfiltered verison will be printed as well."
    )->group("Settings");

    sub->add_option(
        "--max-level",
        opt->max_tax_level,
        "Maximal level of the taxonomy to be printed. Default is 0, that is, the whole taxonomy "
        "is printed. If set to a value about 0, only this many levels are printed. That is, "
        "taxonomic levels below the specified one are omitted."
    )->group("Settings");

    sub->add_option(
        "--distribution-ratio",
        opt->dist_ratio,
        "Ratio by which LWR is split between annotations if an edge has two possible annotations. "
        "Specifies the amount going to the proximal annotation. If not set program will determine "
        "the ratio automatically from the 'distal length' specified per placement."
    )->check(CLI::Range(0.0,1.0))->group("Settings");

    sub->add_flag(
        "--resolve-missing-paths",
        opt->resolve_missing_labels,
        "Should the taxon file be incomplete and leave some taxa without taxopaths, fill in the "
        "missing node labels using the closest (in the tree) label.\n"
        "If not specified, those parts of the tree remain unlabeled, and their placements unassigned."
    )->group("Settings");

    // Output
    opt->output_dir.add_output_dir_opt_to_app( sub );

    auto cami_flag = sub->add_flag(
        "--cami",
        opt->cami,
        "EXPERIMENTAL: Print result in the CAMI Taxonomic Profiling Output Format."
    )->group("Output")->needs(taxonomy_option);

    sub->add_option(
        "--sample-id",
        opt->sample_id,
        "Sample-ID string to be used in the CAMI output file"
    )->group("Output")->needs(cami_flag);

    sub->add_flag(
        "--krona",
        opt->krona,
        "Print result in the Krona text format."
    )->group("Output");

    sub->add_flag(
        "--sativa",
        opt->sativa,
        "Print result as SATIVA would."
    )->group("Output");

    sub->add_flag(
        "--per-query-results",
        opt->per_query_results,
        "Print intermediate / per-query results (per_query.tsv)."
    )->group("Output");

    sub->add_flag(
        "--best-hit",
        opt->best_hit,
        "In the per-query results, only print the taxonomic path with the highest LWR."
    )->group("Output");


    // Set the run function as callback to be called when this subcommand is issued.
    // Hand over the options by copy, so that their shared ptr stays alive in the lambda.
    sub->callback( gappa_cli_callback(
        sub,
        {},
        [ opt ]() {
            run_assign( *opt );
        }
    ));
}

// =================================================================================================
//      Run
// =================================================================================================

using namespace genesis;
using namespace genesis::placement;
using namespace genesis::tree;
using namespace genesis::taxonomy;
using namespace genesis::utils;

bool equals_closely( std::string const& lhs, std::string const& rhs )
{
    return to_lower( lhs ) == to_lower( rhs );
}

Taxopath intersect( Taxopath const& lhs, Taxopath const& rhs )
{
    Taxopath result;

    for (size_t i = 0; ( i < std::min( lhs.size(), rhs.size() ) ) and ( lhs[i] == rhs[i] ); ++i) {
        result.push_back( lhs[i] );
    }

    // allow the posibility of empty taxopaths, mark them accordingly so that they may be fixed later

    return result;
}

// go through the tree in postorder fashion and label inner nodes according to the most common taxonomic rank of the children
// allow empty labels and propagate them up, however any label is always better than no label
// so intersect should always return the common most specific taxopath, except when one taxopath is fully empty,
// in which case it should take the nonempty one
void postorder_label( PlacementTree const& tree, std::vector<Taxopath>& node_labels )
{
    for ( auto it : postorder( tree ) ) {
        if( is_inner( it.node() )) {
            auto const child_1_idx = it.node().link().next().outer().node().index();
            auto const child_2_idx = it.node().link().next().next().outer().node().index();

            if ( node_labels[ child_1_idx ].empty() ) {
                node_labels[ it.node().index() ] = node_labels[ child_2_idx ];
            } else if ( node_labels[ child_2_idx ].empty() ) {
                node_labels[ it.node().index() ] = node_labels[ child_1_idx ];
            } else {
                node_labels[ it.node().index() ] = intersect( node_labels[ child_1_idx ], node_labels[ child_2_idx ] );
            }
        }
    }
}

void print_labelled( PlacementTree const& tree,
                            std::vector<Taxopath> const& node_labels,
                            std::string const& file_name )
{
    CommonTreeNewickWriter writer;
    writer.node_to_element_plugins.push_back(
        [&]( TreeNode const& node, NewickBrokerElement& element ){
            element.comments.emplace_back(
                TaxopathGenerator().to_string( node_labels[node.index()] )
            );
        }
    );
    writer.to_file( tree, file_name );
}

std::vector<Taxopath> assign_leaf_taxopaths(PlacementTree const& tree,
                                            std::string const& taxon_file)
{
    TaxopathParser tpp;
    CsvReader csv_reader;
    csv_reader.separator_chars( "\t" );
    std::vector<Taxopath> node_labels( tree.node_count() );

    utils::InputStream it( utils::make_unique< utils::FileInputSource >( taxon_file ));
    while (it) {
        auto fields = csv_reader.parse_line( it );

        if ( fields.size() != 2 ) {
            throw std::runtime_error{"A line in the taxon file didn't have two tab separated columns."};
        }

        auto name = fields[0];
        std::string tax_string = fields[1];

        auto node_ptr = find_node( tree, name );

        if ( node_ptr == nullptr ) {
            throw std::runtime_error{"Could not find node with name: " + name};
        }

        node_labels[ node_ptr->index() ] = tpp.parse( tax_string );
    }

    return node_labels;
}

void add_lwr_to_taxonomy(   const double lwr,
                            Taxopath const& path,
                            Taxonomy& taxonomy )
{
    if ( path.empty() ) {
        return;
    }
    auto cur_tax = &add_from_taxopath( taxonomy, path );

    bool first = true;

    do {
        if( not cur_tax->has_data() ) {
            cur_tax->reset_data( AssignTaxonData::create() );
        }

        if ( first ) {
            // add normal elw to this taxon
            cur_tax->data<AssignTaxonData>().LWR += lwr;
            first = false;
        }

        // add accumulated elw up the taxpath
        cur_tax->data<AssignTaxonData>().aLWR += lwr;
        cur_tax = cur_tax->parent();
    } while( cur_tax != nullptr );
}

Taxon const * get_most_supported(Taxonomy const& tax)
{
    Taxon const * most_supported = nullptr;

    // determine which path is the "most supported"
    postorder_for_each( tax, [&]( Taxon const& taxon ){
        // Only print if there is some weight.
        if ( taxon.data<AssignTaxonData>().aLWR == 0 ) {
            return;
        }

        if ( not most_supported ) {
            most_supported = &taxon;
        } else {
            auto cur_max = most_supported->data<AssignTaxonData>().LWR;
            if ( taxon.data<AssignTaxonData>().LWR > cur_max ) {
                most_supported = &taxon;
            }
        }
    });

    return most_supported;
}

void print_weighted_taxopath(
    std::ostream& stream,
    std::string const& name,
    Taxon const& taxon,
    double const sum)
{
    stream << std::setprecision(4);
    if ( not name.empty() ) {
        stream << name << "\t";
    }
    stream << taxon.data<AssignTaxonData>().LWR;
    stream << "\t" << taxon.data<AssignTaxonData>().LWR / sum;
    stream << "\t" << taxon.data<AssignTaxonData>().aLWR;
    stream << "\t" << taxon.data<AssignTaxonData>().aLWR / sum;
    stream << "\t" << TaxopathGenerator().to_string( taxon );
    stream << "\n";
}

void print_taxonomy_with_lwr(
    std::ostream& stream,
    std::string const& name,
    Taxonomy const& tax,
    size_t const base_tax_level,
    AssignOptions const& options
) {
    // get total LWR as sum of all top level aLWR
    double sum = 0.0;
    for( auto const& ct : tax ) {
        sum += ct.data<AssignTaxonData>().aLWR;
    }

    if ( options.best_hit ) {
        Taxon const * most_supported = get_most_supported( tax );
        print_weighted_taxopath(stream, name, *most_supported, sum);
    } else {
        preorder_for_each( tax, [&]( Taxon const& taxon ){
            // Only print if there is some weight.
            if ( taxon.data<AssignTaxonData>().aLWR == 0 ) {
                return;
            }

            // Only print if the level of the taxon is within the max level
            // (if specified by the user), taking care of the base level for subtaxonomies.
            auto const tax_level = taxon_level( taxon ) - base_tax_level;
            if( options.max_tax_level > 0 && tax_level >= options.max_tax_level ) {
                return;
            }

            print_weighted_taxopath(stream, name, taxon, sum);
        });
    }
}

void print_sativa_string(
    std::ostream& stream,
    std::string const& name,
    Taxonomy const& tax
) {
    Taxon const * most_supported = get_most_supported( tax );

    // get that path in full, with the per rank confidences
    std::vector<std::string> taxpath;
    std::vector<double> confidences;

    while ( most_supported ) {
        confidences.push_back( most_supported->data<AssignTaxonData>().aLWR );
        taxpath.emplace_back( most_supported->name() );

        most_supported = most_supported->parent();
    }

    std::reverse( std::begin(confidences), std::end(confidences) );
    std::reverse( std::begin(taxpath), std::end(taxpath) );

    stream << name;
    stream << "\t" << join( taxpath, ";" );
    stream << "\t" << join( confidences, ";" );
    stream << "\n";
}

void print_taxonomy_table(
    AssignOptions const& options,
    size_t const base_tax_level,
    Taxonomy const& tax,
    std::string const& path
) {
    std::ofstream stream;
    genesis::utils::file_output_stream( path, stream );

    stream << "LWR\tfract\taLWR\tafract\ttaxopath\n";
    print_taxonomy_with_lwr( stream, "", tax, base_tax_level, options );
}

using vec_iter = std::vector<std::string>::const_iterator;

/**
 * Inserts as many taxons between first and last as specified by the rank name range [rank_first, rank end).
 * Updates the 'last' pointer to point to the new
 */
Taxon* insert_between(
    Taxon* first,
    Taxon* last,
    vec_iter const rank_first,
    vec_iter const rank_end,
    Taxon const* map_entry
) {
    assert( std::distance(rank_first, rank_end) > 0 );
    assert( first );
    assert( last );
    assert( map_entry );

    // ensure map_entry starts at parent-equvalent (in the map) of 'last'
    assert( equals_closely( last->name(), map_entry->name() ) );
    assert( map_entry->parent() );
    map_entry = map_entry->parent();

    // generate a list of Taxons to be inserted
    auto first_id = first->id();
    std::vector<Taxon> to_insert;
    do {
        auto& entry = *map_entry;
        // only add taxa that conform to the rank constraint
        auto found = std::find( rank_first, rank_end, entry.rank() );
        if ( found != rank_end ) {
            to_insert.emplace_back(
                Taxon( entry.name(), entry.rank(), entry.id() ).reset_data( AssignTaxonData::create() )
            );
        }
        map_entry = entry.parent();
    } while ( map_entry and map_entry->id() != first_id );

    // go through the list of taxons to add in reverse order
    Taxon* running = first;
    if ( not to_insert.empty() ) {
        for ( auto it = to_insert.rbegin(); it != to_insert.rend(); ++it ) {
            LOG_MSG3 << "Inserting '" << it->name() << "' ('" << it->rank() << "', " << it->id() << ")";
            running = &( running->add_child( *it ) );
        }

        // finally, set parent pointer of 'last' to the last added empty Taxon
        auto new_last = &( running->add_child( *last ) );

        // delete 'last' taxon as it is now copied into the taxonomy as a new sub tree
        last->parent()->remove_child( last->name() );

        // last = new_last;

        return new_last;
    } else {
        return last;
    }
}

void transfer_lwr( Taxon* source, Taxon* dest ) {
    assert( source );
    assert( source->data_ptr() );
    if ( not dest ) {
        throw std::runtime_error{"No last successful match to assign LWR to. (taxopath and Taxonomy"
                                " fundamentally incompatible?)"};
    }
    assert( dest->data_ptr() );


    if ( source->data_ptr() == nullptr or dest->data_ptr() == nullptr ) {
        throw std::runtime_error{"bad data pointer"};
    }

    auto& dd = dest->data<AssignTaxonData>();
    auto& sd = source->data<AssignTaxonData>();
    dd.LWR += sd.LWR;

    // (aLWR already accounted for by definition)

    // reset the old
    sd.LWR = 0.0;
    sd.aLWR = 0.0;
}

/**
 * Prune a Taxon from the taxonomy, transferring its children to the parent.
 *
 * returns a pointer to the parent
 */
Taxon* prune( Taxon* const taxon )
{
    auto parent = taxon->parent();
    assert( parent );
    // if we are at the top.. well fuck, throw for now
    assert( taxon->parent() );

    LOG_MSG3 << "Pruning '" << taxon->name() << "' to '" << parent->name() << "' (" << parent->rank() << ")";

    // get the index of the taxon to prune
    auto const remove_index = parent->index_of( taxon->name() );

    // transfer the children
    for ( auto& child : *taxon ) {
        parent->add_child( child, false );
    }

    // remove the taxon from parent
    parent->remove_at( remove_index );

    return parent;
}

/**
 * Maps the given Taxon and its predeccesors according to the given Taxonomy.
 *
 * Here, mapping means assigning the appropriate rank name and ID when a Taxon matches.
 * If no match is found, a taxon has its LWR/aLWR transferred to the last parent to still be mapped
 * successfully.
 *
 */
Taxon const* map_according_to( Taxonomy const& map, Taxon& taxon, std::string const& rank_constraint )
{
    // short cuircuit if Taxon already mapped
    if( not taxon.id().empty() ) {
        LOG_MSG3 << "Already Mapped!";
        return &taxon;
    }

    // make rank contraint into a vector
    auto const valid_ranks = split( rank_constraint, "|" );
    auto rank_iter = valid_ranks.begin();
    auto const rank_end = valid_ranks.end();

    // pointer to the last successfully mapped Taxon
    Taxon* last_success = nullptr;
    std::vector<int> taxon_list;

    // Go up the Taxon-chain to the top or the last successful mapping
    Taxon* cur_taxon = &taxon;
    // signal the end
    taxon_list.emplace_back(-1);
    for (; cur_taxon->parent() and cur_taxon->parent()->id().empty(); cur_taxon = cur_taxon->parent() ) {
        // add to a taxon-pointer-list while doing so
        taxon_list.emplace_back( cur_taxon->data<AssignTaxonData>().tmp_id );
    }

    // if we stopped before the top that means we have a last_success
    if ( cur_taxon->parent() ) {
        last_success = cur_taxon->parent();

        // make sure we start the valid rank stuff accordingly
        rank_iter = std::find( rank_iter, rank_end, last_success->rank() );
        if ( rank_iter == rank_end ) {
            throw std::runtime_error{std::string()
                + "last_success somehow did not have a valid rank! last_success->rank(): "
                + last_success->rank()
            };
        }
    }

    // then go through the taxon_list in reverse order
    // Use a pointer that is updated in the loop while we go deeper and deeper into the mapping taxonomy.
    Taxonomy const* cur_ref_taxon = &map;
    for (auto it = taxon_list.rbegin(); it != taxon_list.rend(); ++it) {
        bool do_mapping = true;
        // find current taxon in map
        auto const entry = find_taxon( *cur_ref_taxon, [&cur_taxon](Taxon const& other) {
            return equals_closely( cur_taxon->name(), other.name() );
        });

        if ( entry ) {
        // success:
            // check rank name validities
            if ( entry->rank() != *rank_iter ) {
                // did we perhaps skip some?
                auto found_rank = std::find( rank_iter, rank_end, entry->rank() );
                if ( found_rank != rank_end ) {
                    // looks like we skipped some, so lets insert some empty Taxon between last success and here
                    LOG_MSG3 << "Inserting " << std::distance(rank_iter, found_rank) << " ranks between '"
                             << last_success->name() << "' (" << last_success->rank() << ") and '"
                             << cur_taxon->name()  << "' (" << cur_taxon->rank()  << ")";

                    cur_taxon = insert_between( last_success, cur_taxon, rank_iter, found_rank, entry );
                    rank_iter = found_rank;
                } else {
                    // nope, this entries rank just doesnt make any sense according to the constraint
                    // so lets transfer its LWR to the last succesful rank
                    LOG_MSG3 << "Transferring LWR from '" << cur_taxon->name() << "' to '"
                             << last_success->name() << "', because rank '"
                             << entry->rank() << "' is outside of the constraint.";

                    transfer_lwr( cur_taxon, last_success );
                    // not just do we need to skip, we also need to prune this taxon
                    cur_taxon = prune( cur_taxon );
                    // skip the actual ID assignment
                    do_mapping = false;
                }
            }

            if ( do_mapping ) {
                LOG_MSG3 << "Mapping '" << cur_taxon->name() << "' to '" << entry->name()
                         << "' (" << entry->rank() << ")";

                // copy over rank name and ID
                cur_taxon->id( entry->id() );
                cur_taxon->rank( entry->rank() );
                // need to take the new name as well, as we do a ignore-case search
                cur_taxon->name( entry->name() );
                // update last_success to this Taxon
                last_success = cur_taxon;
                // update rolling ref taxonomy ptr to the entry
                cur_ref_taxon = entry;
                // iterate valid rank
                ++rank_iter;
            }
        } else {
        // failure:
            // transfer lwr to last_success
            LOG_MSG3 << "Transferring LWR from '" << cur_taxon->name() << "' to '"
                     << last_success->name() << "'";
            transfer_lwr( cur_taxon, last_success );
            cur_taxon = prune( cur_taxon );
        }

        if ( *it >= 0 ) {
            // we have to get the next iterations pointer from the current,
            // as the structure may have changed! (in the insert_between case)
            cur_taxon = find_taxon(*cur_taxon, [&it](Taxon const& t){
                return t.data<AssignTaxonData>().tmp_id == *it;
            }, BreadthFirstSearch{} );
            assert( cur_taxon );
        }
    }
    return cur_taxon;
}

/**
 * Adds Taxonomic IDs to the taxopaths according to the Taxonomy file
 * @param tax     Taxonomy structure to be updated
 * @param options options object
 */
void add_taxon_ids( Taxonomy& tax, AssignOptions const& options )
{
    // load taxonomy tsv into internal
    Taxonomy map;
    auto tr = TaxonomyReader();
    tr.id_field_position(1);
    tr.rank_field_position(2);
    tr.read( genesis::utils::from_file( options.taxonomy_file ), map );

    /* dirty hack time! yaay
    *  since we will have a very hard time changing the taxonomy while traversing it
    *  my dirty solution is to first give the leaves of the taxonomy unique IDs
    *  based on their traversal. Then we iterate over these IDs, using them to
    *  get the Taxons in-order, always from the currently 'fresh' taxonomy.
    */

    // set temporary, unique ids
    int tmp_id = 0;
    postorder_for_each( tax, [&tmp_id]( Taxon& taxon ) {
        taxon.data<AssignTaxonData>().tmp_id = tmp_id++;
    }, true); // true means also traverse inner taxa

    // map all taxa
    // LOG_MSG3 << "ROOT NAME: " << dynamic_cast<Taxon*>(&tax)->name();
    for (int i = 0; i < tmp_id; ++i) {
        auto taxon = find_taxon( tax, [&i](Taxon const& other) {
            return other.data<AssignTaxonData>().tmp_id == i;
        });

        // its possible that we don't find the taxon since it may have been pruned
        if ( taxon ) {
            LOG_MSG3 << "== trying to map " << taxon->name() << " ==";
            map_according_to( map, *taxon, options.rank_constraint );
        }
    }
}

std::string get_rank_string( Taxonomy const& tax )
{
    std::vector<std::string> ranks;
    preorder_for_each( tax, [&]( Taxon const& taxon ) {
        auto const level = taxon_level( taxon );

        // Add missing ranks.
        for (size_t i = ranks.size(); i <= level; ++i) {
            ranks.push_back("");
        }

        // Check consistency
        if ( not ranks[level].empty() and ranks[level] != taxon.rank() ) {
            throw std::runtime_error{
                std::string() +
                "Taxonomy has internally inconsistent taxonomic rank annotations. " +
                "ranks["+std::to_string(level)+"]: " + ranks[level] +
                " |vs| taxon.rank(): " + taxon.rank() +
                "\nCulprit: " + TaxopathGenerator().to_string( taxon )
            };
        }

        // Add level
        ranks[level] = taxon.rank();
    });

    return join(ranks, "|");
}

void print_cami(
    AssignOptions const& options,
    Taxonomy const& tax,
    std::string const& path
) {
    std::ofstream stream;
    genesis::utils::file_output_stream( path, stream );

    // taxopath generator
    auto gen = TaxopathGenerator().delimiter("|");
    using Field = TaxopathGenerator::TaxonField;

    // Print the header
    stream << "@SampleID: " << options.sample_id << "\n"; // TODO: sampleid based on file name?
    stream << "@Version:0.9.3\n";
    stream << "@Ranks:superkingdom|phylum|class|order|family|genus|species" << "\n";

    // data section
    stream << "@@TAXID\tRANK\tTAXPATH\tTAXPATHSN\tPERCENTAGE\n";

    // get total LWR as sum of all top level aLWR
    double sum = 0.0;
    for( auto const& ct : tax ) {
        sum += ct.data<AssignTaxonData>().aLWR;
    }

    preorder_for_each( tax, [&]( Taxon const& taxon ){
        // Only print if there is some weight.
        if ( taxon.data<AssignTaxonData>().aLWR == 0 ) {
            return;
        }

        // Only print if the level of the taxon is within the max level (if specified by the user).
        if( options.max_tax_level > 0 && taxon_level( taxon ) >= options.max_tax_level ) {
            return;
        }

        stream << std::setprecision(4);
        stream << taxon.id();                                                     // TAXID
        stream << "\t" << taxon.rank();                                           // RANK
        stream << "\t" << gen.field(Field::kId).to_string( taxon );               // TAXPATH
        stream << "\t" << gen.field(Field::kName).to_string( taxon );             // TAXPATHSN
        stream << "\t" << ( taxon.data<AssignTaxonData>().aLWR / sum ) * 100.0;   // PERCENTAGE
        stream << "\n";
    });
}

void print_krona(
    AssignOptions const& options,
    Taxonomy const& tax,
    std::string const& path
) {
    std::ofstream stream;
    genesis::utils::file_output_stream( path, stream );
    stream << std::setprecision(4);

    // taxopath generator
    auto gen = TaxopathGenerator().delimiter("\t");

    preorder_for_each( tax, [&]( Taxon const& taxon ){
        // Only print if there is some weight.
        if ( taxon.data<AssignTaxonData>().aLWR == 0 ) {
            return;
        }

        // Only print if the level of the taxon is within the max level (if specified by the user).
        if( options.max_tax_level > 0 && taxon_level( taxon ) >= options.max_tax_level ) {
            return;
        }

        stream << taxon.data<AssignTaxonData>().LWR;
        stream << "\t" << gen.to_string( taxon );
        stream << "\n";
    });
}

Taxon& get_subtaxonomy( Taxonomy tax, AssignOptions const& options )
{
    // This function is only called if the option for sub tax is spefied.
    assert( ! options.sub_taxopath.empty() );

    auto const taxopath = TaxopathParser().parse( options.sub_taxopath );
    auto const subtax = find_taxon_by_taxopath( tax, taxopath );

    if( subtax == nullptr ) {
        throw std::runtime_error(
            "Taxon " + options.sub_taxopath + " not found in the taxonomy."
        );
    }

    assert( subtax != nullptr );
    return *subtax;
}

static void assign( Sample const& sample,
                    std::vector<Taxopath> const& node_labels,
                    AssignOptions const& options,
                    std::string per_pquery_result_file )
{
    bool    const   auto_ratio = ( options.dist_ratio < 0.0 );
    double  const   dist_ratio = options.dist_ratio;
    assert( auto_ratio or ( dist_ratio >= 0.0 and dist_ratio <= 1.0 ) );

    auto const& tree = sample.tree();

    Taxonomy diversity;

    std::ofstream per_pquery_out_stream;
    bool const per_query_results = options.per_query_results;

    if ( per_query_results ) {
        genesis::utils::file_output_stream( per_pquery_result_file, per_pquery_out_stream );
        per_pquery_out_stream << "name\tLWR\tfract\taLWR\tafract\ttaxopath\n";
    }

    std::ofstream sativa_out_stream;
    if ( options.sativa ) {
        genesis::utils::file_output_stream( options.output_dir.out_dir() + "sativa.tsv", sativa_out_stream );
    }

    for ( auto const& pq : sample.pqueries() ) {
        Taxonomy per_pq_assignments;

        using PqueryName = genesis::placement::PqueryName;

        // take the multiplicity of a PQuery as the sum of all named multiplicites within it
        auto const multiplicity = std::accumulate(  pq.begin_names(),
                                                    pq.end_names(),
                                                    PqueryName("", 0),
            []( const PqueryName& a, const PqueryName& b ){
                PqueryName ret;
                ret.multiplicity = a.multiplicity + b.multiplicity;
                return ret;
            }
        ).multiplicity;


        for ( auto const& p : pq.placements() ) {
            // scale the LWR by the multiplicity
            auto const lwr = p.like_weight_ratio * multiplicity;
            // get its adjacent nodes
            auto const& edge = tree.edge_at( p.edge().index() );
            auto const& proximal_node   = edge.primary_node();
            auto const& distal_node     = edge.secondary_node();

            // get the taxopaths
            auto const& proximal_tax    = node_labels[ proximal_node.index() ];
            auto const& distal_tax      = node_labels[ distal_node.index() ];

            double ratio = dist_ratio;
            // determine the ratio
            if ( auto_ratio ) {
                auto const position         = p.proximal_length;
                auto const branch_length    = edge.data<CommonEdgeData>().branch_length;
                // in percent, how far toward the distal are we?
                auto const toward_distal    = (1.0 / branch_length) * position;
                // the ratio is effectively "how much lwr mass should go toward the PROXIMAL", so we need to flip it
                ratio = 1.0 - toward_distal;

                // guarding against improperly rounded inputs
                ratio = std::min(ratio, 1.0);
                ratio = std::max(ratio, 0.0);

                assert(ratio >= 0.0);
                assert(ratio <= 1.0);
            }

            // calculate lwr portions
            auto proximal_portion   = lwr * ratio;
            auto distal_portion     = lwr * (1.0 - ratio);

            assert(proximal_portion >= 0.0);
            assert(distal_portion >= 0.0);


            // add LW to taxopaths of the nodes according to strategy
            // first to the local one
            if ( per_query_results ) {
                add_lwr_to_taxonomy( proximal_portion,  proximal_tax,   per_pq_assignments );
                add_lwr_to_taxonomy( distal_portion,    distal_tax,     per_pq_assignments );
            }

            // then to the global one
            add_lwr_to_taxonomy( proximal_portion, proximal_tax, diversity );
            add_lwr_to_taxonomy( distal_portion, distal_tax, diversity );
        }

        if ( per_query_results ) {
            std::string composite_name;
            for ( auto const& name : pq.names() ) {
                if ( not composite_name.empty() ) {
                    composite_name += ";";
                }
                composite_name += name;
            }
            print_taxonomy_with_lwr(per_pquery_out_stream,
                                    composite_name,
                                    per_pq_assignments,
                                    0,
                                    options );

            if ( options.sativa ) {
                print_sativa_string( sativa_out_stream, composite_name, per_pq_assignments );
            }
        }
    }

    // if specified, use the taxonomy table to label the taxopaths according to their tax IDs
    if ( not options.taxonomy_file.empty() ) {
        add_taxon_ids( diversity, options );
    }

    // ========= OUTPUT =============

    std::string out_dir = options.output_dir.out_dir();

    // return diversity profile
    print_taxonomy_table( options, 0, diversity, out_dir + "profile.tsv" );

    // print result in CAMI format if desired
    if ( options.cami ) {
        print_cami( options, diversity, out_dir + "cami.profile" );
    }

    // print result in krona format if desired
    if ( options.krona ) {
        print_krona( options, diversity, out_dir + "krona.profile" );
    }

    // constrain to subtaxonomy if specified
    if ( not options.sub_taxopath.empty() ) {
        const auto& subtaxonomy = get_subtaxonomy( diversity, options );

        // Get the level of the taxon to be printed.
        // Need this for the max level filter.
        auto const base_level = taxon_level( subtaxonomy );

        // and print to file
        print_taxonomy_table( options, base_level, subtaxonomy, out_dir + "profile_filtered.tsv" );
    }
}

TreeEdge* lowest_common_ancestor( Tree& tree, std::vector<TreeNode const*>& nodes )
{
    assert( not nodes.empty() );

    auto bipart = find_smallest_subtree( tree, bipartition_set( tree ), nodes );

    if ( bipart.empty() ) {
        throw std::invalid_argument{"Rooting could not be determined."};
    }

    return const_cast<TreeEdge*>( &bipart.link().edge() );

}

void outgroup_rooting(  Sample& sample,
                        std::vector<std::string> const& outgroup_names )
{
    // find MRCA edge containing all matched to the outgroup pattern
    auto& tree = sample.tree();
    std::vector<PlacementTreeNode const*> nodes;
    for ( auto& name : outgroup_names ) {
        auto node_ptr = find_node( tree, name );

        if ( node_ptr == nullptr ) {
            throw std::invalid_argument{name + " was not found in the tree!"};
        }

        nodes.push_back( node_ptr );
    }

    PlacementTreeEdge* edge_ptr = nullptr;

    if ( nodes.size() == 0 ) {
        throw std::invalid_argument{"Outgroup file didn't contain any valid taxa."};
    } else if ( nodes.size() == 1 ) {
        edge_ptr = const_cast<PlacementTreeEdge*>(&( nodes[0]->primary_link().edge() ));
    } else {
        edge_ptr = lowest_common_ancestor( tree, nodes );
    }

    assert( edge_ptr );

    // root on that edge
    make_rooted( sample, *edge_ptr );
}

// Label undetermined nodes by passing the closest taxopath down
void label_undetermined_nodes( PlacementTree const& tree, std::vector<Taxopath>& node_labels ) {
    for ( size_t node_id = 0; node_id < node_labels.size(); ++node_id ) {
        auto& taxopath = node_labels[ node_id ];
        if ( taxopath.empty() ) {
            // found an undetermined node label!
            // travel up the tree until a node has a taxopath
            size_t cur_node_id = node_id;
            std::vector<size_t> nodes_to_fix;
            while ( node_labels[ cur_node_id ].empty() ) {
                // unsolvable if root is unassigned
                if ( is_root( tree.node_at( cur_node_id ) ) ) {
                    throw std::runtime_error{"Cannot resolve taxonomic assignment of unassigned taxa as the root node seems to be unassigned"};
                }

                // track that this node needs to be assigned a proper taxopath
                nodes_to_fix.push_back( cur_node_id );

                // iterate by going to the parent node
                cur_node_id = tree.node_at( cur_node_id ).link().outer().node().index();
            }

            // cur_node_id's node now has an actual taxopath
            auto const& closest_taxopath = node_labels[ cur_node_id ];
            // apply that label to all nodes found to be lacking
            for ( auto const& fix_id : nodes_to_fix ) {
                node_labels[ fix_id ] = closest_taxopath;
            }
        }
    }
}

void run_assign( AssignOptions const& options )
{
    auto out_dir = options.output_dir.out_dir();

    options.jplace_input.print();
    auto sample = options.jplace_input.merged_samples();
    auto& tree = sample.tree();

    if ( not is_bifurcating(tree) ) {
        throw std::runtime_error{"Supplied tree is not bifurcating."};
    }

    // User output.
    LOG_MSG1 << "Running the assignment";

    // root the tree if necessary
    if ( not options.outgroup_file.empty() ) {
        // get the names of the outgroup taxa
        std::vector<std::string> names;
        std::ifstream outgroup_file( options.outgroup_file );
        std::copy(std::istream_iterator<std::string>(outgroup_file),
            std::istream_iterator<std::string>(),
            std::back_inserter(names));
        // root
        outgroup_rooting(sample, names);
    }

    // vector to hold the per node taxopaths
    // fill the per node taxon assignments
    auto node_labels = assign_leaf_taxopaths(tree, options.taxon_map_file);

    // assign taxpaths to inner nodes
    postorder_label( tree, node_labels );

    // label those leaves that didn't come with a taxonomic path assignment
    if ( options.resolve_missing_labels ) {
        label_undetermined_nodes( tree, node_labels );
    }

    // print taxonomically labelled tree as intermediate result
    print_labelled( tree, node_labels, out_dir + "labelled_tree.newick" );

    // per rank LWR score eval
    assign( sample, node_labels, options, out_dir + "per_query.tsv" );
}
