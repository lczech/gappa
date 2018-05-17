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

#include "options/global.hpp"

#include "genesis/placement/function/functions.hpp"
#include "genesis/placement/function/masses.hpp"
#include "genesis/utils/core/fs.hpp"
#include "genesis/utils/text/string.hpp"

#include <iostream>
#include <stdexcept>

#ifdef GENESIS_OPENMP
#   include <omp.h>
#endif

// =================================================================================================
//      Setup Functions
// =================================================================================================

CLI::Option* JplaceInputOptions::add_jplace_input_opt_to_app( CLI::App* sub, bool required )
{
    // TODO add avg tree option?!

    // Correct setup check.
    if( jplace_input_option != nullptr ) {
        throw std::domain_error( "Cannot set up the same JplaceInputOptions object multiple times." );
    }

    jplace_input_option = FileInputOptions::add_multi_file_input_opt_to_app(
        sub, "jplace", "jplace", required, "Input"
    );
    return jplace_input_option;
}

CLI::Option* JplaceInputOptions::add_point_mass_opt_to_app( CLI::App* sub )
{
    // Correct setup check.
    if( point_mass_option != nullptr ) {
        throw std::domain_error( "Cannot set up --point-mass option multiple times." );
    }

    point_mass_option = sub->add_flag(
        "--point-mass",
        point_mass_,
        "Treat every pquery as a point mass concentrated on the highest-weight placement."
    )->group( "Settings" );

    return point_mass_option;
}

CLI::Option* JplaceInputOptions::add_ignore_multiplicities_opt_to_app( CLI::App* sub )
{
    // Correct setup check.
    if( ignore_multiplicities_option != nullptr ) {
        throw std::domain_error( "Cannot set up --ignore-multiplicities option multiple times." );
    }

    // Currently, this is a hidden option, because it goes too deep for the average user.
    ignore_multiplicities_option = sub->add_flag(
        "--ignore-multiplicities",
        ignore_multiplicities_,
        "Set the multiplicity of each pquery to 1."
    )->group( "" );
    // )->group( "Settings" );

    return ignore_multiplicities_option;
}

CLI::Option* JplaceInputOptions::add_absolute_mass_opt_to_app( CLI::App* sub )
{
    // Correct setup check.
    if( absolute_mass_option != nullptr ) {
        throw std::domain_error( "Cannot set up --absolute-mass option multiple times." );
    }

    absolute_mass_option = sub->add_flag(
        "--absolute-mass",
        absolute_mass_,
        "Do not normalize the placement masses per sample, "
        "but use the absolute mass of all pqueries in the sample."
    )->group( "Settings" );

    return absolute_mass_option;
}

// =================================================================================================
//      Run Functions
// =================================================================================================

genesis::placement::Sample JplaceInputOptions::sample( size_t index ) const
{
    using namespace genesis;
    using namespace genesis::placement;

    // Do the reading.
    auto sample = reader_.from_file( file_path( index ) );

    // Point mass: remove all but the most likely placement, and set its weight to one.
    if( point_mass_option && point_mass_ ) {
        filter_n_max_weight_placements( sample );
        normalize_weight_ratios( sample );
    }

    // Ignore multiplicities: normalize each pquery so that it has a multiplicity of one.
    if( ignore_multiplicities_option && ignore_multiplicities_ ) {
        for( auto& pquery : sample ) {
            auto const tm = total_multiplicity( pquery );
            for( auto& name : pquery.names() ) {
                name.multiplicity /= tm;
            }
        }
    }

    // No absolute mass: use relative mass, that is, normalize the masses by the total of the sample.
    // We use the multiplicity for the normalization, as this does not affect mehtods that rely
    // on LWRs close to 1.
    if( absolute_mass_option && ! absolute_mass_ ) {
        auto const tm = total_placement_mass_with_multiplicities( sample );
        for( auto& pquery : sample ) {
            for( auto& name : pquery.names() ) {
                name.multiplicity /= tm;
            }

            // Alternative: normalize by changing the LWRs directly.
            // for( auto& placement : pquery.placements() ) {
            //     placement.like_weight_ratio /= tm;
            // }
        }
    }

    return sample;
}

genesis::placement::SampleSet JplaceInputOptions::sample_set() const
{
    using namespace genesis;
    using namespace genesis::placement;

    // TODO dont report errors in jplace. offer subcommand for that
    // TODO nope. also report them here. just not while reading, but use a validator function.
    // TODO offer avg tree option
    // TODO add/offer validity checks etc

    // The following is copied from the JplaceReader class of genesis,
    // but adds progress report to it. A bit ugly, but we can live with it for now.

    // Result.
    SampleSet set;
    auto const paths = file_paths();
    size_t fc = 0;

    // Make a vector of default-constructed Samples of the needed size.
    // We do this so that the order of input jplace files is kept.
    auto tmp = std::vector<Sample>( paths.size() );

    // Parallel parsing.
    #pragma omp parallel for schedule(dynamic)
    for( size_t fi = 0; fi < paths.size(); ++fi ) {

        // User output.
        if( global_options.verbosity() >= 2 ) {
            #pragma omp critical(GAPPA_JPLACE_INPUT_PROGRESS)
            {
                ++fc;
                std::cout << "Reading file " << fc << " of " << paths.size();
                std::cout << ": " << paths[ fi ] << "\n";
            }
        }

        tmp[ fi ] = sample( fi );
    }

    // Move to target SampleSet.
    for( size_t fi = 0; fi < paths.size(); ++fi ) {
        auto const name = base_file_name( fi );
        set.add( std::move( tmp[fi] ), name );
    }

    return set;
}

genesis::placement::Sample JplaceInputOptions::merged_samples() const
{
    using namespace genesis;
    using namespace genesis::placement;

    Sample result;
    size_t fc = 0;

    // Read all jplace files and accumulate their pqueries.
    #pragma omp parallel for schedule(dynamic)
    for( size_t fi = 0; fi < file_count(); ++fi ) {

        // User output.
        if( global_options.verbosity() >= 2 ) {
            #pragma omp critical(GAPPA_JPLACE_INPUT_PROGRESS)
            {
                ++fc;
                std::cout << "Reading file " << fc << " of " << file_count();
                std::cout << ": " << file_path( fi ) << "\n";
            }
        }

        // Read in file. This is the part that can trivially be done in parallel.
        auto smpl = sample( fi );

        // The main merging is single threaded.
        #pragma omp critical(GAPPA_JPLACE_INPUT_ACCUMULATE)
        {
            // Merge
            if( result.empty() ) {
                result = std::move( smpl );
            } else {
                try{
                    // The function only throws if something is wrong with the trees.
                    copy_pqueries( smpl, result );
                } catch( ... ) {
                    throw std::runtime_error( "Input jplace files have differing reference trees." );
                }
            }

        }
    }

    return result;
}

void JplaceInputOptions::print() const
{
    // Info about normalization and stuff.
    if( global_options.verbosity() >= 2 ) {
        std::vector<std::string> usings;
        if( point_mass_option && point_mass_ ) {
            usings.push_back( "--point-mass" );
        }
        if( ignore_multiplicities_option && ignore_multiplicities_ ) {
            usings.push_back( "--ignore-multiplicities" );
        }
        if( absolute_mass_option && ! absolute_mass_ ) {
            usings.push_back( "--absolute-mass" );
        }
        if( ! usings.empty() ) {
            std::cout << "Using " << genesis::utils::join( usings, ", " ) << "\n";
        }
    }

    // Print the file list.
    FileInputOptions::print();
}
