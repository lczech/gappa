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

#include "options/color_norm.hpp"

#include "options/global.hpp"

#include "genesis/utils/core/std.hpp"

#include <stdexcept>

// =================================================================================================
//      Setup Functions
// =================================================================================================

CLI::Option* ColorNormOptions::add_log_scaling_opt_to_app(
    CLI::App* sub,
    std::string const& group,
    std::string const& name
) {
    log_scaling_option = sub->add_flag(
        name.empty() ? "--log-scaling" : "--" + name + "-log-scaling",
        log_scaling_option.value(),
        "If set, the sequential color list is logarithmically scaled instead of linearily."
    );
    log_scaling_option.option()->group( group );

    return log_scaling_option.option();
}

CLI::Option* ColorNormOptions::add_min_value_opt_to_app(
    CLI::App* sub,
    std::string const& group,
    std::string const& name
) {
    // Min
    min_value_option = sub->add_option(
        name.empty() ? "--min-value" : "--" + name + "-min-value",
        min_value_option.value(),
        "Minimum value that is represented by the color scale. "
        "If not set, the minimum value of the data is used."
    );
    min_value_option.option()->group( group );

    return min_value_option.option();
}

CLI::Option* ColorNormOptions::add_mid_value_opt_to_app(
    CLI::App* sub,
    std::string const& group,
    std::string const& name
) {
    // Mid
    mid_value_option = sub->add_option(
        name.empty() ? "--mid-value" : "--" + name + "-mid-value",
        mid_value_option.value(),
        "Mid value that is represented by the diverging color scale. "
        "If not set, the mid value of the data is used."
    );
    mid_value_option.option()->group( group );

    return mid_value_option.option();
}

CLI::Option* ColorNormOptions::add_max_value_opt_to_app(
    CLI::App* sub,
    std::string const& group,
    std::string const& name
) {
    // Max
    max_value_option = sub->add_option(
        name.empty() ? "--max-value" : "--" + name + "-max-value",
        max_value_option.value(),
        "Maximum value that is represented by the color scale. "
        "If not set, the maximum value of the data is used."
    );
    max_value_option.option()->group( group );

    return max_value_option.option();
}

CLI::Option* ColorNormOptions::add_mask_value_opt_to_app(
    CLI::App* sub,
    std::string const& group,
    std::string const& name
) {
    // Mask
    mask_value_option = sub->add_option(
        name.empty() ? "--mask-value" : "--" + name + "-mask-value",
        mask_value_option.value(),
        "Mask value that identifies invalid values (in addition to infinities and NaN values, "
        "which are always considered invalid, and hence always masked). "
        "Value of the data that compare equal to the mask value are colored using --mask-color. "
        "This is meant as a simple means of filtering and visualizing invalid values. "
        "If not set, no masking value is applied."
    );
    mask_value_option.option()->group( group );

    return mask_value_option.option();
}

// =================================================================================================
//      Run Functions
// =================================================================================================

std::unique_ptr<genesis::utils::ColorNormalizationLinear> ColorNormOptions::get_sequential_norm() const
{
    using namespace genesis::utils;
    std::unique_ptr<ColorNormalizationLinear> res;

    if( log_scaling_option.value() ) {
        res = make_unique<ColorNormalizationLogarithmic>();
        apply_options( static_cast<ColorNormalizationLogarithmic&>( *res ) );
    } else {
        res = make_unique<ColorNormalizationLinear>();
        apply_options( static_cast<ColorNormalizationLinear&>( *res ));
    }

    return res;
}

genesis::utils::ColorNormalizationDiverging ColorNormOptions::get_diverging_norm() const
{
    using namespace genesis::utils;
    auto res = ColorNormalizationDiverging();
    apply_options( res );
    return res;
}

void ColorNormOptions::apply_options( genesis::utils::ColorNormalizationLinear& norm ) const
{
    // CLI objects evaluate to true if the option was passed by the user.
    // So here, we first test wheter the option object was actually created,
    // that is, whether the command uses it, and then, whether the user also specified a value.
    // Only if both are true, we use the value to overwrite the norm value.

    // if( min_value_option.option() && *min_value_option.option() ) {
    if( min_value_option ) {
        norm.min_value( min_value_option.value() );
    }
    // if( max_value_option.option() && *max_value_option.option() ) {
    if( max_value_option ) {
        norm.max_value( max_value_option.value() );
    }
    // if( mask_value_option.option() && *mask_value_option.option() ) {
    if( mask_value_option ) {
        norm.mask_value( mask_value_option.value() );
    }
}

void ColorNormOptions::apply_options( genesis::utils::ColorNormalizationLogarithmic& norm ) const
{
    apply_options( static_cast<genesis::utils::ColorNormalizationLinear&>( norm ));
}

void ColorNormOptions::apply_options( genesis::utils::ColorNormalizationDiverging& norm ) const
{
    // First apply base class.
    apply_options( static_cast<genesis::utils::ColorNormalizationLinear&>( norm ));

    // Then special divergent options.
    if( mid_value_option ) {
        norm.mid_value( mid_value_option.value() );
    }
}
