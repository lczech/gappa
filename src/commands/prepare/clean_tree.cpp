/*
    gappa - Genesis Applications for Phylogenetic Placement Analysis
    Copyright (C) 2017-2022 Lucas Czech

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
    Lucas Czech <lczech@carnegiescience.edu>
    Department of Plant Biology, Carnegie Institution For Science
    260 Panama Street, Stanford, CA 94305, USA
*/

#include "commands/prepare/clean_tree.hpp"

#include "options/global.hpp"
#include "tools/cli_setup.hpp"

#include "CLI/CLI.hpp"

#include "genesis/tree/formats/newick/simple_reader.hpp"
#include "genesis/tree/formats/newick/simple_tree.hpp"
#include "genesis/tree/formats/newick/simple_writer.hpp"
#include "genesis/tree/function/functions.hpp"

#include "genesis/utils/core/fs.hpp"

#include <cassert>
#include <string>
#include <stdexcept>

// =================================================================================================
//      Setup
// =================================================================================================

void setup_clean_tree( CLI::App& app )
{
    // Create the options and subcommand objects.
    auto opt = std::make_shared<CleanTreeOptions>();
    auto sub = app.add_subcommand(
        "clean-tree",
        "Clean a tree in Newick format by removing parts that other parsers have difficulties with."
    );

    // -----------------------------------------------------------
    //     Input Data
    // -----------------------------------------------------------

    // Tree file
    auto tree_file_opt = sub->add_option(
        "--tree-file",
        opt->tree_file,
        "Tree file in Newick format."
    );
    tree_file_opt->check( CLI::ExistingFile );
    tree_file_opt->group( "Input" );
    tree_file_opt->required();

    // -----------------------------------------------------------
    //     Settings
    // -----------------------------------------------------------

    // remove inner labels
    sub->add_flag(
        "--remove-inner-labels",
        opt->remove_inner_labels,
        "Some Newick trees contain inner node labels, which can confuse some parsers. "
        "This option removes them."
    )->group( "Settings" );

    // replace invalid chars
    sub->add_flag(
        "--replace-invalid-chars",
        opt->replace_invalid_chars,
        "Replace invalid characters in node labels (` ,:;\"()[]`) by underscores. "
        "The Newick format requires node labels to be wrapped in double quotation marks "
        "if they contain these characters, but many parsers cannot handle this. "
        "For such cases, replacing the characters can help."
    )->group( "Settings" );

    // remove comments and nhx
    sub->add_flag(
        "--remove-comments-and-nhx",
        opt->remove_comments_and_nhx,
        "The Newick format allows for comments in square brackets `[]`, "
        "which are also often (mis-)used for ad-hoc and more established extensions such as the "
        "New Hampshire eXtended (NHX) format `[&&NHX:key=value:...]`. "
        "Many parsers cannot handle this; this option removes such annotations."
    )->group( "Settings" );

    // remove extra numbers
    sub->add_flag(
        "--remove-extra-numbers",
        opt->remove_extra_numbers,
        "The Rich/Rice Newick format extension allows to annotate bootstrap values and probabilities "
        "per branch, by adding additional `:[bootstrap]:[prob]` fields after the branch length. "
        "Many parsers cannot handle this; this option removes such annotations."
    )->group( "Settings" );

    // remove jplace tags
    sub->add_flag(
        "--remove-jplace-tags",
        opt->remove_jplace_tags,
        "The Jplace file format for phylogenetic placements also uses a custom Newick extension, "
        "by introducing curly brackets to annotate edge numbers in the tree `{1}`. "
        "We are not aware of any other Newick extension that uses this style, "
        "but still, with this option, all annotations in curly brackets is removed."
    )->group( "Settings" );

    // -----------------------------------------------------------
    //     Output Options
    // -----------------------------------------------------------

    opt->file_output.add_default_output_opts_to_app( sub );

    // -----------------------------------------------------------
    //     Callback
    // -----------------------------------------------------------

    // Set the run function as callback to be called when this subcommand is issued.
    // Hand over the options by copy, so that their shared ptr stays alive in the lambda.
    sub->callback( gappa_cli_callback(
        sub,
        {},
        [ opt ]() {
            run_clean_tree( *opt );
        }
    ));
}

// =================================================================================================
//      Run
// =================================================================================================

void run_clean_tree( CleanTreeOptions const& options )
{
    using namespace ::genesis;
    using namespace ::genesis::tree;
    using namespace ::genesis::utils;

    // Check if the output file name already exists. If so, fail early.
    options.file_output.check_output_files_nonexistence( "clean-tree", "newick" );

    // If clean is given, read it.
    LOG_MSG1 << "Reading input tree.";
    auto tree = SimpleNewickTreeNewickReader().read( from_file( options.tree_file ));
    LOG_MSG1 << "Tree contains " << leaf_node_count( tree ) << " taxa (terminal branches).";

    // We want to warn the user if no cleaning option was provided.
    bool ran_one = false;

    // Remove inner labels
    if( options.remove_inner_labels ) {
        size_t cnt = 0;
        for( auto& node : tree.nodes() ) {
            auto& data = node.data<CommonNodeData>();
            if( is_inner( node ) && ! data.name.empty() ) {
                data.name = "";
                ++cnt;
            }
        }
        LOG_MSG1 << "Removed " << cnt << " inner node labels.";
        ran_one = true;
    }

    // Replace invalid chars
    if( options.replace_invalid_chars ) {
        auto is_valid_name_char = [&]( char c ){
            return   ::isprint(c)
            && ! ::isspace(c)
            && c != ':'
            && c != ';'
            && c != '('
            && c != ')'
            && c != '['
            && c != ']'
            && c != ','
            && c != '"'
            ;
        };
        size_t cnt = 0;
        for( auto& node : tree.nodes() ) {
            auto& name = node.data<CommonNodeData>().name;
            bool valid_name = true;
            for( size_t i = 0; i < name.size(); ++i ) {
                if( ! is_valid_name_char( name[i] ) ) {
                    valid_name = false;
                    name[i] = '_';
                }
            }
            if( !valid_name ) {
                ++cnt;
            }
        }
        LOG_MSG1 << "Replaced invalid characters in " << cnt << " node labels.";
        ran_one = true;
    }

    // Remove comments and nhx
    if( options.remove_comments_and_nhx ) {
        size_t cnt = 0;
        for( auto& node : tree.nodes() ) {
            auto& data = node.data<SimpleNewickNodeData>();
            if( data.comments.size() > 0 ) {
                ++cnt;
            }
            data.comments.clear();
        }
        LOG_MSG1 << "Removed comments (such as NHX information) from " << cnt << " nodes.";
        ran_one = true;
    }

    // Remove extra numbers
    if( options.remove_extra_numbers ) {
        size_t cnt = 0;
        for( auto& edge : tree.edges() ) {
            auto& data = edge.data<SimpleNewickEdgeData>();
            if( data.values.size() > 0 ) {
                ++cnt;
            }
            data.values.clear();
        }
        LOG_MSG1 << "Removed extra branch numbers on " << cnt << " branches.";
        ran_one = true;
    }

    // Remove jplace tags
    if( options.remove_jplace_tags ) {
        size_t cnt = 0;
        for( auto& edge : tree.edges() ) {
            auto& data = edge.data<SimpleNewickEdgeData>();
            if( data.tags.size() > 0 ) {
                ++cnt;
            }
            data.tags.clear();
        }
        LOG_MSG1 << "Removed (jplace) tags on " << cnt << " branches.";
        ran_one = true;
    }

    if( ! ran_one ) {
        LOG_WARN << "No cleaning option was provided. Tree will be written as-is.";
    }

    // Create a newick tree from it.
    LOG_MSG1 << "Writing output tree.";
    auto nw = SimpleNewickTreeNewickWriter();
    nw.write( tree, options.file_output.get_output_target( "clean-tree", "newick" ));
}
