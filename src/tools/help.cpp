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

#include "tools/help.hpp"

#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

// =================================================================================================
//      Functions
// =================================================================================================

void print_wiki_help( CLI::App const& app, std::string prev )
{
    auto const subcomms = app.get_subcommands( false );
    auto const& opts = app.get_options();

    // We do not count the help option, so we need to manually check if there are any others.
    bool has_options = false;
    for( auto const& opt : opts ) {
        if( opt->get_name() != "-h,--help" ) {
            has_options = true;
        }
    }

    std::cout << "Subcommand: " << app.get_name() << "\n";
    std::cout << "================================\n\n";

    // Header for the command.
    if( prev.empty() ) {
        prev = app.get_name();
    } else {
        prev += " " + app.get_name();
    }
    std::cout << app.get_description() << "\n\n";
    std::cout << "Usage: `" << prev;
    if( has_options ) {
        std::cout << " [options]";
    }
    if( ! subcomms.empty() ) {
        if( app.get_require_subcommand_min() > 0 ) {
            std::cout << " subcommand";
        } else {
            std::cout << " [subcommand]";
        }
    }
    std::cout << "`\n\n";

    // Print the options of thus command.
    if( has_options ) {

        std::cout << "## Options\n\n";

        // map from group name to table contents.
        // we use a vec to keep order.
        // std::map<std::string, std::string> opt_helps;
        std::vector<std::pair<std::string, std::string>> opt_helps;

        // Add lines for each group.
        for( auto const& opt : opts ) {

            // Do not add help option.
            if( opt->get_name() == "-h,--help" ) {
                continue;
            }

            std::stringstream os;
            // os << "| <nobr>`" << opt->get_name() << "`</nobr> ";
            // os << "|";
            // if( opt->get_required() ) {
            //     os << " **Required.**";
            // }
            // if( ! opt->help_aftername().empty() ) {
            //     // print stuff without leading space.
            //     os << " `" << opt->help_aftername().substr( 1 ) << "`<br>";
            // }
            //
            // auto descr = opt->get_description();
            // if( descr.substr( 0, 10 ) == "Required. " ) {
            //     descr = descr.substr( 10 );
            // }
            // os << " " << descr << " |\n";
            // // os << " " << opt->get_description() << " |\n";
            // // os << "| " << opt->get_description() << " |\n";

            // Add content to the group help.
            os << "<tr><td><code>" << opt->get_name() << "</code></td>";
            os << "<td>";
            if( opt->get_required() ) {
                os << "<strong>Required.</strong>";
            }
            if( ! opt->help_aftername().empty() ) {
                // print stuff without leading space.
                auto han = opt->help_aftername().substr( 1 );
                auto const rp = han.find( " (REQUIRED)" );
                if( rp != std::string::npos ) {
                    han.erase( rp,  11 );
                }

                os << " <code>" << han << "</code><br>";
            }
            auto descr = opt->get_description();
            if( descr.substr( 0, 10 ) == "Required. " ) {
                descr = descr.substr( 10 );
            }
            os << " " << descr << "</td></tr>\n";
            // os << " " << opt->get_description() << " |\n";
            // os << "| " << opt->get_description() << " |\n";

            // Add content to the group help.
            // first check if the group was already used, and if not add it.
            auto get_group_content = [&]( std::string const& name ) -> std::string& {
                for( auto& elem : opt_helps ) {
                    if( elem.first == name ) {
                        return elem.second;
                    }
                }
                opt_helps.push_back({ name, "" });
                return opt_helps.back().second;
            };
            get_group_content( opt->get_group() ) += os.str();
            // opt_helps[ opt->get_group() ] += os.str();
        }

        // Print the groups and their tables
        // for( auto const& gr : opt_helps ) {
        //     std::cout << "**" << gr.first << ":**\n\n";
        //     std::cout << "| Option  | Description |\n";
        //     std::cout << "| ------- | ----------- |\n";
        //     // std::cout << "| Option  | Type | Description |\n";
        //     // std::cout << "| ------- | ---- | ----------- |\n";
        //     std::cout << gr.second << "\n";
        // }

        // Print the groups and their tables
        std::cout << "<table>\n";
        bool done_first_group = false;
        for( auto const& gr : opt_helps ) {
            if( done_first_group ) {
                std::cout << "<tr height=30px></tr>\n";
            }
            std::cout << "<thead><tr><th colspan=\"2\" align=\"left\">" << gr.first << "</th></tr></thead>\n";
            std::cout << "<tbody>\n";
            std::cout << gr.second;
            std::cout << "</tbody>\n";
            done_first_group = true;
        }
        std::cout << "</table>\n\n";
    }

    // Print the subcommands of this command.
    if( ! subcomms.empty() ) {

        std::cout << "## Subcommands\n\n";

        std::cout << "| Subcommand  | Description |\n";
        std::cout << "| ----------- | ----------- |\n";

        for( auto const& subcomm : subcomms ) {
            std::cout << "| [" << subcomm->get_name() << "](../wiki/Subcommand:-" << subcomm->get_name() << ") ";
            std::cout << "| " << subcomm->get_description() << " |\n";
        }
        std::cout << "\n";
    }

    std::cout << "## Description\n\n\n\n";

    // Recurse.
    if( ! subcomms.empty() ) {
        for( auto const& subcomm : subcomms ) {
            print_wiki_help( *subcomm, prev );
        }
    }
}
