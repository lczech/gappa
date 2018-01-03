#ifndef GAPPA_OPTIONS_FILE_INPUT_H_
#define GAPPA_OPTIONS_FILE_INPUT_H_

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
#include <vector>

// =================================================================================================
//      Sequence Input Options
// =================================================================================================

class FileInputOptions
{
public:

    // -------------------------------------------------------------------------
    //     Constructor and Rule of Five
    // -------------------------------------------------------------------------

    FileInputOptions()  = default;
    virtual ~FileInputOptions() = default;

    FileInputOptions( FileInputOptions const& other ) = default;
    FileInputOptions( FileInputOptions&& )            = default;

    FileInputOptions& operator= ( FileInputOptions const& other ) = default;
    FileInputOptions& operator= ( FileInputOptions&& )            = default;

    // -------------------------------------------------------------------------
    //     Setup Functions
    // -------------------------------------------------------------------------

protected:

    /**
     * @brief Add the options to an App.
     *
     * Takes a file type used for help messages, and an extension for valid files.
     * The extenion can be a regex, e.g., `(fas|fasta)`.
     */
    void add_file_input_options(
        CLI::App* sub, std::string const& type, std::string const& extension
    );

    // -------------------------------------------------------------------------
    //     Run Functions
    // -------------------------------------------------------------------------

public:

    /**
     * @brief Get the resolved full file paths of all files provided by the user.
     *
     * This function uses the list of paths given by the user.
     * For each of them, it checks whether it is a file or a directory.
     * Files are immediately added to the result list, while directories are scanned
     * for any files with the extension in them, which are then added to the result list.
     *
     * This allows the users to hand over either their own files (no matter their extension), or
     * whole directory paths for convenience, which then however only use files ending in the
     * extension.
     */
    std::vector<std::string> const& input_file_paths() const;

    /**
     * @brief Get the number of files that were provided by the user.
     *
     * Returns `input_file_paths().size()`.
     */
    size_t input_file_count() const;

    /**
     * @brief Get a specific file from the list.
     *
     * Returns `input_file_paths().at( index )`.
     */
    std::string const& input_file_path( size_t index ) const;

    /**
     * @brief Print some user output related to these options.
     */
    void input_files_print() const;

    /**
     * @brief Return the list of paths as provided by the user, that is, without processing.
     */
    std::vector<std::string> const& input_file_cli_paths() const;

    /**
     * @brief Get the file names of the provided files, i.e., without directory and ending.
     *
     * This function calls genesis::utils::file_basename() and genesis::utils::file_filename() for
     * all paths. The result is for example useful for user output.
     */
    std::vector<std::string> input_files_base_file_names() const;

    // -------------------------------------------------------------------------
    //     Option Members
    // -------------------------------------------------------------------------

private:

    std::vector<std::string> raw_paths_;
    mutable std::vector<std::string> resolved_paths_;

    std::string file_type_;
    std::string file_ext_;

};

#endif // include guard
