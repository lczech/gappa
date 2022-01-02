#ifndef GAPPA_TOOLS_CLI_OPTION_H_
#define GAPPA_TOOLS_CLI_OPTION_H_

/*
    gappa - Genesis Applications for Phylogenetic Placement Analysis
    Copyright (C) 2017-2022 Lucas Czech

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

#include <string>
#include <stdexcept>

// =================================================================================================
//      CLI11 Option Helper
// =================================================================================================

/**
 * @brief Helper that encapsulates an option for the command line interface,
 * storing its value and the CLI11 object used in the interface to change that value.
 */
template<typename T>
class CliOption
{
public:

    // ----------------------------------------------------
    //     Constructor and Rule of Five
    // ----------------------------------------------------

    CliOption() = default;

    /**
     * @brief Construct by forwarding to the value.
     */
    template<typename... Args>
    CliOption( Args&&... args )
        : value_( std::forward<Args>(args)... )
    {}

    ~CliOption() = default;

    CliOption( CliOption const& other ) = default;
    CliOption( CliOption&& )            = default;

    CliOption& operator= ( CliOption const& other ) = default;
    CliOption& operator= ( CliOption&& )            = default;

    // ----------------------------------------------------
    //     Setters
    // ----------------------------------------------------

    /**
     * @brief Set the option.
     */
    CliOption& operator =( CLI::Option* opt )
    {
        if( option_ ) {
            throw std::domain_error(
                "Internal error: Option set multiple times: " +
                option_->get_lnames()[0]
            );
        }
        option_ = opt;
        return *this;
    }

    /**
     * @brief Set the value.
     */
    template<typename... Args>
    CliOption& operator =( Args&&... args )
    {
        value_ = T( std::forward<Args>(args)... );
        return *this;
    }

    // ----------------------------------------------------
    //     Option Access
    // ----------------------------------------------------

    operator bool() const
    {
        return option_ && *option_;
    }

    operator CLI::Option*()
    {
        return option_;
    }

    operator CLI::Option const*() const
    {
        return option_;
    }

    CLI::Option* option()
    {
        return option_;
    }

    CLI::Option const* option() const
    {
        return option_;
    }

    // ----------------------------------------------------
    //     Value Access
    // ----------------------------------------------------

    operator T&()
    {
        return value_;
    }

    operator T const&() const
    {
        return value_;
    }

    T& value()
    {
        return value_;
    }

    T const& value() const
    {
        return value_;
    }

    // ----------------------------------------------------
    //     Member Variables
    // ----------------------------------------------------

private:

    CLI::Option* option_ = nullptr;
    T            value_  = {};

};

// /* *
//  * @brief Specialized version that allows construction from char arrays,
//  * so that we can easility initialize the class in standard use cases.
//  */
// template<>
// struct CliOption<std::string>
// {
//     CliOption() = default;
//
//     CliOption( std::string const& val )
//         : value( val )
//     {}
//
//     CliOption( char const* val )
//         : value( val )
//     {}
//
//     CliOption& operator =( CLI::Option* opt )
//     {
//         option = opt;
//         return *this;
//     }
//
//     // CliOption& operator =( std::string const& val )
//     // {
//     //     value = val;
//     //     return *this;
//     // }
//     //
//     // CliOption& operator =( char const* val )
//     // {
//     //     value = val;
//     //     return *this;
//     // }
//
//     std::string  value  = {};
//     CLI::Option* option = nullptr;
// };

#endif // include guard
