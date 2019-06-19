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

#include "tools/references.hpp"

#include "genesis/utils/text/string.hpp"

#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

// =================================================================================================
//      Citations
// =================================================================================================

// -------------------------------------------------------------------------
//     List of References
// -------------------------------------------------------------------------

struct Citation
{
    struct Author
    {
        std::string first;
        std::string last;
    };

    std::vector<Author> authors;
    std::string title;
    std::string journal;
    std::string volume;
    std::string issue;
    std::string year;
    std::string doi;
};

static std::unordered_map<std::string, Citation> citations_ = {
    { "Czech2018-phat-and-multilevel-placement", {
        {{ "Lucas", "Czech" }, { "Pierre", "Barbera" }, { "Alexandros", "Stamatakis" }},
        "Methods for Automatic Reference Trees and Multilevel Phylogenetic Placement",
        "Bioinformatics",
        "",
        "",
        "2018",
        "10.1093/bioinformatics/bty767"
    }},
    { "Czech2019-analyzing-and-visualizing-samples", {
        {{ "Lucas", "Czech" }, { "Alexandros", "Stamatakis" }},
        "Scalable Methods for Analyzing and Visualizing Phylogenetic Placement of Metagenomic Samples",
        "PLOS ONE",
        "",
        "",
        "2019",
        "10.1371/journal.pone.0217050"
    }},
    { "Czech2019-genesis-and-gappa", {
        {{ "Lucas", "Czech" }, { "Pierre", "Barbera" }, { "Alexandros", "Stamatakis" }},
        "Genesis and Gappa: Processing, Analyzing and Visualizing Phylogenetic (Placement) Data",
        "bioRxiv",
        "",
        "",
        "2019",
        "10.1101/647958"
    }},
    { "Matsen2011-edgepca-and-squash-clustering", {
        {{ "Frederick", "Matsen" }, { "Steven", "Evans" }},
        "Edge Principal Components and Squash Clustering: Using the Special Structure of Phylogenetic Placement Data for Sample Comparison",
        "PLOS ONE",
        "",
        "",
        "2013",
        "10.1371/journal.pone.0056859"
    }},
    { "Evans2012-kr-distance", {
        {{ "Steven", "Evans" }, { "Frederick", "Matsen" }},
        "The phylogenetic Kantorovich-Rubinstein metric for environmental sequence samples",
        "Journal of the Royal Statistical Society",
        "",
        "",
        "2012",
        "10.1111/j.1467-9868.2011.01018.x"
    }},
    { "Washburne2017-phylofactorization", {
        {
            { "Alex", "Washburne" }, { "Justin", "Silverman" }, { "Jonathan", "Leff" },
            { "Dominic", "Bennett" }, { "John", "Darcy" }, { "Sayan", "Mukherjee" },
            { "Noah", "Fierer" }, { "Lawrence", "David" }
        },
        "Phylogenetic Factorization of Compositional Data Yields Lineage-Level Associations in Microbiome Datasets",
        "PeerJ",
        "",
        "",
        "2017",
        "10.7717/peerj.2969"
    }}
};

// -------------------------------------------------------------------------
//     Helper Functions
// -------------------------------------------------------------------------

void check_citation_duplicates( std::vector<std::string> keys )
{
    std::sort( keys.begin(), keys.end() );
    auto uniq = std::unique( keys.begin(), keys.end() );
    if( uniq != keys.end() ) {
        throw std::runtime_error( "Duplicate citation keys: " + (*uniq) );
    }
}

Citation const& get_citation( std::string const& key )
{
    if( citations_.count(key) == 0 ) {
        throw std::runtime_error( "Invalid citation key: " + key );
    }
    auto const& entry = citations_.at(key);

    if(
        entry.authors.empty() || entry.title.empty() ||
        entry.journal.empty() || entry.year.empty() || entry.doi.empty()
    ) {
        throw std::runtime_error( "Citation is missing some information: " + key );
    }
    for( auto const& author : entry.authors ) {
        if( author.first.empty() || author.last.empty() ) {
            throw std::runtime_error( "Citation is missing author information: " + key );
        }
    }

    return entry;
}

std::string cite_authors( Citation const& entry, bool first_last, std::string const& delim )
{
    std::vector<std::string> authors;
    for( auto const& author : entry.authors ) {
        if( first_last ) {
            authors.push_back( author.first + " " + author.last );
        } else {
            authors.push_back( author.last + ", " + author.first );
        }
    }
    return genesis::utils::join( authors, delim );
}

void check_all_citations()
{
    // Simply retrieve all citations, which triggers their validity checks.
    for( auto const& entry : citations_ ) {
        (void) get_citation( entry.first );
    }
}

void check_citation( std::string const& key )
{
    // Simply retrieve the citation, which triggers its validity checks.
    (void) get_citation( key );
}

void check_citations( std::vector<std::string> const& keys )
{
    // Simply retrieve all citations, which triggers their validity checks.
    for( auto const& entry : keys ) {
        (void) get_citation( entry );
    }

    check_citation_duplicates( keys );
}

std::vector<std::string> get_all_citation_keys()
{
    std::vector<std::string> result;
    for( auto const& entry : citations_ ) {
        result.push_back( entry.first );
    }
    std::sort( result.begin(), result.end() );
    return result;
}

// -------------------------------------------------------------------------
//     Run Functions
// -------------------------------------------------------------------------

std::string cite_bibtex( std::string const& key )
{
    auto const& entry = get_citation( key );

    std::stringstream ss;
    ss << "@article{" << key << ",\n";
    ss << "    author = {" << cite_authors( entry, false, " and " ) << "},\n";
    ss << "    title = {{" << entry.title << "}},\n";
    ss << "    journal = {" << entry.journal << "},\n";
    ss << "    year = {" << entry.year << "},\n";
    if( ! entry.volume.empty() ) {
        ss << "    volume = {" << entry.volume << "},\n";
    }
    if( ! entry.issue.empty() ) {
        ss << "    number = {" << entry.issue << "},\n";
    }
    ss << "    doi = {" << entry.doi << "}\n";
    ss << "}\n";
    return ss.str();
}

std::string cite_markdown( std::string const& key, bool with_quote_block, bool with_key )
{
    auto const& entry = get_citation( key );
    std::string const in = ( with_quote_block ? "> " : "" );

    std::stringstream ss;
    if( with_key ) {
        ss << key << ":\n";
    }
    ss << in << cite_authors( entry, true, ", " ) << ".\n";
    ss << in << "**" << entry.title << ".**\n";
    ss << in << "*" << entry.journal << "*";
    if( ! entry.volume.empty() ) {
        ss << ", vol. " << entry.volume;
    }
    if( ! entry.issue.empty() ) {
        ss << ", no. " << entry.issue;
    }
    ss << ", " << entry.year << ".\n";
    ss << in << "doi:[" << entry.doi << "](https://doi.org/" << entry.doi << ")\n";
    return ss.str();
}

std::string cite_bibtex( std::vector<std::string> const& keys )
{
    check_citation_duplicates( keys );

    std::string result;
    for( auto const& key : keys ) {
        if( &key != &keys[0] ) {
            result += "\n";
        }
        result += cite_bibtex( key );
    }
    return result;
}

std::string cite_markdown( std::vector<std::string> const& keys, bool with_quote_block, bool with_key )
{
    check_citation_duplicates( keys );

    std::string result;
    for( auto const& key : keys ) {
        if( &key != &keys[0] ) {
            result += "\n";
        }
        result += cite_markdown( key, with_quote_block, with_key );
    }
    return result;
}
