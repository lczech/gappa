#ifndef GAPPA_TOOLS_MISC_H_
#define GAPPA_TOOLS_MISC_H_

/*
    gappa - Genesis Applications for Phylogenetic Placement Analysis
    Copyright (C) 2017-2023 Lucas Czech

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
    Lucas Czech <lczech@carnegiescience.edu>
    Department of Plant Biology, Carnegie Institution For Science
    260 Panama Street, Stanford, CA 94305, USA
*/

#include "CLI/CLI.hpp"

#include <iosfwd>
#include <string>
#include <stdexcept>

// =================================================================================================
//      Legacy Commands
// =================================================================================================

class RenamedCommandError : public std::runtime_error {

public:

    RenamedCommandError( std::string message )
        : std::runtime_error( message )
    {}
};

void add_legacy_command(
    CLI::App& app,
    std::string const& old_name,
    std::string const& new_path
);

// =================================================================================================
//      Formatting
// =================================================================================================

std::string format_columns(
    std::string const& left,
    std::string const& right,
    size_t left_width
);

void write_columns(
    std::ostream& out,
    std::string const& left,
    std::string const& right,
    size_t left_width,
    size_t right_width
);

// =================================================================================================
//      Misc
// =================================================================================================

std::string random_indexed_name( size_t index, size_t max );

/**
 * @brief Alternative for normal `assert()` that allows to specify an error message,
 * throws an exception instead of terminating, and is always used, also in release mode.
 */
inline void internal_check(
    bool condition,
    std::string const& error_message
) {
    if( ! condition ) {
        throw std::domain_error(
            "Internal error: " + ( error_message.empty() ? "unknown error" : error_message )
        );
    }
}

#endif // include guard
