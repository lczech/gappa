#ifndef GAPPA_OPTIONS_COLOR_H_
#define GAPPA_OPTIONS_COLOR_H_

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

#include "CLI/CLI.hpp"

#include "genesis/utils/tools/color.hpp"
#include "genesis/utils/tools/color/map.hpp"
#include "genesis/utils/tools/color/normalization.hpp"

#include <limits>
#include <memory>
#include <string>
#include <vector>

// =================================================================================================
//      Color Options
// =================================================================================================

/**
 * @brief Helper class to add command line parameter to use color.
 */
class ColorOptions
{
public:

    // -------------------------------------------------------------------------
    //     Constructor and Rule of Five
    // -------------------------------------------------------------------------

    ColorOptions();
    virtual ~ColorOptions() = default;

    ColorOptions( ColorOptions const& other ) = default;
    ColorOptions( ColorOptions&& )            = default;

    ColorOptions& operator= ( ColorOptions const& other ) = default;
    ColorOptions& operator= ( ColorOptions&& )            = default;

    // -------------------------------------------------------------------------
    //     Setup Functions
    // -------------------------------------------------------------------------

    /**
     * @brief Add an option to input a list of colours to an app, e.g., in order to customize
     * the color scheme of gradients.
     *
     * The function takes either a name of a list, a file containing colors, or a comma separated
     * list of colors. Colors can be specified as names (web and xkcd), or as hex, using a leading #.
     */
    CLI::Option* add_color_list_opt_to_app(
        CLI::App* sub,
        bool add_log_option = false,
        std::string const& group = "Color"
    );

    /**
     * @brief Add options to change the minumum and under flow values.
     */
    CLI::Option* add_min_opt_to_app(
        CLI::App* sub,
        std::string const& group = "Color"
    );

    /**
     * @brief Add options to change the maximum and over flow values.
     */
    CLI::Option* add_max_opt_to_app(
        CLI::App* sub,
        std::string const& group = "Color"
    );

    /**
     * @brief
     */
    CLI::Option* add_mask_opt_to_app(
        CLI::App* sub,
        std::string const& group = "Color"
    );

    // -------------------------------------------------------------------------
    //     Option Objects
    // -------------------------------------------------------------------------

    /**
     * @brief Return the CLI11 option for the color list that this object belongs to.
     */
    CLI::Option* color_list_option()
    {
        return color_list_option_;
    }

    /**
     * @brief Return the CLI11 option for the color list that this object belongs to.
     */
    CLI::Option const* color_list_option() const
    {
        return color_list_option_;
    }

    /**
     * @brief Return the CLI11 option for the mininum value that this object belongs to.
     */
    CLI::Option* min_option()
    {
        return min_option_;
    }

    /**
     * @brief Return the CLI11 option for the mininum value that this object belongs to.
     */
    CLI::Option const* min_option() const
    {
        return min_option_;
    }

    /**
     * @brief Return the CLI11 option for the maximum value that this object belongs to.
     */
    CLI::Option* max_option()
    {
        return max_option_;
    }

    /**
     * @brief Return the CLI11 option for the maximum value that this object belongs to.
     */
    CLI::Option const* max_option() const
    {
        return max_option_;
    }

    /**
     * @brief Return the CLI11 option for the mask value that this object belongs to.
     */
    CLI::Option* mask_option()
    {
        return mask_option_;
    }

    /**
     * @brief Return the CLI11 option for the mask value that this object belongs to.
     */
    CLI::Option const* mask_option() const
    {
        return mask_option_;
    }

    // -------------------------------------------------------------------------
    //     Run Functions
    // -------------------------------------------------------------------------

public:

    genesis::utils::ColorMap const& color_map() const;

    double min_value() const
    {
        return min_value_;
    }

    double max_value() const
    {
        return max_value_;
    }

    double mask_value() const
    {
        return mask_value_;
    }

    bool log_scaling() const
    {
        return log_scaling_;
    }

    // -------------------------------------------------------------------------
    //     Option Members
    // -------------------------------------------------------------------------

private:

    // Helper members that store the user input for (list of) colors.
    // We need this, because this cannot bind directly to the properties of the color objects,
    // and because we take colors as strings of differnet format and need to convert first.
    std::string palette_param_;
    std::string under_color_param_;
    std::string over_color_param_;
    std::string mask_color_param_;

    // Norm properties to bind to. As the normalization is a class hierarchy,
    // we cannot set those values in a fixed object :-(
    // As always, inheritance is the base class of evil!
    double min_value_ = 0.0;
    double max_value_ = 1.0;
    double mask_value_ = std::numeric_limits<double>::quiet_NaN();
    bool log_scaling_ = false;

    mutable genesis::utils::ColorMap color_map_;

    CLI::Option* color_list_option_ = nullptr;
    CLI::Option* min_option_ = nullptr;
    CLI::Option* max_option_ = nullptr;
    CLI::Option* mask_option_ = nullptr;

};

#endif // include guard
