#ifndef GAPPA_COMMANDS_EXAMINE_ASSIGN_H_
#define GAPPA_COMMANDS_EXAMINE_ASSIGN_H_

/*
    gappa - Genesis Applications for Phylogenetic Placement Analysis
    Copyright (C) 2017-2020 Pierre Barbera, Lucas Czech and HITS gGmbH

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

#include "CLI/CLI.hpp"

#include "options/jplace_input.hpp"
#include "options/file_output.hpp"

#include "genesis/taxonomy/taxon_data.hpp"

#include <string>
#include <vector>

// =================================================================================================
//      Options
// =================================================================================================

class AssignOptions
{
public:

    std::string         taxon_map_file;
    std::string         taxonomy_file;
    std::string         outgroup_file;
    std::string         rank_constraint = "superkingdom|phylum|class|order|family|genus|species";
    std::string         sub_taxopath;
    size_t              max_tax_level;
    JplaceInputOptions  jplace_input;

    double              dist_ratio          = -1.0;
    double              consensus_threshold = 1.0;

    FileOutputOptions   file_output;

    bool                cami    = false;
    bool                krona   = false;
    bool                sativa  = false;
    bool                best_hit= false;
    bool                resolve_missing_labels = false;
    bool                per_query_results = false;
    bool                distant_label = false;

    std::string         sample_id = "";
};

// =================================================================================================
//      Functions
// =================================================================================================

void setup_assign( CLI::App& app );
void run_assign( AssignOptions const& options );

#endif // include guard
