#ifndef GAPPA_TOOLS_CLI_FORMATTER_H_
#define GAPPA_TOOLS_CLI_FORMATTER_H_

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

#include "CLI/CLI.hpp"

#include "genesis/utils/core/options.hpp"
#include "genesis/utils/text/string.hpp"

#include <string>

// =================================================================================================
//      CLI11 Formatter
// =================================================================================================

class GappaFormatter : public CLI::Formatter
{
public:

    // -----------------------------------------------------------
    //     Overridden Formatter Functions
    // -----------------------------------------------------------

    virtual std::string make_subcommand( CLI::App const* sub) const override
    {
        auto const lcol = sub->get_name();
        auto const rcol = sub->get_description();
        return format_columns( lcol, rcol );
    }

    virtual std::string make_option( CLI::Option const* opt, bool is_positional ) const override
    {
        auto const lcol = make_option_name(opt, is_positional) + make_option_opts(opt);
        auto const rcol = make_option_desc(opt);
        return format_columns( lcol, rcol );
    }

    // -----------------------------------------------------------
    //     Helper Functions
    // -----------------------------------------------------------

protected:

    std::string format_columns( std::string const& left, std::string const& right ) const
    {
        // If std out is a terminal, we use its width for the maximal line length.
        unsigned long twidth = 0;
        if( genesis::utils::Options::get().stdout_is_terminal() ) {
            twidth = static_cast<unsigned long>(
                genesis::utils::Options::get().terminal_size().first
            );
        }

        // Get the widths of the columns. If there is not room for the second one,
        // make it 0 length, meaning all is written in one line.
        auto const lwidth = get_column_width();
        auto const rwidth = ( twidth > lwidth ? twidth - lwidth : 0 );

        // Write.
        std::stringstream out;
        write_columns( out, left, right, lwidth, rwidth );
        return out.str();
    }

    void write_columns(
        std::ostream& out,
        std::string left, std::string right,
        size_t left_w, size_t right_w
    ) const {
        // Write left column.
        left = "  " + left;
        out << std::setw(static_cast<int>( left_w )) << std::left << left;

        // Write right column.
        if( ! right.empty() ) {

            // If the left is already longer than the column allows, start a new line.
            if( left.length() >= left_w ) {
                out << "\n" << std::setw(static_cast<int>( left_w )) << "";
            }

            // If we have an actual useful width for the right column, wrap it.
            // Otherwise, we just put everything in one long line.
            if( right_w > 0 ) {
                right = genesis::utils::wrap( right, right_w );
            }

            // Indent and then trim again. The trimming removes the leading whitespace,
            // which we do not want, as we already inserted enough, and it removes
            // the trailing new line from the wrapping, which we do not want, as we output
            // one later anyway.
            right = genesis::utils::indent( right, std::string( left_w, ' ' ));
            right = genesis::utils::trim( right );
            out << right;
        }
        out << "\n";
    }

};

#endif // include guard