/*
    gappa - Genesis Applications for Phylogenetic Placement Analysis
    Copyright (C) 2017-2024 Lucas Czech

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

#include "options/color_single.hpp"

#include "options/global.hpp"

#include "genesis/utils/color/functions.hpp"
#include "genesis/utils/color/names.hpp"
#include "genesis/utils/text/string.hpp"

#include <stdexcept>

// =================================================================================================
//      Setup Functions
// =================================================================================================

CLI::Option* SingleColorOptions::add_single_color_opt_to_app(
    CLI::App* sub,
    std::string const& name,
    std::string const& default_color
) {
    // Set Default
    if( ! default_color.empty() ) {
        color_option.value() = default_color;
    }

    // Color List
    color_option = sub->add_option(
        "--" + name + "-color",
        color_option.value(),
        "Colors to use for " + name + ".",
        true
    )->group( "Color" );

    return color_option;
}

// =================================================================================================
//      Run Functions
// =================================================================================================

genesis::utils::Color SingleColorOptions::color() const
{
    try {
        return genesis::utils::resolve_color_string( color_option.value() );
    } catch( std::exception& ex ) {
        auto const name = color_option.option() && ! color_option.option()->get_lnames().empty()
            ? color_option.option()->get_lnames()[0]
            : "--color"
        ;
        throw CLI::ValidationError(
            name, "Invalid color '" + color_option.value() + "': " +
            std::string( ex.what() )
        );
    }
}
