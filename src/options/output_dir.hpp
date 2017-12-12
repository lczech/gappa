#ifndef GAPPA_OPTIONS_OUTPUT_DIR_H_
#define GAPPA_OPTIONS_OUTPUT_DIR_H_

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

#include "genesis/utils/core/fs.hpp"

#include <string>
#include <vector>

// =================================================================================================
//      Output Directory Options
// =================================================================================================

class OutputDirOptions
{
public:

    // -------------------------------------------------------------------------
    //     Constructor and Rule of Five
    // -------------------------------------------------------------------------

    OutputDirOptions()  = default;
    virtual ~OutputDirOptions() = default;

    OutputDirOptions( OutputDirOptions const& other ) = default;
    OutputDirOptions( OutputDirOptions&& )            = default;

    OutputDirOptions& operator= ( OutputDirOptions const& other ) = default;
    OutputDirOptions& operator= ( OutputDirOptions&& )            = default;

    // -------------------------------------------------------------------------
    //     Setup Functions
    // -------------------------------------------------------------------------

    void add_output_dir_options( CLI::App* sub )
    {
        sub->add_option(
            "--out-dir",
            out_dir_,
            "Directory to write files to",
            true
        )->check( CLI::ExistingDirectory );

        // TODO instead of expecting an existing dir, create it if needed.
        // TODO add function to overwrite files, which sets the genesis option for this
    }

    // -------------------------------------------------------------------------
    //     Run Functions
    // -------------------------------------------------------------------------

    /**
     * @brief Return the normalized output dir provided by the user.
     */
    std::string out_dir() const
    {
        return genesis::utils::dir_normalize_path( out_dir_ );
    }

    /**
     * @brief Check whether any of the files in @p filenames exist in the out dir.
     * If so, print an error message and throw an error.
     */
    void check_nonexistent_output_files( std::vector<std::string> const& filenames ) const
    {
        using namespace genesis::utils;

        std::string dir = dir_normalize_path( out_dir_ );
        for( auto const& file : filenames ) {
            if( file_exists( dir + file ) ) {
                throw CLI::ValidationError(
                    "--out-dir (" + out_dir_ +  ")", "Output file already exists: " + file
                );
            }
        }
    }

    // -------------------------------------------------------------------------
    //     Option Members
    // -------------------------------------------------------------------------

    std::string out_dir_ = ".";

};

#endif // include guard
