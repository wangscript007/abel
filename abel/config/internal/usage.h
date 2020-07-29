//

#ifndef ABEL_CONFIG_INTERNAL_USAGE_H_
#define ABEL_CONFIG_INTERNAL_USAGE_H_

#include <iosfwd>
#include <string>

#include <abel/config/declare.h>
#include <abel/config/internal/command_line_flag.h>
#include <abel/strings/string_view.h>

// --------------------------------------------------------------------
// Usage reporting interfaces

namespace abel {

    namespace flags_internal {

// The format to report the help messages in.
        enum class help_format {
            kHumanReadable,
        };

// Outputs the help message describing specific flag.
        void flag_help(std::ostream &out, const flags_internal::command_line_flag &flag,
                      help_format format = help_format::kHumanReadable);

// Produces the help messages for all flags matching the filter. A flag matches
// the filter if it is defined in a file with a filename which includes
// filter string as a substring. You can use '/' and '.' to restrict the
// matching to a specific file names. For example:
//   flags_help(out, "/path/to/file.");
// restricts help to only flags which resides in files named like:
//  .../path/to/file.<ext>
// for any extension 'ext'. If the filter is empty this function produces help
// messages for all flags.
        void flags_help(std::ostream &out, abel::string_view filter,
                       help_format format, abel::string_view program_usage_message);

// --------------------------------------------------------------------

// If any of the 'usage' related command line flags (listed on the bottom of
// this file) has been set this routine produces corresponding help message in
// the specified output stream and returns:
//  0 - if "version" or "only_check_flags" flags were set and handled.
//  1 - if some other 'usage' related flag was set and handled.
// -1 - if no usage flags were set on a commmand line.
// Non negative return values are expected to be used as an exit code for a
// binary.
        int handle_usage_flags(std::ostream &out,
                             abel::string_view program_usage_message);

    }  // namespace flags_internal

}  // namespace abel

ABEL_DECLARE_FLAG(bool, help);
ABEL_DECLARE_FLAG(bool, helpfull);
ABEL_DECLARE_FLAG(bool, helpshort);
ABEL_DECLARE_FLAG(bool, helppackage);
ABEL_DECLARE_FLAG(bool, version);
ABEL_DECLARE_FLAG(bool, only_check_args);
ABEL_DECLARE_FLAG(std::string, helpon);
ABEL_DECLARE_FLAG(std::string, helpmatch);

#endif  // ABEL_CONFIG_INTERNAL_USAGE_H_
