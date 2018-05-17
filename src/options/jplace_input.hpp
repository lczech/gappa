#ifndef GAPPA_OPTIONS_JPLACE_INPUT_H_
#define GAPPA_OPTIONS_JPLACE_INPUT_H_

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

#include "CLI/CLI.hpp"

#include "options/file_input.hpp"

#include "genesis/placement/sample.hpp"
#include "genesis/placement/sample_set.hpp"
#include "genesis/placement/formats/jplace_reader.hpp"

#include <string>
#include <vector>

// =================================================================================================
//      Jplace Input Options
// =================================================================================================

/**
 * @brief Helper class to add multiple jplace file input options to a command.
 */
class JplaceInputOptions
    : public FileInputOptions
{
public:

    // -------------------------------------------------------------------------
    //     Constructor and Rule of Five
    // -------------------------------------------------------------------------

    JplaceInputOptions()  = default;
    virtual ~JplaceInputOptions() = default;

    JplaceInputOptions( JplaceInputOptions const& other ) = default;
    JplaceInputOptions( JplaceInputOptions&& )            = default;

    JplaceInputOptions& operator= ( JplaceInputOptions const& other ) = default;
    JplaceInputOptions& operator= ( JplaceInputOptions&& )            = default;

    // -------------------------------------------------------------------------
    //     Setup Functions
    // -------------------------------------------------------------------------

    CLI::Option* add_jplace_input_opt_to_app( CLI::App* sub, bool required = true );

    CLI::Option* add_point_mass_opt_to_app( CLI::App* sub );
    CLI::Option* add_ignore_multiplicities_opt_to_app( CLI::App* sub );
    CLI::Option* add_absolute_mass_opt_to_app( CLI::App* sub );

    // -------------------------------------------------------------------------
    //     Run Functions
    // -------------------------------------------------------------------------

    /**
     * @brief Read in the jplace files at @p index in the list of input files and return it.
     *
     * See FileInputOptions::file_count() for the number of input files (valid range for the index)
     * and FileInputOptions::file_paths() for their list.
     */
    genesis::placement::Sample sample( size_t index ) const;

    /**
     * @brief Read in all jplace files given by the user and return them as a SampleSet.
     */
    genesis::placement::SampleSet sample_set() const;

    /**
     * @brief Read in all jplace files given by the user and merge all their pqueries them into a sample.
     *
     * This expects that all use the same reference tree. Otherwise, the function throws.
     */
    genesis::placement::Sample merged_samples() const;

    /**
     * @brief Return the JplaceReader used for the convenience functions.
     *
     * By modifying the settings of the reader before calling sample() or sample_set(),
     * the reading behaviour can be customized if needed for a program.
     */
    genesis::placement::JplaceReader& reader()
    {
        return reader_;
    }

    genesis::placement::JplaceReader const& reader() const
    {
        return reader_;
    }

    bool point_mass() const
    {
        return point_mass_;
    }

    bool ignore_multiplicities() const
    {
        return ignore_multiplicities_;
    }

    bool absolute_mass() const
    {
        return absolute_mass_;
    }

    virtual void print() const override;

    // -------------------------------------------------------------------------
    //     Option Members
    // -------------------------------------------------------------------------

private:

    genesis::placement::JplaceReader reader_;

    bool point_mass_            = false;
    bool ignore_multiplicities_ = false;
    bool absolute_mass_         = false;

public:

    CLI::Option* jplace_input_option          = nullptr;
    CLI::Option* point_mass_option            = nullptr;
    CLI::Option* ignore_multiplicities_option = nullptr;
    CLI::Option* absolute_mass_option         = nullptr;

};

#endif // include guard
