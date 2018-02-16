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

#include "options/color_tree_output.hpp"

#include "genesis/tree/drawing/functions.hpp"

#include <iostream>
#include <stdexcept>

// =================================================================================================
//      Setup Functions
// =================================================================================================

void ColorTreeOutputOptions::add_color_tree_opts_to_app( CLI::App* sub )
{
    svg_tree_output.add_svg_tree_opts_to_app( sub );

    // TODO add options to deactivate certain output formats
}

// =================================================================================================
//      Run Functions
// =================================================================================================

void ColorTreeOutputOptions::write_tree_to_files(
    genesis::tree::DefaultTree const&         tree,
    std::vector<genesis::utils::Color> const& color_per_branch,
    std::string const&                        file_path_prefix
) const {
    using namespace genesis::tree;

    write_color_tree_to_svg_file(
        tree,
        svg_tree_output.layout_parameters(),
        color_per_branch,
        file_path_prefix + ".svg"
    );
}

void ColorTreeOutputOptions::write_tree_to_files(
    genesis::tree::DefaultTree const&         tree,
    std::vector<genesis::utils::Color> const& color_per_branch,
    genesis::utils::ColorMap const&           color_map,
    genesis::utils::ColorNormalization const& color_norm,
    std::string const&                        file_path_prefix
) const {
    using namespace genesis::tree;

    write_color_tree_to_svg_file(
        tree,
        svg_tree_output.layout_parameters(),
        color_per_branch,
        color_map,
        color_norm,
        file_path_prefix + ".svg"
    );
}
