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

    void add_output_dir_options( CLI::App* sub );

    // -------------------------------------------------------------------------
    //     Run Functions
    // -------------------------------------------------------------------------

    /**
     * @brief Return the normalized output dir provided by the user.
     */
    std::string out_dir() const;

    /**
     * @brief Check whether any of the files in @p filenames exist in the out dir.
     * If so, print an error message and throw an error.
     */
    void check_nonexistent_output_files( std::vector<std::string> const& filenames ) const;

    // -------------------------------------------------------------------------
    //     Option Members
    // -------------------------------------------------------------------------

private:

    std::string out_dir_ = ".";

};

#endif // include guard
