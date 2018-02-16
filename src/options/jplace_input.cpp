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

#include "genesis/placement/function/functions.hpp"

#include <iostream>
#include <stdexcept>

// =================================================================================================
//      Setup Functions
// =================================================================================================

CLI::Option* JplaceInputOptions::add_jplace_input_opt_to_app( CLI::App* sub, bool required )
{
    // TODO add avg tree option?!

    return FileInputOptions::add_multi_file_input_opt_to_app( sub, "jplace", "jplace", required, "Jplace Input" );
}

CLI::Option* JplaceInputOptions::add_point_mass_opt_to_app( CLI::App* sub )
{
    return sub->add_flag(
        "--point-mass",
        point_mass_,
        "Treat every pquery as a point mass concentrated on the highest-weight placement."
    )->group( "Jplace Input" );
}

// =================================================================================================
//      Run Functions
// =================================================================================================

genesis::placement::Sample JplaceInputOptions::sample( size_t index ) const
{
    using namespace genesis;
    using namespace genesis::placement;

    auto sample = reader_.from_file( file_path( index ) );
    if( point_mass_ ) {
        filter_n_max_weight_placements( sample );
        normalize_weight_ratios( sample );
    }
    return sample;
}

genesis::placement::SampleSet JplaceInputOptions::sample_set() const
{
    // TODO dont report errors in jplace. offer subcommand for that
    // TODO nope. also report them here. just not while reading, but use a validator function.
    // TODO offer avg tree option
    // TODO add/offer validity checks etc

    auto set = reader_.from_files( file_paths() );
    if( point_mass_ ) {
        for( auto& sample : set ) {
            filter_n_max_weight_placements( sample.sample );
            normalize_weight_ratios( sample.sample );
        }
    }
    return set;
}
