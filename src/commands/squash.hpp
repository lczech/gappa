#ifndef GAPPA_COMMANDS_SQUASH_H_
#define GAPPA_COMMANDS_SQUASH_H_

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

#include "options/jplace_input.hpp"
#include "options/general.hpp"
#include "options/output_dir.hpp"

#include <memory>
#include <string>
#include <vector>

// =================================================================================================
//      Options
// =================================================================================================

class SquashOptions
    : public JplaceInputOptions
    , public OutputDirOptions
{
public:

    bool point_mass = false;
    bool normalize = false; // TODO unused
};

// =================================================================================================
//      Functions
// =================================================================================================

void setup_squash( CLI::App& app, GeneralOptions const& opt_general );
void run_squash( SquashOptions const& options, GeneralOptions const& opt_general );

#endif // include guard
