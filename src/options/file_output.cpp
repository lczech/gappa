/*
    gappa - Genesis Applications for Phylogenetic Placement Analysis
    Copyright (C) 2017-2021 Lucas Czech

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

#include "options/file_output.hpp"

#include "genesis/utils/core/exception.hpp"
#include "genesis/utils/core/fs.hpp"
#include "genesis/utils/core/logging.hpp"
#include "genesis/utils/core/options.hpp"
#include "genesis/utils/text/string.hpp"

#include "options/global.hpp"
#include "tools/misc.hpp"

#include <algorithm>
#include <cassert>
#include <stdexcept>

// =================================================================================================
//      File Type/Name Setup Functions
// =================================================================================================

void FileOutputOptions::set_optionname( std::string const& optionname )
{
    internal_check(
        ! out_dir_option && ! prefix_option && ! suffix_option && ! compress_option,
        "Have to call set_optionname() before adding any option."
    );
    optionname_ = optionname;
}

void FileOutputOptions::set_group( std::string const& group )
{
    internal_check(
        ! out_dir_option && ! prefix_option && ! suffix_option && ! compress_option,
        "Have to call set_group() before adding any option."
    );
    group_ = group;
}

// =================================================================================================
//      Extra Setup Functions
// =================================================================================================

void FileOutputOptions::add_default_output_opts_to_app(
    CLI::App* sub,
    std::string const& output_dir_initial_value,
    std::string const& prefix_initial_value,
    std::string const& suffix_initial_value
) {
    add_output_dir_opt_to_app( sub, output_dir_initial_value );
    add_file_prefix_opt_to_app( sub, prefix_initial_value );
    add_file_suffix_opt_to_app( sub, suffix_initial_value );
}

CLI::Option* FileOutputOptions::add_output_dir_opt_to_app(
    CLI::App* sub,
    std::string const& initial_value
) {
    // Correct setup check.
    internal_check(
        out_dir_option == nullptr,
        "Cannot use the same FileOutputOptions object multiple times."
    );

    // Setup.
    auto const optname = "--" + optionname_ + ( optionname_.empty() ? "" : "-" ) + "out-dir";
    out_dir_ = initial_value;

    // Add option
    out_dir_option = sub->add_option(
        optname,
        out_dir_,
        "Directory to write " + optionname_ + ( optionname_.empty() ? "" : " " ) + "files to",
        true
    );
    // out_dir_option->check( CLI::ExistingDirectory );
    out_dir_option->group( group_ );

    return out_dir_option;
}

CLI::Option* FileOutputOptions::add_file_prefix_opt_to_app(
    CLI::App* sub,
    std::string const& initial_value
) {
    return add_filefix_opt_( sub, initial_value, "prefix", prefix_option, prefix_ );
}

CLI::Option* FileOutputOptions::add_file_suffix_opt_to_app(
    CLI::App* sub,
    std::string const& initial_value
) {
    return add_filefix_opt_( sub, initial_value, "suffix", suffix_option, suffix_ );
}

CLI::Option* FileOutputOptions::add_filefix_opt_(
    CLI::App* sub,
    std::string const& initial_value,
    std::string const& fixname,
    CLI::Option* target_opt,
    std::string& target_var
) {
    // Correct setup check.
    internal_check(
        target_opt == nullptr,
        "Cannot use the same FileOutputOptions object multiple times."
    );

    // Setup.
    auto const optname = "--" + optionname_ + ( optionname_.empty() ? "" : "-" ) + "file-" + fixname;
    target_var = initial_value;

    // Add option
    target_opt = sub->add_option(
        optname,
        target_var,
        "File " + fixname + " for " + ( optionname_.empty() ? "output" : optionname_ ) + " files",
        true
    );
    target_opt->check([fixname]( std::string const& fix ){
        if( ! genesis::utils::is_valid_filename( fix ) ) {
            return std::string(
                "File " + fixname + " contains invalid characters (`<>:\"\\/|?*`), non-printable " +
                "characters, or surrounding whitespace."
            );
        }
        return std::string();
    });
    target_opt->group( group_ );

    return target_opt;
}

CLI::Option* FileOutputOptions::add_file_compress_opt_to_app(
    CLI::App* sub
) {
    // Correct setup check.
    internal_check(
        compress_option == nullptr,
        "Cannot use the same FileOutputOptions object multiple times."
    );

    // Setup.
    auto const optname = "--" + optionname_ + ( optionname_.empty() ? "" : "-" ) + "compress";

    // TODO add optinal arguments for this: none, fastest, smallest, balanced, plus additonal
    // "block" or "multithreaded" or "parallelized" keyword --> make this a hidden option?

    // Add option
    compress_option = sub->add_flag(
        optname,
        compress_,
        "If set, compress the " + ( optionname_.empty() ? "output" : optionname_ ) +
        " files using gzip. Output file extensions are automatically extended by `.gz`."
    );
    compress_option->group( group_ );
    return compress_option;
}

// =================================================================================================
//      Output Filenames
// =================================================================================================

std::string FileOutputOptions::get_output_filename(
    std::string const& infix, std::string const& extension, bool with_dir
) const {
    // Get the normalized output dir (with trailing slash) and extenion (with leading dot,
    // so that empty extensions also work without introducing extra dots).
    // We then simply assert that the extension has no further dots, which we can do,
    // as we are the only ones setting extensions in this program (the user cannot chose them).
    auto const dir = ( with_dir ? genesis::utils::dir_normalize_path( out_dir_ ) : "" );
    auto const ext = ( extension.empty() || extension[0] == '.' ) ? extension : "." + extension;
    internal_check( ext.size() < 2 || ext[1] != '.', "Extension contains multiple leading dots." );

    return dir + prefix_ + infix + suffix_ + ext + ( compress_ ? ".gz" : "" );
}

void FileOutputOptions::check_output_files_nonexistence(
    std::string const& infix, std::string const& extension
) const {
    check_output_files_nonexistence(
        std::vector<std::pair<std::string, std::string>>{{ infix, extension }}
    );
}

void FileOutputOptions::check_output_files_nonexistence(
    std::string const& infix, std::vector<std::string> const& extensions
) const {
    std::vector<std::pair<std::string, std::string>> list;
    for( auto const& ext : extensions ) {
        list.emplace_back( infix, ext );
    }
    check_output_files_nonexistence( list );
}

void FileOutputOptions::check_output_files_nonexistence(
    std::vector<std::pair<std::string, std::string>> const& infixes_and_extensions
) const {
    using namespace genesis::utils;

    // Shortcut: if the dir is not created yet, there cannot be any existing files in it.
    // We do this check here, so that we can be sure later in this function that the dir
    // is there, so that listing it contents etc actually works.
    if( ! genesis::utils::dir_exists( out_dir_ ) ) {
        return;
    }

    // Helper function for reporting existing files
    auto report_file_ = [&]( std::string const& path ){
        // If we allow overwriting, only warn about the files.
        if( genesis::utils::Options::get().allow_file_overwriting() ) {
            LOG_WARN << "Warning: Output file already exists and will be overwritten: " + path;
            // LOG_BOLD;
        } else {
            throw genesis::except::ExistingFileError(
                "Output file already exists: " + path + "\nUse " + allow_file_overwriting_flag +
                " to allow gappa to overwrite the file.",
                path
            );
        }
    };

    // Get all files in the output dir.
    auto const dir_cont = dir_list_contents( out_dir_, false );

    // Go through all filenames without dir names that we want to check.
    // We use this as the outer loop to avoid recomputing file names.
    for( auto const& in_ex : infixes_and_extensions ) {

        // Get the file path for the current entry.
        auto const path = get_output_filename( in_ex.first, in_ex.second, false );

        // Check if it exists, report/throw if so. We do not use the dir list regex here,
        // as this would interfere with our automatic file names (parenthesis, dots, etc
        // that are all valid filename AND regex characters...). Instead, we do simple wildcards.
        for( auto const& dirfile : dir_cont ) {
            if( match_wildcards( dirfile, path ) ) {

                // If we have a match, report with a nice full file name with directory.
                auto const dir = genesis::utils::dir_normalize_path( out_dir_ );
                report_file_( get_output_filename( in_ex.first, in_ex.second ));
            }
        }
    }
}

// =================================================================================================
//      Output Targets
// =================================================================================================

std::shared_ptr<genesis::utils::BaseOutputTarget> FileOutputOptions::get_output_target(
    std::string const& infix, std::string const& extension
) const {
    using namespace genesis::utils;

    // Create dir if needed. This might create the dir also in cases were something fails later,
    // so we end up with an empty dir. This is however common in many other programs as well,
    // so let's not bother with this.
    dir_create( out_dir_, true );

    // Make an output target, optionally using gzip compression.
    return to_file(
        get_output_filename( infix, extension ),
        compress_ ? GzipCompressionLevel::kDefaultCompression : GzipCompressionLevel::kNoCompression
    );
}
