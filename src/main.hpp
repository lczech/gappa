#ifndef GAPPA_MAIN_H_
#define GAPPA_MAIN_H_

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

#include <string>
#include <vector>

// =================================================================================================
//      Options
// =================================================================================================

class MainOptions
{
public:

    // -------------------------------------------------------------------------
    //     Setup Functions
    // -------------------------------------------------------------------------

    void add_main_options( CLI::App& app )
    {
        auto v_s = app.add_option(
            "--verbosity",
            verbosity_,
            "Verbosity level [0-3]",
            true
        );
        auto v_c = app.add_flag(
            "-v",
            verbosity_cnt_,
            "Verbosity; add multiple times for more (-vvv)"
        );
        v_s->excludes( v_c );
        v_c->excludes( v_s );
    }

    // -------------------------------------------------------------------------
    //     Run Functions
    // -------------------------------------------------------------------------

    size_t verbosity() const
    {
        return ( verbosity_cnt_ > 0 ? verbosity_cnt_ + 1 : verbosity_ );
    }

    // -------------------------------------------------------------------------
    //     Option Members
    // -------------------------------------------------------------------------

private:

    size_t verbosity_ = 1;
    size_t verbosity_cnt_ = 0;

};

#endif // include guard
