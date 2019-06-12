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

#include "tools/misc.hpp"

#include "genesis/utils/core/options.hpp"
#include "genesis/utils/text/string.hpp"

#include <cmath>
#include <cstdlib>
#include <stdexcept>

// =================================================================================================
//      Legacy Commands
// =================================================================================================

void add_legacy_command(
    CLI::App& app,
    std::string const& old_name,
    std::string const& new_path
) {
    auto sub = app.add_subcommand( old_name );
    sub->group("");
    sub->callback( [ new_path ]() {
        throw RenamedCommandError( "Command has been renamed to `gappa " + new_path + "`" );
    });
}

// =================================================================================================
//      Formatting
// =================================================================================================

std::string format_columns(
    std::string const& left,
    std::string const& right,
    size_t left_w
) {
    // If std out is a terminal, we use its width for the maximal line length.
    unsigned long twidth = 0;
    if( genesis::utils::Options::get().stdout_is_terminal() ) {
        twidth = static_cast<unsigned long>(
            genesis::utils::Options::get().terminal_size().first
        );
    }

    // Get the widths of the columns. If there is not room for the second one,
    // make it 0 length, meaning all is written in one line.
    auto const lwidth = left_w;
    auto const rwidth = ( twidth > lwidth ? twidth - lwidth : 0 );

    // Write.
    std::stringstream out;
    write_columns( out, left, right, lwidth, rwidth );
    return out.str();
}

void write_columns(
    std::ostream& out,
    std::string const& left,
    std::string const& right,
    size_t left_w,
    size_t right_w
) {
    // Write left column.
    out << std::setw(static_cast<int>( left_w )) << std::left << left;

    // Write right column.
    if( ! right.empty() ) {
        auto rcpy = right;

        // If the left is already longer than the column allows, start a new line.
        if( left.length() >= left_w ) {
            out << "\n" << std::setw(static_cast<int>( left_w )) << "";
        }

        // If we have an actual useful width for the right column, wrap it.
        // Otherwise, we just put everything in one long line.
        if( right_w > 0 ) {
            rcpy = genesis::utils::wrap( rcpy, right_w );
        }

        // Indent and then trim again. The trimming removes the leading whitespace,
        // which we do not want, as we already inserted enough, and it removes
        // the trailing new line from the wrapping, which we do not want, as we output
        // one later anyway.
        rcpy = genesis::utils::indent( rcpy, std::string( left_w, ' ' ));
        rcpy = genesis::utils::trim( rcpy );
        out << rcpy;
    }
    out << "\n";
}

// =================================================================================================
//      Misc
// =================================================================================================

std::string random_indexed_name( size_t index, size_t max )
{
    // Some safety. Just in case.
    if( index > max ) {
        throw std::runtime_error( "Internal error: Cannot generate random name." );
    }

    // Calculate needed length of the result string.
    // If we hit an even power of 26, we do not need to add the extra char.
    auto const ll = static_cast<size_t>( std::log( max ) / std::log( 26 ));
    auto const lp = static_cast<size_t>( std::pow( 26, ll ));
    size_t const len = ll + ( lp == max ? 0 : 1 );

    // Fill the chars of the result according to their index.
    auto result = std::string( len, '-' );
    for( size_t i = 0; i < len; ++i ) {
        auto c = index % 26;
        result[ len - i - 1 ] = 'a' + c;
        index /= 26;
    }
    return result;
}
