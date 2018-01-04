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

#include "options/sequence_input.hpp"

#include "genesis/sequence/sequence_set.hpp"
#include "genesis/utils/core/fs.hpp"

#include <iostream>
#include <stdexcept>

// =================================================================================================
//      Constructor and Constants
// =================================================================================================

// Fasta extensions: https://en.wikipedia.org/wiki/FASTA_format#File_extension
const std::string SequenceInputOptions::fasta_extensions_ = "fasta|fas|fsa|fna|ffn|faa|frn";
const std::string SequenceInputOptions::phylip_extensions_ = "phylip|phy";

SequenceInputOptions::SequenceInputOptions()
{
    fasta_reader_.to_upper( false );
    phylip_reader_.to_upper( false );
    phylip_reader_.mode( genesis::sequence::PhylipReader::Mode::kAutomatic );
}

// =================================================================================================
//      Setup Functions
// =================================================================================================

void SequenceInputOptions::add_sequence_input_options( CLI::App* sub )
{
    add_file_input_options( sub, "sequence", "(" + fasta_extensions_ + "|" + phylip_extensions_ + ")" );
}

void SequenceInputOptions::add_fasta_input_options( CLI::App* sub )
{
    add_file_input_options( sub, "sequence", "(" + fasta_extensions_ + ")" );
}

// =================================================================================================
//      Run Functions
// =================================================================================================

genesis::sequence::SequenceSet SequenceInputOptions::sequence_set( size_t index ) const
{
    using namespace genesis::sequence;
    using namespace genesis::utils;

    SequenceSet result;
    auto const& file_name = input_file_path( index );
    auto const ext = file_extension( file_name );

    if( ext == "phylip" || ext == "phy" ) {

        // Try phylip if extension says so. Return if successfull.
        try{
            phylip_reader_.from_file( file_name, result );
            return result;
        } catch ( ... ) {}

        // Otherwise try fasta, again returning on success.
        try{
            result.clear();
            fasta_reader_.from_file( file_name, result );
            return result;
        } catch ( ... ) {}

    } else {

        // For all other extensions, try fasta first. Return if successfull.
        try{
            fasta_reader_.from_file( file_name, result );
            return result;
        } catch ( ... ) {}

        // If this does not work, try phylip.
        try{
            result.clear();
            phylip_reader_.from_file( file_name, result );
            return result;
        } catch ( ... ) {}
    }

    // If we are here, none of the above worked.
    throw std::runtime_error(
        "Input file " + file_name + " cannot be read as either fasta or phylip."
    );
}

genesis::sequence::SequenceSet SequenceInputOptions::sequence_set_all() const
{
    using namespace genesis::sequence;
    SequenceSet result;
    for( size_t i = 0; i < input_file_count(); ++i ) {
        auto tmp = sequence_set( i );
        for( auto& seq : tmp ) {
            result.add( std::move( seq ));
        }
    }
    return result;
}
