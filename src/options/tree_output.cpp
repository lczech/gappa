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

#include "options/tree_output.hpp"
#include "options/global.hpp"

#include "genesis/tree/drawing/functions.hpp"
#include "genesis/utils/text/string.hpp"
#include "genesis/utils/tools/color/functions.hpp"
#include "genesis/utils/tools/color/helpers.hpp"
#include "genesis/utils/tools/tickmarks.hpp"

#include <cassert>
#include <iostream>
#include <stdexcept>

// =================================================================================================
//      Setup Functions
// =================================================================================================

void TreeOutputOptions::add_tree_output_opts_to_app( CLI::App* sub )
{
    sub->add_flag(
        "--write-newick-tree",
        write_newick_tree_,
        "If set, the tree is written to a Newick file."
    )->group( "Tree Output" );
    sub->add_flag(
        "--write-nexus-tree",
        write_nexus_tree_,
        "If set, the tree is written to a Nexus file."
    )->group( "Tree Output" );
    sub->add_flag(
        "--write-phyloxml-tree",
        write_phyloxml_tree_,
        "If set, the tree is written to a Phyloxml file."
    )->group( "Tree Output" );
    auto svg_tree_opt = sub->add_flag(
        "--write-svg-tree",
        write_svg_tree_,
        "If set, the tree is written to a Svg file."
    )->group( "Tree Output" );

    svg_tree_output.add_svg_tree_output_opts_to_app( sub, svg_tree_opt );
}

// =================================================================================================
//      Run Functions
// =================================================================================================

void TreeOutputOptions::check_tree_formats() const
{
    if( ! write_newick_tree_ && ! write_nexus_tree_ && ! write_phyloxml_tree_ && ! write_svg_tree_ ) {
        LOG_WARN << "Warning: You did not specify any tree output format. "
                 << "Thus, no tree files will be written. "
                 << "In order to specify the wanted formats, use the --write-...-tree options.";
    }
}

std::vector<std::string> TreeOutputOptions::get_extensions() const
{
    std::vector<std::string> res;

    if( write_newick_tree_ ) {
        res.push_back( "newick" );
    }
    if( write_nexus_tree_ ) {
        res.push_back( "nexus" );
    }
    if( write_phyloxml_tree_ ) {
        res.push_back( "phyloxml" );
    }
    if( write_svg_tree_ ) {
        res.push_back( "svg" );
    }

    return res;
}

void TreeOutputOptions::write_tree_to_files(
    genesis::tree::CommonTree const&          tree,
    FileOutputOptions const&                  file_output_options,
    std::string const&                        infix
) const {
    using namespace genesis::tree;

    // Currently, this tree output class uses the file output options only to get the file names,
    // but writes to them using different functions. Hence, we need to make sure that we do not
    // accidentally write to gz files (gappa commands that write tree files should never activate
    // this options as of now, so here we check that this is true).
    // In the future, once the genesis shortcut functions for writing trees that we use here are
    // refactored to use an output target instead of a file name, we can also allow to write
    // compressed trees. But not for now.
    assert( file_output_options.compress_option == nullptr );
    assert( !file_output_options.compress() );

    if( write_newick_tree_ ) {
        write_tree_to_newick_file( tree, file_output_options.get_output_filename( infix, "newick" ));
    }

    if( write_nexus_tree_ ) {
        write_tree_to_nexus_file( tree, file_output_options.get_output_filename( infix, "nexus" ));
    }

    if( write_phyloxml_tree_ ) {
        write_tree_to_phyloxml_file( tree, file_output_options.get_output_filename( infix, "phyloxml" ));
    }

    if( write_svg_tree_ ) {
        write_tree_to_svg_file(
            tree,
            svg_tree_output.layout_parameters(),
            file_output_options.get_output_filename( infix, "svg" )
        );
    }
}

void TreeOutputOptions::write_tree_to_files(
    genesis::tree::CommonTree const&          tree,
    std::vector<genesis::utils::Color> const& color_per_branch,
    FileOutputOptions const&                  file_output_options,
    std::string const&                        infix
) const {
    using namespace genesis::tree;

    // See above for reasoning of these assertions.
    assert( file_output_options.compress_option == nullptr );
    assert( !file_output_options.compress() );

    if( write_newick_tree_ ) {
        if( !( write_nexus_tree_ || write_phyloxml_tree_ || write_svg_tree_ )) {
            LOG_WARN << "Warning: Option --write-newick-tree is set, but the output contains colors, "
                     << "which are not available in the Newick format. The Newick tree only "
                     << "contains the topology of the tree with names and branch lengths. "
                     << "Use another format such as nexus, phyloxml, or svg to get a colored tree!";
        }

        write_tree_to_newick_file(
            tree, file_output_options.get_output_filename( infix, "newick" )
        );
    }

    if( write_nexus_tree_ ) {
        write_color_tree_to_nexus_file(
            tree, color_per_branch, file_output_options.get_output_filename( infix, "nexus" )
        );
    }

    if( write_phyloxml_tree_ ) {
        write_color_tree_to_phyloxml_file(
            tree, color_per_branch, file_output_options.get_output_filename( infix, "phyloxml" )
        );
    }

    if( write_svg_tree_ ) {
        write_color_tree_to_svg_file(
            tree,
            svg_tree_output.layout_parameters(),
            color_per_branch,
            file_output_options.get_output_filename( infix, "svg" )
        );
    }
}

void TreeOutputOptions::write_tree_to_files(
    genesis::tree::CommonTree const&          tree,
    std::vector<genesis::utils::Color> const& color_per_branch,
    genesis::utils::ColorMap const&           color_map,
    genesis::utils::ColorNormalization const& color_norm,
    FileOutputOptions const&                  file_output_options,
    std::string const&                        infix
) const {
    using namespace genesis::tree;
    using namespace genesis::utils;

    // See above for reasoning of these assertions.
    assert( file_output_options.compress_option == nullptr );
    assert( !file_output_options.compress() );

    // In case we output a non svg tree, we need to report colors and tickmarks,
    // as they are not available in the other formats.
    bool print_legend = false;

    if( write_newick_tree_ ) {
        if( !( write_nexus_tree_ || write_phyloxml_tree_ || write_svg_tree_ )) {
            LOG_WARN << "Warning: Option --write-newick-tree is set, but the output contains colors, "
                     << "which are not available in the Newick format. The Newick tree only "
                     << "contains the topology of the tree with names and branch lengths. "
                     << "Use another format such as nexus, phyloxml, or svg to get a colored tree!";
        }

        write_tree_to_newick_file(
            tree, file_output_options.get_output_filename( infix, "newick" )
        );
    }

    if( write_nexus_tree_ ) {
        write_color_tree_to_nexus_file(
            tree, color_per_branch, file_output_options.get_output_filename( infix, "nexus" )
        );
        print_legend = true;
    }

    if( write_phyloxml_tree_ ) {
        write_color_tree_to_phyloxml_file(
            tree, color_per_branch, file_output_options.get_output_filename( infix, "phyloxml" )
        );
        print_legend = true;
    }

    if( write_svg_tree_ ) {
        write_color_tree_to_svg_file(
            tree,
            svg_tree_output.layout_parameters(),
            color_per_branch,
            color_map,
            color_norm,
            file_output_options.get_output_filename( infix, "svg" )
        );
    }

    if( print_legend ) {
        // TODO maybe make the num ticks changable. if so, also use it for the svg output!
        auto const tickmarks = color_tickmarks( color_norm, 5 );

        LOG_MSG1 << "Output options --write-nexus-tree and --write-phyloxml-tree produce trees "
                 << "with colored branches. These formats are however not able to store the legend, "
                 << "that is, which color represents which value. Thus, use to following positions "
                 << "to create a legend (with linear color interpolation between the positions). "
                 << "These positions range from 0.0 (lowest) to 1.0 (heighest), and are labeled "
                 << "with the values and colors represented by those positions.";

        for( auto const& tick : tickmarks ) {
            auto const rel_pos = tick.first;
            auto label = tick.second;

            if( rel_pos == 0.0 && color_map.clip_under() ) {
                label = "≤ " + label;
            }
            if( rel_pos == 1.0 && color_map.clip_over() ) {
                label = "≥ " + label;
            }

            auto const col_str = color_to_hex( color_map( rel_pos ));
            LOG_MSG1 << "    At " << to_string_precise( rel_pos, 3 ) << ": Label '"
                     << label << "', Color " << col_str;
        }

        LOG_MSG1 << "Alternatively, use the option --write-svg-tree to create an Svg file "
                 << "from which the color legend can be copied.";
        LOG_BOLD;
    }
}
