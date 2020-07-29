//
//
// -----------------------------------------------------------------------------
// File: usage_config.h
// -----------------------------------------------------------------------------
//
// This file defines the main usage reporting configuration interfaces and
// documents abel's supported built-in usage flags. If these flags are found
// when parsing a command-line, abel will exit the program and display
// appropriate help messages.
#ifndef ABEL_CONFIG_USAGE_CONFIG_H_
#define ABEL_CONFIG_USAGE_CONFIG_H_

#include <functional>
#include <string>
#include <abel/strings/string_view.h>

// -----------------------------------------------------------------------------
// Built-in Usage Flags
// -----------------------------------------------------------------------------
//
// abel supports the following built-in usage flags. When passed, these flags
// exit the program and :
//
// * --help
//     Shows help on important flags for this binary
// * --helpfull
//     Shows help on all flags
// * --helpshort
//     Shows help on only the main module for this program
// * --helppackage
//     Shows help on all modules in the main package
// * --version
//     Shows the version and build info for this binary and exits
// * --only_check_args
//     Exits after checking all flags
// * --helpon
//     Shows help on the modules named by this flag value
// * --helpmatch
//     Shows help on modules whose name contains the specified substring

namespace abel {


    namespace flags_internal {
        using flag_kind_filter = std::function<bool(abel::string_view)>;
    }  // namespace flags_internal

// flags_usage_config
//
// This structure contains the collection of callbacks for changing the behavior
// of the usage reporting routines in abel Flags.
    struct flags_usage_config {
        // Returns true if flags defined in the given source code file should be
        // reported with --helpshort flag. For example, if the file
        // "path/to/my/code.cc" defines the flag "--my_flag", and
        // contains_helpshort_flags("path/to/my/code.cc") returns true, invoking the
        // program with --helpshort will include information about --my_flag in the
        // program output.
        flags_internal::flag_kind_filter contains_helpshort_flags;

        // Returns true if flags defined in the filename should be reported with
        // --help flag. For example, if the file
        // "path/to/my/code.cc" defines the flag "--my_flag", and
        // contains_help_flags("path/to/my/code.cc") returns true, invoking the
        // program with --help will include information about --my_flag in the
        // program output.
        flags_internal::flag_kind_filter contains_help_flags;

        // Returns true if flags defined in the filename should be reported with
        // --helppackage flag. For example, if the file
        // "path/to/my/code.cc" defines the flag "--my_flag", and
        // contains_helppackage_flags("path/to/my/code.cc") returns true, invoking the
        // program with --helppackage will include information about --my_flag in the
        // program output.
        flags_internal::flag_kind_filter contains_helppackage_flags;

        // Generates std::string containing program version. This is the std::string reported
        // when user specifies --version in a command line.
        std::function<std::string()> version_string;

        // Normalizes the filename specific to the build system/filesystem used. This
        // routine is used when we report the information about the flag definition
        // location. For instance, if your build resides at some location you do not
        // want to expose in the usage output, you can trim it to show only relevant
        // part.
        // For example:
        //   normalize_filename("/my_company/some_long_path/src/project/file.cc")
        // might produce
        //   "project/file.cc".
        std::function<std::string(abel::string_view)> normalize_filename;
    };

// set_flags_usage_config()
//
// Sets the usage reporting configuration callbacks. If any of the callbacks are
// not set in usage_config instance, then the default value of the callback is
// used.
    void set_flags_usage_config(flags_usage_config usage_config);

    namespace flags_internal {

        flags_usage_config get_usage_config();

        void report_usage_error(abel::string_view msg, bool is_fatal);

    }  // namespace flags_internal

}  // namespace abel

extern "C" {

// Additional report of fatal usage error message before we std::exit. Error is
// fatal if is_fatal argument to report_usage_error is true.
void abel_report_fatal_usage_error(abel::string_view);

}  // extern "C"

#endif  // ABEL_CONFIG_USAGE_CONFIG_H_
