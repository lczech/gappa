#ifndef GAPPA_OPTIONS_MATRIX_OUTPUT_H_
#define GAPPA_OPTIONS_MATRIX_OUTPUT_H_

/*
    gappa - Genesis Applications for Phylogenetic Placement Analysis
    Copyright (C) 2017-2020 Lucas Czech and HITS gGmbH

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

#include "options/file_output.hpp"

#include "genesis/utils/containers/matrix.hpp"
#include "genesis/utils/containers/matrix/writer.hpp"

#include <string>
#include <vector>

// =================================================================================================
//      Matrix Output Options
// =================================================================================================

/**
 * @brief
 */
class MatrixOutputOptions
{
public:

    // -------------------------------------------------------------------------
    //     Constructor and Rule of Five
    // -------------------------------------------------------------------------

    MatrixOutputOptions()  = default;
    ~MatrixOutputOptions() = default;

    MatrixOutputOptions( MatrixOutputOptions const& other ) = default;
    MatrixOutputOptions( MatrixOutputOptions&& )            = default;

    MatrixOutputOptions& operator= ( MatrixOutputOptions const& other ) = default;
    MatrixOutputOptions& operator= ( MatrixOutputOptions&& )            = default;

    // -------------------------------------------------------------------------
    //     Setup Functions
    // -------------------------------------------------------------------------

    void add_matrix_output_opts_to_app(
        CLI::App* sub,
        std::string const& name = "",
        bool offer_triangular_mode = true,
        bool offer_omit_labels = true
    );

    // -------------------------------------------------------------------------
    //     Run Functions
    // -------------------------------------------------------------------------

    void write_matrix(
        std::shared_ptr<genesis::utils::BaseOutputTarget> target,
        genesis::utils::Matrix<double> const& mat,
        std::vector<std::string> const& row_names = {},
        std::vector<std::string> const& col_names = {},
        std::string const& corner = ""
    ) const;

    // void write_matrix( genesis::utils::Matrix<std::string> const& mat ) const;

    // -------------------------------------------------------------------------
    //     Option Members
    // -------------------------------------------------------------------------

private:

    std::string name_ = "";
    std::string format_ = "matrix";
    bool omit_labels_ = false;

};

#endif // include guard
