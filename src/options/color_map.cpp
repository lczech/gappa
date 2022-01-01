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

#include "options/color_map.hpp"

#include "options/global.hpp"

#include "genesis/utils/core/fs.hpp"
#include "genesis/utils/text/string.hpp"
#include "genesis/utils/tools/color/functions.hpp"
#include "genesis/utils/tools/color/list_diverging.hpp"
#include "genesis/utils/tools/color/list_misc.hpp"
#include "genesis/utils/tools/color/list_qualitative.hpp"
#include "genesis/utils/tools/color/list_sequential.hpp"
#include "genesis/utils/tools/color/names.hpp"

#include <stdexcept>

// =================================================================================================
//      Constructor
// =================================================================================================

ColorMapOptions::ColorMapOptions()
{
    using namespace genesis::utils;
    over_color_option.value()  = color_to_hex( color_map_.over_color() );
    under_color_option.value() = color_to_hex( color_map_.under_color() );
    mask_color_option.value()  = color_to_hex( color_map_.mask_color() );
}

// =================================================================================================
//      Setup Functions
// =================================================================================================

void ColorMapOptions::add_color_list_opt_to_app(
    CLI::App* sub,
    std::string const& default_color_list,
    std::string const& group,
    std::string const& name
) {
    // Set Default
    color_list_option.value() = default_color_list;

    // Color List
    color_list_option = sub->add_option(
        name.empty() ? "--color-list" : "--" + name + "-color-list",
        color_list_option.value(),
        "List of colors to use for the palette. Can either be the name of a color list, "
        "a file containing one color per line, or an actual comma-separated list of colors. "
        "Colors can be specified in the format `#rrggbb` using hex values, or by web color names.",
        true
    );
    color_list_option.option()->group( group );

    // Reverse
    reverse_color_list_option = sub->add_flag_function(
        name.empty() ? "--reverse-color-list" : "--" + name + "-reverse-color-list",
        [this]( size_t ){
            reverse_color_list_option.value() = true;
            color_map_.reverse( true );
        },
        "If set, the order of colors of the `--color-list` is reversed."
    )->group( group );
}

void ColorMapOptions::add_under_opt_to_app(
    CLI::App* sub,
    std::string const& default_color,
    std::string const& group,
    std::string const& name
) {
    // Default color
    if( ! default_color.empty() ) {
        under_color_option.value() = default_color;
    }

    // Under Color
    under_color_option = sub->add_option(
        name.empty() ? "--under-color" : "--" + name + "-under-color",
        under_color_option.value(),
        "Color used to indicate values below the min value. "
        "Color can be specified in the format `#rrggbb` using hex values, or by web color names.",
        true
    )->group( group );

    // Clip Under
    clip_under_option = sub->add_flag_function(
        name.empty() ? "--clip-under" : "--" + name + "-clip-under",
        [this]( size_t ){
            clip_under_option.value() = true;
            color_map_.clip_under( true );
        },
        "Clip (i.e., clamp) values less than min to be inside `[ min, max ]`, "
        "by setting values that are too low to the specified min value. "
        "If set, `--under-color` is not used to indicate values out of range."
    );
    clip_under_option.option()->group( group );

    // Special: If we also use over color, we can offer a clip option shortcut.
    if( clip_over_option.option() ) {
        add_clip_opt_to_app( sub, group, name );
    }
}

void ColorMapOptions::add_over_opt_to_app(
    CLI::App* sub,
    std::string const& default_color,
    std::string const& group,
    std::string const& name
) {
    // Default color
    if( ! default_color.empty() ) {
        over_color_option.value() = default_color;
    }

    // Over Color
    over_color_option = sub->add_option(
        name.empty() ? "--over-color" : "--" + name + "-over-color",
        over_color_option.value(),
        "Color used to indicate values above the max value. "
        "Color can be specified in the format `#rrggbb` using hex values, or by web color names.",
        true
    );
    over_color_option.option()->group( group );

    // Clip Over
    clip_over_option = sub->add_flag_function(
        name.empty() ? "--clip-over" : "--" + name + "-clip-over",
        [this]( size_t ){
            clip_over_option.value() = true;
            color_map_.clip_over( true );
        },
        "Clip (i.e., clamp) values greater than max to be inside `[ min, max ]`, "
        "by setting values that are too high to the specified max value. "
        "If set, `--over-color` is not used to indicate values out of range."
    );
    clip_over_option.option()->group( group );

    // Special: If we also use under color, we can offer a clip option shortcut.
    if( clip_under_option.option() ) {
        add_clip_opt_to_app( sub, group, name );
    }
}

void ColorMapOptions::add_clip_opt_to_app(
    CLI::App* sub,
    std::string const& group,
    std::string const& name
) {
    // Do not set again if already set.
    if( clip_option.option() ) {
        return;
    }

    clip_option = sub->add_flag_function(
        name.empty() ? "--clip" : "--" + name + "-clip",
        [this]( size_t ){
            clip_option.value() = true;
            color_map_.clip( true );
        },
        "Clip (i.e., clamp) values to be inside `[ min, max ]`, "
        "by setting values outside of that interval to the nearest boundary of it. "
        "This option is a shortcut to set `" +
        ( name.empty() ? "--clip-under" : "--" + name + "-clip-under" ) +
        "` and " +
        ( name.empty() ? "--clip-over" : "--" + name + "-clip-over" ) +
        "` at once."
    );
    clip_option.option()->group( group );
}

void ColorMapOptions::add_mask_opt_to_app(
    CLI::App* sub,
    std::string const& default_color,
    std::string const& group,
    std::string const& name
) {
    // Default color
    if( ! default_color.empty() ) {
        mask_color_option.value() = default_color;
    }

    // Mask Color
    mask_color_option = sub->add_option(
        name.empty() ? "--mask-color" : "--" + name + "-mask-color",
        mask_color_option.value(),
        "Color used to indicate masked or invalid values, such as infinities or NaNs. "
        "Color can be specified in the format `#rrggbb` using hex values, or by web color names.",
        true
    );
    mask_color_option.option()->group( group );
}

// =================================================================================================
//      Run Functions
// =================================================================================================

genesis::utils::ColorMap const& ColorMapOptions::color_map() const
{
    using namespace genesis::utils;

    // If map was already filled in previous call, just return it.
    if( ! color_map_.empty() ) {
        return color_map_;
    }

    // Resolve special colors.
    // As the second param for the resolve call, we use a lengthy expression that
    // tries to provide the correct option long name, so that it is correctly displayed to the user
    // if an invalid color is specified.
    color_map_.under_color( resolve_color_string_(
        under_color_option.value(),
        under_color_option.option() && ! under_color_option.option()->get_lnames().empty()
        ? under_color_option.option()->get_lnames()[0]
        : "--under-color"
    ));
    color_map_.over_color(  resolve_color_string_(
        over_color_option.value(),
        over_color_option.option() && ! over_color_option.option()->get_lnames().empty()
        ? over_color_option.option()->get_lnames()[0]
        : "--over-color"
    ));
    color_map_.mask_color(  resolve_color_string_(
        mask_color_option.value(),
        mask_color_option.option() && ! mask_color_option.option()->get_lnames().empty()
        ? mask_color_option.option()->get_lnames()[0]
        : "--mask-color"
    ));

    // Now resolve the actual color list.
    // Try color list names first.
    if( contains_ci( color_list_diverging_names(), color_list_option.value() )) {
        color_map_.palette( color_list_diverging( color_list_option.value() ));
        return color_map_;
    }
    if( contains_ci( color_list_qualitative_names(), color_list_option.value() )) {
        color_map_.palette( color_list_qualitative( color_list_option.value() ));
        return color_map_;
    }
    if( contains_ci( color_list_sequential_names(), color_list_option.value() )) {
        color_map_.palette( color_list_sequential( color_list_option.value() ));
        return color_map_;
    }
    if( contains_ci( color_list_misc_names(), color_list_option.value() )) {
        color_map_.palette( color_list_misc( color_list_option.value() ));
        return color_map_;
    }

    // Now check if it is a file with colors.
    if( is_file( color_list_option.value() ) ) {
        auto const list = split( file_read( color_list_option.value() ), "\n\r", true );
        color_map_.palette( resolve_color_list_(
            list,
            color_list_option.option() && ! color_list_option.option()->get_lnames().empty()
            ? color_list_option.option()->get_lnames()[0]
            : "--color-list"
        ));
        return color_map_;
    }

    // Finally, treat it as a comma separated list of colors.
    auto const list = split( color_list_option.value(), ",", true );
    color_map_.palette( resolve_color_list_(
        list,
        color_list_option.option() && ! color_list_option.option()->get_lnames().empty()
        ? color_list_option.option()->get_lnames()[0]
        : "--color-list"
    ));
    return color_map_;
}

// =================================================================================================
//      Helper Functions
// =================================================================================================

genesis::utils::Color ColorMapOptions::resolve_color_string_(
    std::string const& color_str,
    std::string const& param_name
) const {
    try {
        return genesis::utils::resolve_color_string( color_str );
    } catch( std::exception& ex ) {
        throw CLI::ValidationError(
            param_name, "Invalid color '" + color_str + "': " +
            std::string( ex.what() )
        );
    }
}

std::vector<genesis::utils::Color> ColorMapOptions::resolve_color_list_(
    std::vector<std::string> const& list,
    std::string const& param_name
) const {
    std::vector<genesis::utils::Color> result;

    for( auto const& color_str : list ) {
        result.push_back( resolve_color_string_( color_str, param_name ));
    }

    return result;
}
