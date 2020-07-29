//
//

#include <abel/config/internal/usage.h>

#include <map>
#include <string>

#include <abel/config/flag.h>
#include <abel/config/internal/path_util.h>
#include <abel/config/internal/program_name.h>
#include <abel/config/usage_config.h>
#include <abel/strings/ascii.h>
#include <abel/strings/str_cat.h>
#include <abel/strings/str_split.h>
#include <abel/strings/string_view.h>
#include <abel/thread/mutex.h>


ABEL_FLAG(bool, help, false,
          "show help on important flags for this binary [tip: all flags can "
          "have two dashes]");
ABEL_FLAG(bool, helpfull, false, "show help on all flags");
ABEL_FLAG(bool, helpshort, false,
          "show help on only the main module for this program");
ABEL_FLAG(bool, helppackage, false,
          "show help on all modules in the main package");
ABEL_FLAG(bool, version, false, "show version and build info and exit");
ABEL_FLAG(bool, only_check_args, false, "exit after checking all flags");
ABEL_FLAG(std::string, helpon, "",
          "show help on the modules named by this flag value");
ABEL_FLAG(std::string, helpmatch, "",
          "show help on modules whose name contains the specified substr");

namespace abel {

    namespace flags_internal {
        namespace {

            abel::string_view TypenameForHelp(const flags_internal::command_line_flag &flag) {
                // Only report names of v1 built-in types
#define HANDLE_V1_BUILTIN_TYPE(t) \
  if (flag.is_of_type<t>()) {       \
    return #t;                    \
  }

                HANDLE_V1_BUILTIN_TYPE(bool);
                HANDLE_V1_BUILTIN_TYPE(int32_t);
                HANDLE_V1_BUILTIN_TYPE(int64_t);
                HANDLE_V1_BUILTIN_TYPE(uint64_t);
                HANDLE_V1_BUILTIN_TYPE(double);
#undef HANDLE_V1_BUILTIN_TYPE

                if (flag.is_of_type<std::string>()) {
                    return "string";
                }

                return "";
            }

// This class is used to emit an XML element with `tag` and `text`.
// It adds opening and closing tags and escapes special characters in the text.
// For example:
// std::cout << XMLElement("title", "Milk & Cookies");
// prints "<title>Milk &amp; Cookies</title>"
            class XMLElement {
            public:
                XMLElement(abel::string_view tag, abel::string_view txt)
                        : tag_(tag), txt_(txt) {}

                friend std::ostream &operator<<(std::ostream &out,
                                                const XMLElement &xml_elem) {
                    out << "<" << xml_elem.tag_ << ">";

                    for (auto c : xml_elem.txt_) {
                        switch (c) {
                            case '"':
                                out << "&quot;";
                                break;
                            case '\'':
                                out << "&apos;";
                                break;
                            case '&':
                                out << "&amp;";
                                break;
                            case '<':
                                out << "&lt;";
                                break;
                            case '>':
                                out << "&gt;";
                                break;
                            default:
                                out << c;
                                break;
                        }
                    }

                    return out << "</" << xml_elem.tag_ << ">";
                }

            private:
                abel::string_view tag_;
                abel::string_view txt_;
            };

// --------------------------------------------------------------------
// Helper class to pretty-print info about a flag.

            class flag_help_pretty_printer {
            public:
                // Pretty printer holds on to the std::ostream& reference to direct an output
                // to that stream.
                flag_help_pretty_printer(int max_line_len, std::ostream *out)
                        : out_(*out),
                          max_line_len_(max_line_len),
                          line_len_(0),
                          first_line_(true) {}

                void Write(abel::string_view str, bool wrap_line = false) {
                    // Empty std::string - do nothing.
                    if (str.empty()) return;

                    std::vector<abel::string_view> tokens;
                    if (wrap_line) {
                        for (auto line : abel::string_split(str, abel::by_any_char("\n\r"))) {
                            if (!tokens.empty()) {
                                // Keep line separators in the input std::string.
                                tokens.push_back("\n");
                            }
                            for (auto token :
                                    abel::string_split(line, abel::by_any_char(" \t"), abel::skip_empty())) {
                                tokens.push_back(token);
                            }
                        }
                    } else {
                        tokens.push_back(str);
                    }

                    for (auto token : tokens) {
                        bool new_line = (line_len_ == 0);

                        // Respect line separators in the input std::string.
                        if (token == "\n") {
                            EndLine();
                            continue;
                        }

                        // Write the token, ending the std::string first if necessary/possible.
                        if (!new_line &&
                            (static_cast<size_t>(line_len_) + token.size() >= static_cast<size_t>(max_line_len_))) {
                            EndLine();
                            new_line = true;
                        }

                        if (new_line) {
                            StartLine();
                        } else {
                            out_ << ' ';
                            ++line_len_;
                        }

                        out_ << token;
                        line_len_ += token.size();
                    }
                }

                void StartLine() {
                    if (first_line_) {
                        out_ << "    ";
                        line_len_ = 4;
                        first_line_ = false;
                    } else {
                        out_ << "      ";
                        line_len_ = 6;
                    }
                }

                void EndLine() {
                    out_ << '\n';
                    line_len_ = 0;
                }

            private:
                std::ostream &out_;
                const int max_line_len_;
                int line_len_;
                bool first_line_;
            };

            void FlagHelpHumanReadable(const flags_internal::command_line_flag &flag,
                                       std::ostream *out) {
                flag_help_pretty_printer printer(80, out);  // Max line length is 80.

                // Flag name.
                printer.Write(abel::string_cat("--", flag.name()));

                // Flag help.
                printer.Write(abel::string_cat("(", flag.help(), ");"), /*wrap_line=*/true);

                // Flag data type (for V1 flags only).
                if (!flag.is_abel_flag() && !flag.is_retired()) {
                    printer.Write(abel::string_cat("type: ", TypenameForHelp(flag), ";"));
                }

                // The listed default value will be the actual default from the flag
                // definition in the originating source file, unless the value has
                // subsequently been modified using set_command_line_option() with mode
                // SET_FLAGS_DEFAULT.
                std::string dflt_val = flag.default_value();
                if (flag.is_of_type<std::string>()) {
                    dflt_val = abel::string_cat("\"", dflt_val, "\"");
                }
                printer.Write(abel::string_cat("default: ", dflt_val, ";"));

                if (flag.is_modified()) {
                    std::string curr_val = flag.current_value();
                    if (flag.is_of_type<std::string>()) {
                        curr_val = abel::string_cat("\"", curr_val, "\"");
                    }
                    printer.Write(abel::string_cat("currently: ", curr_val, ";"));
                }

                printer.EndLine();
            }

// Shows help for every filename which matches any of the filters
// If filters are empty, shows help for every file.
// If a flag's help message has been stripped (e.g. by adding '#define
// STRIP_FLAG_HELP 1' then this flag will not be displayed by '--help'
// and its variants.
            void FlagsHelpImpl(std::ostream &out, flags_internal::flag_kind_filter filter_cb,
                               help_format format, abel::string_view program_usage_message) {
                if (format == help_format::kHumanReadable) {
                    out << flags_internal::short_program_invocation_name() << ": "
                        << program_usage_message << "\n\n";
                } else {
                    // XML schema is not a part of our public API for now.
                    out << "<?xml version=\"1.0\"?>\n"
                        << "<!-- This output should be used with care. We do not report type "
                           "names for flags with user defined types -->\n"
                        << "<!-- Prefer flag only_check_args for validating flag inputs -->\n"
                        // The document.
                        << "<AllFlags>\n"
                        // The program name and usage.
                        << XMLElement("program", flags_internal::short_program_invocation_name())
                        << '\n'
                        << XMLElement("usage", program_usage_message) << '\n';
                }

                // Map of package name to
                //   map of file name to
                //     vector of flags in the file.
                // This map is used to output matching flags grouped by package and file
                // name.
                std::map<std::string,
                        std::map<std::string,
                                std::vector<const flags_internal::command_line_flag *>>>
                        matching_flags;

                flags_internal::for_each_flag([&](flags_internal::command_line_flag *flag) {
                    std::string flag_filename = flag->file_name();

                    // Ignore retired flags.
                    if (flag->is_retired()) return;

                    // If the flag has been stripped, pretend that it doesn't exist.
                    if (flag->help() == flags_internal::kStrippedFlagHelp) return;

                    // Make sure flag satisfies the filter
                    if (!filter_cb || !filter_cb(flag_filename)) return;

                    matching_flags[std::string(flags_internal::package(flag_filename))]
                    [flag_filename]
                            .push_back(flag);
                });

                abel::string_view
                        package_separator;             // controls blank lines between packages.
                abel::string_view file_separator;  // controls blank lines between files.
                for (const auto &package : matching_flags) {
                    if (format == help_format::kHumanReadable) {
                        out << package_separator;
                        package_separator = "\n\n";
                    }

                    file_separator = "";
                    for (const auto &flags_in_file : package.second) {
                        if (format == help_format::kHumanReadable) {
                            out << file_separator << "  Flags from " << flags_in_file.first
                                << ":\n";
                            file_separator = "\n";
                        }

                        for (const auto *flag : flags_in_file.second) {
                            flags_internal::flag_help(out, *flag, format);
                        }
                    }
                }

                if (format == help_format::kHumanReadable) {
                    if (filter_cb && matching_flags.empty()) {
                        out << "  No modules matched: use -helpfull\n";
                    }
                } else {
                    // The end of the document.
                    out << "</AllFlags>\n";
                }
            }

        }  // namespace

// --------------------------------------------------------------------
// Produces the help message describing specific flag.
        void flag_help(std::ostream &out, const flags_internal::command_line_flag &flag,
                      help_format format) {
            if (format == help_format::kHumanReadable)
                flags_internal::FlagHelpHumanReadable(flag, &out);
        }

// --------------------------------------------------------------------
// Produces the help messages for all flags matching the filter.
// If filter is empty produces help messages for all flags.
        void flags_help(std::ostream &out, abel::string_view filter, help_format format,
                       abel::string_view program_usage_message) {
            flags_internal::flag_kind_filter filter_cb = [&](abel::string_view filename) {
                return filter.empty() || filename.find(filter) != abel::string_view::npos;
            };
            flags_internal::FlagsHelpImpl(out, filter_cb, format, program_usage_message);
        }

// --------------------------------------------------------------------
// Checks all the 'usage' command line flags to see if any have been set.
// If so, handles them appropriately.
        int handle_usage_flags(std::ostream &out,
                             abel::string_view program_usage_message) {
            if (abel::get_flag(FLAGS_helpshort)) {
                flags_internal::FlagsHelpImpl(
                        out, flags_internal::get_usage_config().contains_helpshort_flags,
                        help_format::kHumanReadable, program_usage_message);
                return 1;
            }

            if (abel::get_flag(FLAGS_helpfull)) {
                // show all options
                flags_internal::flags_help(out, "", help_format::kHumanReadable,
                                          program_usage_message);
                return 1;
            }

            if (!abel::get_flag(FLAGS_helpon).empty()) {
                flags_internal::flags_help(
                        out, abel::string_cat("/", abel::get_flag(FLAGS_helpon), "."),
                        help_format::kHumanReadable, program_usage_message);
                return 1;
            }

            if (!abel::get_flag(FLAGS_helpmatch).empty()) {
                flags_internal::flags_help(out, abel::get_flag(FLAGS_helpmatch),
                                          help_format::kHumanReadable,
                                          program_usage_message);
                return 1;
            }

            if (abel::get_flag(FLAGS_help)) {
                flags_internal::FlagsHelpImpl(
                        out, flags_internal::get_usage_config().contains_help_flags,
                        help_format::kHumanReadable, program_usage_message);

                out << "\nTry --helpfull to get a list of all flags.\n";

                return 1;
            }

            if (abel::get_flag(FLAGS_helppackage)) {
                flags_internal::FlagsHelpImpl(
                        out, flags_internal::get_usage_config().contains_helppackage_flags,
                        help_format::kHumanReadable, program_usage_message);

                out << "\nTry --helpfull to get a list of all flags.\n";

                return 1;
            }

            if (abel::get_flag(FLAGS_version)) {
                if (flags_internal::get_usage_config().version_string)
                    out << flags_internal::get_usage_config().version_string();
                // Unlike help, we may be asking for version in a script, so return 0
                return 0;
            }

            if (abel::get_flag(FLAGS_only_check_args)) {
                return 0;
            }

            return -1;
        }

    }  // namespace flags_internal

}  // namespace abel
