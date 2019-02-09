/*
    gappa - Genesis Applications for Phylogenetic Placement Analysis
    Copyright (C) 2017-2019 Lucas Czech and HITS gGmbH

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

#include "options/matrix_output.hpp"

#include "genesis/utils/containers/matrix/operators.hpp"
#include "genesis/utils/core/fs.hpp"
#include "genesis/utils/text/string.hpp"

#include <iostream>
#include <stdexcept>

// =================================================================================================
//      Setup Functions
// =================================================================================================

void MatrixOutputOptions::add_matrix_output_opts_to_app(
    CLI::App* sub,
    std::string const& name,
    bool offer_triangular_mode,
    bool offer_omit_labels
) {
    name_ = name;

    add_output_dir_opt_to_app( sub, name, ".", "Matrix Output" );
    add_file_prefix_opt_to_app( sub, name, name, "Matrix Output" );

    // Add output format.
    auto formats = std::set<std::string>{ "matrix", "list" };
    if( offer_triangular_mode ) {
        formats.insert( "triangular" );
    }
    sub->add_set_ignore_case(
        "--" + name + ( name.empty() ? "" : "-" ) + "matrix-format",
        format_,
        formats,
        "Format of the output matrix file.",
        true
    )->group( "Matrix Output" );

    // Add label setting
    if( offer_omit_labels ) {
        sub->add_flag(
            "--omit-" + name + ( name.empty() ? "" : "-" ) + "matrix-labels",
            omit_labels_,
            "If set, the output matrix is written without column and row labels."
        )->group( "Matrix Output" );
    }
}

// =================================================================================================
//      Run Functions
// =================================================================================================

std::string MatrixOutputOptions::output_filename() const
{
    return file_prefix() + ( name_.empty() ? "matrix" : name_ ) + ".csv";
}

void MatrixOutputOptions::check_nonexistent_output_files() const
{
    FileOutputOptions::check_nonexistent_output_files({ output_filename() });
}

void MatrixOutputOptions::write_matrix(
    genesis::utils::Matrix<double> const& mat,
    std::vector<std::string> const& row_names,
    std::vector<std::string> const& col_names,
    std::string const& corner
) const {
    using namespace genesis;
    using namespace genesis::utils;

    // TODO offer function for dataframe, for other matrix types etc
    // TODO add double presicison
    // TODO add separator char

    auto const filename = out_dir() + output_filename();
    auto writer = MatrixWriter<double>();

    // Set output format.
    if( format_ == "matrix" ) {
        writer.format( MatrixWriter<double>::Format::kMatrix );
    } else if( format_ == "list" ) {
        writer.format( MatrixWriter<double>::Format::kList );
    } else if( format_ == "triangular" ) {
        writer.format( MatrixWriter<double>::Format::kTriangular );
    } else {
        throw CLI::ValidationError(
            "--" + name_ + ( name_.empty() ? "" : "-" ) + "matrix-format", "Invalid format '" + format_ + "'."
        );
    }

    // Do the writing
    if( omit_labels_ ) {
        writer.to_file( mat, filename );
    } else {
        writer.to_file( mat, filename, row_names, col_names, corner );
    }
}
