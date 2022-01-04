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

#include "options/tree_output_svg.hpp"

#include <iostream>
#include <stdexcept>

// =================================================================================================
//      Setup Functions
// =================================================================================================

void SvgTreeOutputOptions::add_svg_tree_output_opts_to_app( CLI::App* sub, CLI::Option* svg_tree_opt )
{
    // Shape
    auto shape_opt = sub->add_option(
        "--svg-tree-shape",
        shape_,
        "Shape of the tree.",
        // "Shape of the tree, 'circular' or 'rectangular'.",
        true
    );
    shape_opt->group( "Svg Tree Output" );
    shape_opt->transform(
        CLI::IsMember({ "circular", "rectangular" }, CLI::ignore_case )
    );
    shape_opt->needs( svg_tree_opt );

    // Type
    auto type_opt = sub->add_option(
        "--svg-tree-type",
        type_,
        "Type of the tree, either using branch lengths (`phylogram`), or not (`cladogram`).",
        // "Type of the tree, 'cladogram' or 'phylogram'.",
        true
    );
    type_opt->group( "Svg Tree Output" );
    type_opt->transform(
        CLI::IsMember({ "cladogram", "phylogram" }, CLI::ignore_case )
    );
    type_opt->needs( svg_tree_opt );

    // Stroke width
    auto stroke_width_opt = sub->add_option(
        "--svg-tree-stroke-width",
        stroke_width_,
        "Svg stroke width for the branches of the tree.",
        true
    );
    stroke_width_opt->group( "Svg Tree Output" );
    stroke_width_opt->needs( svg_tree_opt );

    // Ladderize
    auto ladderize_opt = sub->add_flag(
        "--svg-tree-ladderize",
        ladderize_,
        "If set, the tree is ladderized."
    );
    ladderize_opt->group( "Svg Tree Output" );
    ladderize_opt->needs( svg_tree_opt );
}

// =================================================================================================
//      Run Functions
// =================================================================================================

genesis::tree::LayoutParameters SvgTreeOutputOptions::layout_parameters() const
{
    using namespace genesis;
    using namespace genesis::tree;

    LayoutParameters res;

    if( shape_ == "circular" ) {
        res.shape = LayoutShape::kCircular;
    } else if( shape_ == "rectangular" ) {
        res.shape = LayoutShape::kRectangular;
    } else {
        throw CLI::ValidationError(
            "--svg-tree-shape", "Invalid shape '" + shape_ + "'."
        );
    }

    if( type_ == "cladogram" ) {
        res.type = LayoutType::kCladogram;
    } else if( type_ == "phylogram" ) {
        res.type = LayoutType::kPhylogram;
    } else {
        throw CLI::ValidationError(
            "--svg-tree-type", "Invalid type '" + type_ + "'."
        );
    }

    if( stroke_width_ <= 0.0 ) {
        throw CLI::ValidationError(
            "--svg-tree-stroke-width",
            "Svg stroke width has to be positive."
        );
    }

    res.stroke.width = stroke_width_;
    res.ladderize = ladderize_;
    return res;
}
