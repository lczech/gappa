#ifndef GAPPA_OPTIONS_FILE_OUTPUT_H_
#define GAPPA_OPTIONS_FILE_OUTPUT_H_

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

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

// =================================================================================================
//      Output Directory Options
// =================================================================================================

class FileOutputOptions
{
public:

    // -------------------------------------------------------------------------
    //     Constructor and Rule of Five
    // -------------------------------------------------------------------------

    FileOutputOptions()  = default;
    virtual ~FileOutputOptions() = default;

    FileOutputOptions( FileOutputOptions const& other ) = default;
    FileOutputOptions( FileOutputOptions&& )            = default;

    FileOutputOptions& operator= ( FileOutputOptions const& other ) = default;
    FileOutputOptions& operator= ( FileOutputOptions&& )            = default;

    // -------------------------------------------------------------------------
    //     Setup Functions
    // -------------------------------------------------------------------------

    /**
     * @brief Add a single output dir, with an option named "--out-dir".
     */
    void add_to_app( CLI::App* sub );

    /**
     * @brief Add a named output dir "--name-out-dir".
     *
     * Can be called multiple times for commands that output different types of files.
     */
    void add_to_app(
        CLI::App* sub,
        std::string const& name,
        std::string const& description,
        std::string const& initial_value
    );

    std::string group_name() const
    {
        return "Output";
    }

    // -------------------------------------------------------------------------
    //     Run Functions
    // -------------------------------------------------------------------------

    /**
     * @brief Return the normalized output dir for the single dir "--out-dir".
     */
    std::string out_dir() const;

    /**
     * @brief Return the normalized output dir for the named dir "--name-out-dir".
     */
    std::string out_dir( std::string const& name ) const;

    /**
     * @brief Check whether any of the files in @p filenames exist in the the single dir "--out-dir".
     * If so, print an error message and throw an error.
     *
     * Regex filenames are allowed, e.g., in order to check a range of files like `out_[0-9]+.txt`.
     */
    void check_nonexistent_output_files( std::vector<std::string> const& filenames ) const;

    /**
     * @brief Check whether any of the files in @p filenames exist in a named dir "--name-out-dir".
     * If so, print an error message and throw an error.
     *
     * Regex filenames are allowed, e.g., in order to check a range of files like `out_[0-9]+.txt`.
     */
    void check_nonexistent_output_files(
        std::vector<std::string> const& filenames, std::string const& name
    ) const;

    // -------------------------------------------------------------------------
    //     Option Members
    // -------------------------------------------------------------------------

private:

    // Map from dir names to paths.
    std::unordered_map<std::string, std::string> out_dirs_;

};

#endif // include guard
