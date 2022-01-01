#ifndef GAPPA_OPTIONS_COLOR_MAP_H_
#define GAPPA_OPTIONS_COLOR_MAP_H_

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

#include "CLI/CLI.hpp"

#include "tools/cli_option.hpp"

#include "genesis/utils/tools/color.hpp"
#include "genesis/utils/tools/color/map.hpp"

#include <limits>
#include <memory>
#include <string>
#include <vector>

// =================================================================================================
//      Color Map Options
// =================================================================================================

/**
 * @brief Helper class to add command line parameter to use a color map,
 * that is, to select color palettes and gradients for output.
 */
class ColorMapOptions
{
public:

    // -------------------------------------------------------------------------
    //     Constructor and Rule of Five
    // -------------------------------------------------------------------------

    ColorMapOptions();
    ~ColorMapOptions() = default;

    ColorMapOptions( ColorMapOptions const& other ) = default;
    ColorMapOptions( ColorMapOptions&& )            = default;

    ColorMapOptions& operator= ( ColorMapOptions const& other ) = default;
    ColorMapOptions& operator= ( ColorMapOptions&& )            = default;

    // -------------------------------------------------------------------------
    //     Setup Functions
    // -------------------------------------------------------------------------

    /**
     * @brief Add an option to input a list of colors to an app, e.g., in order to customize
     * the color scheme of gradients.
     *
     * The function takes either a name of a list, a file containing colors, or a comma separated
     * list of colors. Colors can be specified as names (web and xkcd), or as hex, using a leading #.
     */
    void add_color_list_opt_to_app(
        CLI::App* sub,
        std::string const& default_color_list,
        std::string const& group = "Color",
        std::string const& name = ""
    );

    void add_under_opt_to_app(
        CLI::App* sub,
        std::string const& default_color = "",
        std::string const& group = "Color",
        std::string const& name = ""
    );

    void add_over_opt_to_app(
        CLI::App* sub,
        std::string const& default_color = "",
        std::string const& group = "Color",
        std::string const& name = ""
    );

    void add_clip_opt_to_app(
        CLI::App* sub,
        std::string const& group = "Color",
        std::string const& name = ""
    );

    void add_mask_opt_to_app(
        CLI::App* sub,
        std::string const& default_color = "",
        std::string const& group = "Color",
        std::string const& name = ""
    );

    // -------------------------------------------------------------------------
    //     Run Functions
    // -------------------------------------------------------------------------

    /**
     * @brief Get the color map with all settings applied that were provided by the user.
     */
    genesis::utils::ColorMap const& color_map() const;

    // -------------------------------------------------------------------------
    //     Helper Functions
    // -------------------------------------------------------------------------

private:

    /**
     * @brief Helper function that wraps the genesis function of the same name,
     * but offers a nicer error feedback.
     */
    genesis::utils::Color resolve_color_string_(
        std::string const& color_str,
        std::string const& param_name
    ) const;

    /**
     * @brief Same as resolve_color_string_(), but for a whole list of colors.
     */
    std::vector<genesis::utils::Color> resolve_color_list_(
        std::vector<std::string> const& list,
        std::string const& param_name
    ) const;

    // -------------------------------------------------------------------------
    //     Option Members
    // -------------------------------------------------------------------------

private:

    // We internally build a proper color map with all the options,
    // and then hand this over to where it is needed.
    mutable genesis::utils::ColorMap color_map_;

public:

    CliOption<std::string> color_list_option;
    CliOption<bool> reverse_color_list_option {false};

    CliOption<std::string> under_color_option;
    CliOption<bool> clip_under_option {false};

    CliOption<std::string> over_color_option;
    CliOption<bool> clip_over_option {false};

    CliOption<bool> clip_option {false};
    CliOption<std::string> mask_color_option;

};

#endif // include guard
