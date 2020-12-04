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

#include "options/matrix_output.hpp"

#include "genesis/utils/containers/matrix/operators.hpp"
#include "genesis/utils/core/fs.hpp"
#include "genesis/utils/io/output_target.hpp"
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
    std::string const group = "Matrix Output";

    // Add output format.
    auto formats = std::set<std::string>{ "matrix", "list" };
    if( offer_triangular_mode ) {
        formats.insert( "triangular" );
    }
    sub->add_option(
        "--" + name + ( name.empty() ? "" : "-" ) + "matrix-format",
        format_,
        "Format of the output matrix file.",
        true
    )->group( group )
    ->transform(
        CLI::IsMember( formats, CLI::ignore_case )
    );

    // Add label setting
    if( offer_omit_labels ) {
        sub->add_flag(
            "--omit-" + name + ( name.empty() ? "" : "-" ) + "matrix-labels",
            omit_labels_,
            "If set, the output matrix is written without column and row labels."
        )->group( group );
    }
}

// =================================================================================================
//      Run Functions
// =================================================================================================

void MatrixOutputOptions::write_matrix(
    std::shared_ptr<genesis::utils::BaseOutputTarget> target,
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
        writer.write( mat, target);
    } else {
        writer.write( mat, target, row_names, col_names, corner );
    }
}
