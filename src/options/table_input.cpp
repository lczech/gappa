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

#include "options/table_input.hpp"

#include "options/global.hpp"

#include "genesis/utils/core/algorithm.hpp"
#include "genesis/utils/core/fs.hpp"
#include "genesis/utils/io/input_source.hpp"
#include "genesis/utils/io/input_stream.hpp"
#include "genesis/utils/text/string.hpp"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <unordered_set>

// =================================================================================================
//      Setup Functions
// =================================================================================================

void TableInputOptions::add_table_input_opt_to_app(
    CLI::App* sub,
    bool required
) {
    table_input_opt = sub->add_option(
        "--" + name + ( name.empty() ? "" : "-" ) + "table-file",
        table_input_opt.value(),
        "Tabular char-separated input file."
    );
    table_input_opt.option()->group( group );

    if( required ) {
        table_input_opt.option()->required();
    }
    table_input_opt.option()->check( CLI::ExistingFile );
}

void TableInputOptions::add_separator_char_opt_to_app(
    CLI::App* sub
) {
    separator_char_opt = sub->add_option(
        "--" + name + ( name.empty() ? "" : "-" ) + "separator-char",
        separator_char_opt.value(),
        "Separator char for tabular data.",
        true
    )->transform(
        CLI::IsMember({ "comma", "tab", "space", "semicolon" }, CLI::ignore_case )
    );
    separator_char_opt.option()->group( group );
}

void TableInputOptions::add_column_selection_opts_to_app(
    CLI::App* sub
) {
    // Reconstruct name of sep char option.
    auto const scn = "--" + name + ( name.empty() ? "" : "-" ) + "separator-char";

    // Add two complementary ways of selecting columns.
    select_columns_opt = sub->add_option(
        "--" + name + ( name.empty() ? "" : "-" ) + "select-columns",
        select_columns_opt.value(),
        "Set the columns to select, by their name in the first (header) line of the table. "
        "All others columns are ignored. The options expects either a file with one column name "
        "per line, or an actual list of column names separated by " + scn,
        true
    );
    select_columns_opt.option()->group( group );
    ignore_columns_opt = sub->add_option(
        "--" + name + ( name.empty() ? "" : "-" ) + "ignore-columns",
        ignore_columns_opt.value(),
        "Set the columns to ignore, by their name in the first (header) line of the table. "
        "All others columns are selected. The options expects either a file with one column name "
        "per line, or an actual list of column names separated by " + scn,
        true
    );
    ignore_columns_opt.option()->group( group );

    // Make the ways mutally exclusive.
    select_columns_opt.option()->excludes( ignore_columns_opt.option() );
    ignore_columns_opt.option()->excludes( select_columns_opt.option() );
}

// =================================================================================================
//      Run Functions
// =================================================================================================

genesis::utils::CsvReader::Table TableInputOptions::read_table(
    bool use_header_line,
    bool always_include_first_column
) const {
    using namespace genesis::utils;
    auto reader = csv_reader();

    // If we do not use the header line, simply read everything and return it.
    if( ! use_header_line ) {
        return reader.read( from_file( table_input_opt.value() ));
    }

    // Otherwise, do line by line. First the header, then all remaining rows.
    InputStream table_is( from_file( table_input_opt.value() ));
    auto const header_line = reader.parse_line( table_is );

    // Get the columns that we want.
    auto col_idcs = get_column_indices_( header_line );
    if( col_idcs.empty() ) {
        throw std::runtime_error( "No columns selected at all from table." );
    }
    if( always_include_first_column && col_idcs[0] != 0 ) {
        col_idcs.insert( col_idcs.begin(), 0 );
    }

    // Add columns from header.
    genesis::utils::CsvReader::Table result;
    result.push_back({});
    for( auto idx : col_idcs ) {
        assert( idx < header_line.size() );
        result.back().push_back( header_line[idx] );
    }

    // Read all other lines.
    while( table_is ) {
        auto const line = reader.parse_line( table_is );
        if( line.size() == 0 ) {
            continue;
        }
        if( line.size() != header_line.size() ) {
            throw std::runtime_error( "Input table has lines with differing number of columns." );
        }

        result.push_back({});
        for( auto idx : col_idcs ) {
            assert( idx < line.size() );
            result.back().push_back( line[idx] );
        }
    }
    return result;
}

genesis::utils::Dataframe TableInputOptions::read_double_dataframe(
    bool filter_by_header_line
) const {
    using namespace genesis::utils;

    // Prepare a reader that can convert anything to double.
    // We filter out later.
    auto reader = DataframeReader<double>( csv_reader() );
    reader.parse_value_functor( []( std::string const& cell ){
        double v;
        try{
            v = std::stod( cell );
        } catch( ... ) {
            v = std::numeric_limits<double>::quiet_NaN();
        }
        return v;
    });

    // Do the reading.
    auto df = reader.read( from_file( table_input_opt.value() ));

    // Filter columns.
    if( filter_by_header_line ) {
        // Get the filtered list of column names, using the ones that are present in the dataframe.
        auto const column_names = get_column_names_( df.col_names() );

        // Remove the columns that are not in the list. Need to make copy of the names,
        // as removing from a thing while iterating it gives nasty segfaults!
        auto const col_names = df.col_names();
        for( auto const& cn : col_names ) {
            if( column_names.count( cn ) == 0 ) {
                df.remove_col( cn );
            }
        }
    }

    // Now check for any "empty" columns that just contains zeros or invalid values.
    // Those can result from metadata columns that are not numbers,
    // and cannot be used for methods that expect tables of double values. So, remove them.
    std::vector<std::string> rem_names;
    size_t i = 0;
    while( i < df.cols() ) {
        auto const& col = df[i].as<double>();
        auto const bad_vals = std::all_of( col.begin(), col.end(), []( double v ){
            return v == 0.0 || ! std::isfinite( v );
        });
        if( bad_vals ) {
            rem_names.push_back( col.name() );
            df.remove_col( i );
        } else {
            ++i;
        }
    }

    // Some user warning if we removed columns.
    if( rem_names.size() > 0 ) {
        LOG_WARN << "Warning: The following columns of the table file contained non-numerical "
                 << "data or only invalid values, which cannot be used here, and are hence ignored: ";
        for( auto const& e : rem_names ) {
            LOG_WARN << " - " << e;
        }
    }

    // TODO this should also be an option for the other reading functions here!

    // User output
    LOG_MSG1 << "Using table columns: " << join( df.col_names(), ", " );
    for( size_t i = 0; i < df.cols(); ++i ) {
        auto const& col = df[i].as<double>();
        size_t const cnt = std::count_if( col.begin(), col.end(), []( double v ){
            return std::isfinite( v );
        });
        if( cnt != df.rows() ) {
            LOG_MSG2 << " - " << df[i].name() << " (" << cnt << " of " << df.rows() << " valid values)";
        } else {
            LOG_MSG2 << " - " << df[i].name();
        }
    }

    return df;
}

genesis::utils::Dataframe TableInputOptions::read_string_dataframe(
    bool filter_by_header_line
) const {
    using namespace genesis::utils;

    // Do the reading.
    auto reader = DataframeReader<std::string>( csv_reader() );
    auto df = reader.read( from_file( table_input_opt.value() ));

    // Filter columns.
    if( filter_by_header_line ) {
        // Get the filtered list of column names, using the ones that are present in the dataframe.
        auto const column_names = get_column_names_( df.col_names() );

        // Remove the columns that are not in the list. Need to make copy of the names,
        // as removing from a thing while iterating it gives nasty segfaults!
        auto const col_names = df.col_names();
        for( auto const& cn : col_names ) {
            if( column_names.count( cn ) == 0 ) {
                df.remove_col( cn );
            }
        }
    }

    return df;
}

// =================================================================================================
//      Helper Functions
// =================================================================================================

std::string TableInputOptions::separator_char() const
{
    // Set char.
    if( separator_char_opt.value() == "comma" ) {
        return ",";
    } else if( separator_char_opt.value() == "tab" ) {
        return "\t";
    } else if( separator_char_opt.value() == "space" ) {
        return " ";
    } else if( separator_char_opt.value() == "semicolon" ) {
        return ";";
    } else {
        throw CLI::ValidationError(
            "--" + name + ( name.empty() ? "" : "-" ) + "separator-char",
            "Invalid separator char '" + separator_char_opt.value() + "'."
        );
    }
}

genesis::utils::CsvReader TableInputOptions::csv_reader() const
{
    auto reader = genesis::utils::CsvReader();
    reader.separator_chars( separator_char() );
    return reader;
}

bool TableInputOptions::check_row_names(
    genesis::utils::Dataframe const& df,
    std::vector<std::string> const&          row_names
) {

    // Helper function to sort a vector.
    auto sort_vec = []( std::vector<std::string> vec ){
        // std::for_each( vec.begin(), vec.end(), []( std::string& s ){ s = to_lower(s); });
        std::sort( vec.begin(), vec.end() );
        return vec;
    };

    // Check if the filenames match the metadata data rows. Compare vecs order-independently.
    auto const df_sci = sort_vec( df.row_names() );
    auto const bn_sci = sort_vec( row_names );

    return df_sci == bn_sci;
}

genesis::utils::Dataframe TableInputOptions::sort_rows(
    genesis::utils::Dataframe const& df,
    std::vector<std::string> const&          row_name_order
) {
    // We here simply make a sorted copy of the dataframe, because sorting inline is nasty.
    // This is not really a nice solution, but works for now.

    // TODO make this work for non-double dataframes as well!
    // Not needed right now, but might lead to trouble later...

    // Make a dataframe with the correct columns.
    genesis::utils::Dataframe res;
    for( auto const& col : df ) {
        res.add_col<double>( col.name() );
    }

    // Add the rows in the correct order, and fill in the values.
    for( auto const& row_name : row_name_order ) {
        res.add_row( row_name );
        auto const ridx = res.row_index( row_name );
        assert( ridx == res.rows() - 1 );

        auto const old_ridx = df.row_index( row_name );

        for( size_t cidx = 0; cidx < res.cols(); ++cidx ) {
            res.at( cidx ).get<double>( ridx ) = df.at( cidx ).get<double>( old_ridx );
        }
    }

    return res;
}

// =================================================================================================
//      Internal Functions
// =================================================================================================

std::vector<size_t> TableInputOptions::get_column_indices_(
    std::vector<std::string> const& header_line
) const {
    using namespace genesis::utils;

    std::vector<size_t> result;
    auto const col_names = get_column_names_( header_line );
    for( size_t i = 0; i < header_line.size(); ++i ) {
        if( col_names.count( header_line[i] ) > 0 ) {
            result.push_back(i);
        }
    }
    return result;
}

std::unordered_set<std::string> TableInputOptions::get_column_names_(
    std::vector<std::string> const& header_line
) const {
    using namespace genesis::utils;

    // Helper function to either read a file of columns, or parse the input as a list.
    // It returns
    auto get_col_list = [&]( std::string const& input )
    {
        // Unfortunately, both ways of input first produce a vector,
        // but we want fast lookup, so we have to convert to a set...
        std::vector<std::string> input_list;
        if( is_file( input )) {
            input_list = file_read_lines( input );
        } else {
            input_list = split( input, separator_char() );
        }
        std::unordered_set<std::string> result;
        for( auto const& e : input_list ) {
            if( result.count(e) > 0 ) {
                LOG_WARN << "Warning: Column name list contains duplicate entry '" << e << "'.";
            }
            result.insert(e);
        }
        return result;
    };

    // Check if header line contains duplicate entries.
    if( contains_duplicates( header_line ) ) {
        throw std::runtime_error( "Header line of the input table contains duplicate column names." );
    }

    // Temporaries to keep code duplication low.
    std::unordered_set<std::string> column_list;
    std::string option_name;

    // Fill a list of column indices that we want to have.
    std::unordered_set<std::string> result;
    if( ! select_columns_opt.value().empty() && ! ignore_columns_opt.value().empty() ) {

        // Edge case, should not happen, because CLI alreadu takes care of this.
        assert( false );
        throw std::runtime_error(
            "Internal Error: Cannot use select and ignore columns at the same time."
        );

    } else if( ! select_columns_opt.value().empty() ) {

        // Fill it with the ones that are present in the select columns list.
        option_name = "--" + name + ( name.empty() ? "" : "-" ) + "select-columns";
        column_list = get_col_list( select_columns_opt.value() );
        for( auto const& entry : header_line ) {
            if( column_list.count( entry ) > 0 ) {
                assert( result.count( entry ) == 0 );
                result.insert( entry );
                column_list.erase( entry );
            }
        }

    } else if( ! ignore_columns_opt.value().empty() ) {

        // Fill it with the ones that are NOT present in the ignore columns list.
        option_name = "--" + name + ( name.empty() ? "" : "-" ) + "ignore-columns";
        column_list = get_col_list( ignore_columns_opt.value() );
        for( auto const& entry : header_line ) {
            if( column_list.count( entry ) == 0 ) {
                assert( result.count( entry ) == 0 );
                result.insert( entry );
            } else {
                column_list.erase( entry );
            }
        }

    } else {

        // If neither is given, just list all indices.
        for( auto const& entry : header_line ) {
            assert( result.count( entry ) == 0 );
            result.insert( entry );
        }
    }

    // User warning if there are columns not found in the input file.
    if( column_list.size() > 0 ) {
        LOG_WARN << "Warning: There were columns given by " << option_name
                 << " that are not present in the input table:";
        for( auto const& e : column_list ) {
            LOG_WARN << " - " << e << "\n";
        }
    }

    // We need to have some columns, otherwise the table is useless anyway.
    if( result.empty() ) {
        throw std::runtime_error( "No columns selected at all from table." );
    }

    // Return indices of the selected columns
    return result;
}
