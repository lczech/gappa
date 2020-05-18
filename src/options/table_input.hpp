#ifndef GAPPA_OPTIONS_TABLE_INPUT_H_
#define GAPPA_OPTIONS_TABLE_INPUT_H_

/*
    gappa - Genesis Applications for Phylogenetic Placement Analysis
    Copyright (C) 2017-2020 Lucas Czech and HITS gGmbH

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
#include "tools/cli_option.hpp"

#include "genesis/utils/containers/dataframe.hpp"
#include "genesis/utils/containers/dataframe/reader.hpp"
#include "genesis/utils/formats/csv/reader.hpp"

#include <string>
#include <unordered_set>
#include <vector>

// =================================================================================================
//      Table Input Options
// =================================================================================================

/**
 * @brief Input tables in CSV formats, for example as a simple vector of strings, or as a Dataframe.
 */
class TableInputOptions
{
public:

    // -------------------------------------------------------------------------
    //     Constructor and Rule of Five
    // -------------------------------------------------------------------------

    TableInputOptions()  = default;
    TableInputOptions( std::string const& name )
        : name(name)
    {}
    TableInputOptions( std::string const& name, std::string const& group )
        : name(name)
        , group(group)
    {}

    ~TableInputOptions() = default;

    TableInputOptions( TableInputOptions const& other ) = delete;
    TableInputOptions( TableInputOptions&& )            = delete;

    TableInputOptions& operator= ( TableInputOptions const& other ) = delete;
    TableInputOptions& operator= ( TableInputOptions&& )            = delete;

    // -------------------------------------------------------------------------
    //     Setup Functions
    // -------------------------------------------------------------------------

    void add_table_input_opt_to_app( CLI::App* sub, bool required = false );

    void add_separator_char_opt_to_app( CLI::App* sub );
    void add_column_selection_opts_to_app( CLI::App* sub );

    // -------------------------------------------------------------------------
    //     Run Functions
    // -------------------------------------------------------------------------

public:

    /**
     * @brief Read a table file as a simple Csv table.
     *
     * The result is a vector of lines, where each line is a vector of strings for the cells.
     *
     * Using @p filter_by_header_line, the first line of the table is used as header,
     * and filtered by the options given here (`select_columns_opt` and `ignore_columns_opt`).
     * If set to false instead, all of the table is read, without taking the header line into account.
     *
     * If furthermore @p always_include_first_column is given, the first column is always included
     * in the result, even if this is not the result of `select_columns_opt` or `ignore_columns_opt`.
     * This parameter is only used if @p filter_by_header_line is also true, because otherwise,
     * the whole table (including the first column) is returned anyway.
     */
    genesis::utils::CsvReader::Table read_table(
        bool filter_by_header_line = true,
        bool always_include_first_column = false
    ) const;

    /**
     * @brief Read a table as a double Dataframe.
     *
     * Using @p filter_by_header_line, the first line of the table is used as header,
     * and filtered by the options given here (`select_columns_opt` and `ignore_columns_opt`).
     * If set to false instead, all of the table is read, without taking the header line into account.
     */
    genesis::utils::Dataframe read_double_dataframe(
        bool filter_by_header_line = true
    ) const;

    /**
     * @brief Red a table as a string Dataframe.
     *
     * Using @p filter_by_header_line, the first line of the table is used as header,
     * and filtered by the options given here (`select_columns_opt` and `ignore_columns_opt`).
     * If set to false instead, all of the table is read, without taking the header line into account.
     */
    genesis::utils::Dataframe read_string_dataframe(
        bool filter_by_header_line = true
    ) const;

    // -------------------------------------------------------------------------
    //     Helper Functions
    // -------------------------------------------------------------------------

    /**
     * @brief Get the actual separator char to use for the CSV Reader.
     *
     * This differs from the option itself, which takes the name of the separator char instead,
     * such as "comma" or "tab".
     */
    std::string separator_char() const;

    /**
     * @brief Get a CSV Reader with all options allpied to to.
     */
    genesis::utils::CsvReader csv_reader() const;

    /**
     * @brief Return whether the row names of a dataframe are the same as the given list of names,
     * order independently.
     */
    static bool check_row_names(
        genesis::utils::Dataframe const& df,
        std::vector<std::string> const&  row_names
    );

    /**
     * @brief Sort the rows of a Dataframe by a given order.
     */
    static genesis::utils::Dataframe sort_rows(
        genesis::utils::Dataframe const& df,
        std::vector<std::string> const&  row_name_order
    );

    // -------------------------------------------------------------------------
    //     Internal Functions
    // -------------------------------------------------------------------------

protected:

    /**
     * @brief Given the current settings of this TableInput class, parse a header line from a csv
     * file, and return the indices of the selected columns.
     *
     * The function ues the `select_columns_opt` and `ignore_columns_opt` inputs,
     * uses `separator_char_opt` to split them, and then compares against the given @p header_line
     * to get the indices of the matching column names.
     * Warns if the given selected/ignored names are not found in the header_line.
     */
    std::vector<size_t> get_column_indices_(
        std::vector<std::string> const& header_line
    ) const;

    /**
     * @brief Given the current settings of this TableInput class, parse a header line from a csv
     * file, and return the names of the selected columns.
     *
     * The function only ever returns column names that are actually in the given @p header_line,
     * but filters them according to the `select_columns_opt` and `ignore_columns_opt` inputs.
     * It warns if these two settings contain duplicate entries or column names that are not
     * found in the @p header_line.
     *
     * Also, @p header_line is checked for duplicates, in which case an exception is thrown,
     * as we cannot process a table with non-unique column names.
     */
    std::unordered_set<std::string> get_column_names_(
        std::vector<std::string> const& header_line
    ) const;

    // -------------------------------------------------------------------------
    //     Option Members
    // -------------------------------------------------------------------------

public:

    std::string const name = "";
    std::string const group = "Table Input";

    CliOption<std::string> table_input_opt    = "";
    CliOption<std::string> separator_char_opt = "comma";
    CliOption<std::string> select_columns_opt = "";
    CliOption<std::string> ignore_columns_opt = "";

};

#endif // include guard
