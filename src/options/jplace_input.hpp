#ifndef GAPPA_OPTIONS_JPLACE_INPUT_H_
#define GAPPA_OPTIONS_JPLACE_INPUT_H_

/*
    gappa - Genesis Applications for Phylogenetic Placement Analysis
    Copyright (C) 2017 Lucas Czech

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

#include "genesis/placement/sample.hpp"
#include "genesis/placement/sample_set.hpp"

#include <string>
#include <vector>

// =================================================================================================
//      Jplace Input Options
// =================================================================================================

class JplaceInputOptions
{
public:

    // -------------------------------------------------------------------------
    //     Constructor and Rule of Five
    // -------------------------------------------------------------------------

    JplaceInputOptions()  = default;
    virtual ~JplaceInputOptions() = default;

    JplaceInputOptions( JplaceInputOptions const& other ) = default;
    JplaceInputOptions( JplaceInputOptions&& )            = default;

    JplaceInputOptions& operator= ( JplaceInputOptions const& other ) = default;
    JplaceInputOptions& operator= ( JplaceInputOptions&& )            = default;

    // -------------------------------------------------------------------------
    //     Setup Functions
    // -------------------------------------------------------------------------

    void add_jplace_input_options( CLI::App* sub );

    // -------------------------------------------------------------------------
    //     Run Functions
    // -------------------------------------------------------------------------

    /**
     * @brief Print some user output related to these options.
     */
    void print_jplace_input_options( size_t verbosity = 0 ) const;

    /**
     * @brief Return the list of paths as provided by the user, that is, without processing.
     */
    std::vector<std::string> const& cli_paths() const;

    /**
     * @brief Get the file names of the provided files, i.e., without directory and ending.
     *
     * This function calls genesis::utils::file_basename() and genesis::utils::file_filename() for
     * all paths. The result is for example useful for user output.
     */
    std::vector<std::string> jplace_base_file_names() const;

    /**
     * @brief Get the resolved full file paths of all jplace files provided by the user.
     */
    std::vector<std::string> jplace_file_paths() const;

    /**
     * @brief Read in jplace files and return them as a SampleSet.
     */
    genesis::placement::SampleSet sample_set() const;

    // -------------------------------------------------------------------------
    //     Helper Functions
    // -------------------------------------------------------------------------

private:

    /**
     * @brief Given a list of paths, find all jplace files.
     *
     * This function takes the list of paths. For each of them, it checks whether it is a file or a
     * directory. Files are immediately added to the result list, while directories are scanned
     * for any `.jplace` files in them, which are then added to the result list.
     *
     * This allows the users to hand over either their own files (no matter their extension), or
     * whole directory paths for convenience, which then however only use files ending in `.jplace`.
     */
    std::vector<std::string> resolve_jplace_paths_() const;

    // -------------------------------------------------------------------------
    //     Option Members
    // -------------------------------------------------------------------------

    std::vector<std::string> jplace_paths_;
    bool resolve_paths_ = true;

};

#endif // include guard
