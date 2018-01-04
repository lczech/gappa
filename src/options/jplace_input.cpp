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

#include "options/jplace_input.hpp"

#include <iostream>
#include <stdexcept>

// =================================================================================================
//      Setup Functions
// =================================================================================================

void JplaceInputOptions::add_jplace_input_options( CLI::App* sub )
{
    // TODO put in options group. same for all other options
    // TODO add avg tree option?!

    add_file_input_options( sub, "jplace", "jplace" );
}

// =================================================================================================
//      Run Functions
// =================================================================================================

genesis::placement::Sample JplaceInputOptions::sample( size_t index ) const
{
    return reader_.from_file( input_file_path( index ) );
}

genesis::placement::SampleSet JplaceInputOptions::sample_set() const
{
    // TODO add sample( size_t ) function that just reads one file, using the proper jplace reader settings
    // TODO dont report errors in jplace. offer subcommand for that
    // TODO nope. also report them here. just not while reading, but use a validator function.
    // TODO offer avg tree option
    // TODO add/offer validity checks etc

    return reader_.from_files( input_file_paths() );
}
