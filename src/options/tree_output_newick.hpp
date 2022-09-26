#ifndef GAPPA_OPTIONS_TREE_OUTPUT_NEWICK_H_
#define GAPPA_OPTIONS_TREE_OUTPUT_NEWICK_H_

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

#include "genesis/tree/common_tree/functions.hpp"
#include "genesis/tree/common_tree/newick_writer.hpp"
#include "genesis/tree/drawing/functions.hpp"
#include "genesis/tree/formats/newick/writer.hpp"

#include <string>
#include <vector>

// =================================================================================================
//      Newick Tree Output Options
// =================================================================================================

/**
 * @brief
 */
class NewickTreeOutputOptions
{
public:

    // -------------------------------------------------------------------------
    //     Constructor and Rule of Five
    // -------------------------------------------------------------------------

    NewickTreeOutputOptions()  = default;
    ~NewickTreeOutputOptions() = default;

    NewickTreeOutputOptions( NewickTreeOutputOptions const& other ) = default;
    NewickTreeOutputOptions( NewickTreeOutputOptions&& )            = default;

    NewickTreeOutputOptions& operator= ( NewickTreeOutputOptions const& other ) = default;
    NewickTreeOutputOptions& operator= ( NewickTreeOutputOptions&& )            = default;

    // -------------------------------------------------------------------------
    //     Setup Functions
    // -------------------------------------------------------------------------

    /**
     * @brief Add all available Newick settings to the CLI.
     *
     * Takes a newick tree opt as well, which, if provided here, will be a required CLI arg to be
     * provided by the user.
     */
    void add_newick_tree_output_opts_to_app( CLI::App* sub, CLI::Option* newick_tree_opt );

    void add_newick_tree_branch_length_precision_opt_to_app(
        CLI::App* sub, CLI::Option* newick_tree_opt
    );
    void add_newick_tree_quote_invalid_chars_opt_to_app(
        CLI::App* sub, CLI::Option* newick_tree_opt
    );

    // -------------------------------------------------------------------------
    //     Run Functions
    // -------------------------------------------------------------------------

    /**
     * @brief Write a tree in newick format to a file.
     *
     * This differs a bit from the other formats, which we simply write using the genesis
     * convenience functions. Those functions already provide all the params necessary for our
     * customization here. Not the newick convenience function write_tree_to_newick_file() though,
     * as it's lacking support for the custom settings that we provide here. So instead we provide
     * this function here to write in newick while taking care of these extra params.
     */
    void write_tree(
        genesis::tree::CommonTree const& tree,
        std::shared_ptr<genesis::utils::BaseOutputTarget> target
    ) const;

    void write_tree(
        genesis::tree::CommonTreeNewickWriter& writer,
        genesis::tree::CommonTree const& tree,
        std::shared_ptr<genesis::utils::BaseOutputTarget> target
    ) const;

    // -------------------------------------------------------------------------
    //     Option Members
    // -------------------------------------------------------------------------

private:

    int branch_length_precision_ = 6;
    bool quote_invalid_chars_ = false;

};

#endif // include guard
