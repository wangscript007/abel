//
// Copyright(c) 2015 Gabi Melman.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#pragma once

#include <abel/log/details/fmt_helper.h>
#include <abel/log/details/log_msg.h>
#include <abel/format/format.h>
#include <abel/log/formatter.h>
#include <abel/system/process.h>
#include <abel/chrono/time.h>
#include <abel/log/details/file_helper.h>
#include <array>
#include <chrono>
#include <ctime>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace abel {
namespace details {

class flag_formatter {
public:
    virtual ~flag_formatter () = default;
    virtual void format (const details::log_msg &msg, const std::tm &tm_time, fmt::memory_buffer &dest) = 0;
};

///////////////////////////////////////////////////////////////////////
// name & level pattern appenders
///////////////////////////////////////////////////////////////////////
class name_formatter : public flag_formatter {
    void format (const details::log_msg &msg, const std::tm &, fmt::memory_buffer &dest) override {
        fmt_helper::append_str(*msg.logger_name, dest);
    }
};

// log level appender
class level_formatter : public flag_formatter {
    void format (const details::log_msg &msg, const std::tm &, fmt::memory_buffer &dest) override {
        fmt_helper::append_c_str(level::to_c_str(msg.level), dest);
    }
};

// short log level appender
class short_level_formatter : public flag_formatter {
    void format (const details::log_msg &msg, const std::tm &, fmt::memory_buffer &dest) override {
        fmt_helper::append_c_str(level::to_short_c_str(msg.level), dest);
    }
};

///////////////////////////////////////////////////////////////////////
// Date time pattern appenders
///////////////////////////////////////////////////////////////////////

static const char *ampm (const tm &t) {
    return t.tm_hour >= 12 ? "PM" : "AM";
}

static unsigned int to12h (const tm &t) {
    return t.tm_hour > 12 ? t.tm_hour - 12 : t.tm_hour;
}

// Abbreviated weekday name
static const char *days[] {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
class a_formatter : public flag_formatter {
    void format (const details::log_msg &, const std::tm &tm_time, fmt::memory_buffer &dest) override {
        fmt_helper::append_c_str(days[tm_time.tm_wday], dest);
    }
};

// Full weekday name
static const char *full_days[] {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
class A_formatter : public flag_formatter {
    void format (const details::log_msg &, const std::tm &tm_time, fmt::memory_buffer &dest) override {
        fmt_helper::append_c_str(full_days[tm_time.tm_wday], dest);
    }
};

// Abbreviated month
static const char *months[] {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sept", "Oct", "Nov", "Dec"};
class b_formatter : public flag_formatter {
    void format (const details::log_msg &, const std::tm &tm_time, fmt::memory_buffer &dest) override {
        fmt_helper::append_c_str(months[tm_time.tm_mon], dest);
    }
};

// Full month name
static const char *full_months[] {
    "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November",
    "December"};
class B_formatter : public flag_formatter {
    void format (const details::log_msg &, const std::tm &tm_time, fmt::memory_buffer &dest) override {
        fmt_helper::append_c_str(full_months[tm_time.tm_mon], dest);
    }
};

// Date and time representation (Thu Aug 23 15:35:46 2014)
class c_formatter ABEL_INHERITANCE_FINAL : public flag_formatter {
    void format (const details::log_msg &, const std::tm &tm_time, fmt::memory_buffer &dest) override {
        // fmt::format_to(dest, "{} {} {} ", days[tm_time.tm_wday],
        // months[tm_time.tm_mon], tm_time.tm_mday);
        // date
        fmt_helper::append_str(days[tm_time.tm_wday], dest);
        dest.push_back(' ');
        fmt_helper::append_str(months[tm_time.tm_mon], dest);
        dest.push_back(' ');
        fmt_helper::append_int(tm_time.tm_mday, dest);
        dest.push_back(' ');
        // time

        fmt_helper::pad2(tm_time.tm_hour, dest);
        dest.push_back(':');
        fmt_helper::pad2(tm_time.tm_min, dest);
        dest.push_back(':');
        fmt_helper::pad2(tm_time.tm_sec, dest);
        dest.push_back(' ');
        fmt_helper::append_int(tm_time.tm_year + 1900, dest);
    }
};

// year - 2 digit
class C_formatter ABEL_INHERITANCE_FINAL : public flag_formatter {
    void format (const details::log_msg &, const std::tm &tm_time, fmt::memory_buffer &dest) override {
        fmt_helper::pad2(tm_time.tm_year % 100, dest);
    }
};

// Short MM/DD/YY date, equivalent to %m/%d/%y 08/23/01
class D_formatter ABEL_INHERITANCE_FINAL : public flag_formatter {
    void format (const details::log_msg &, const std::tm &tm_time, fmt::memory_buffer &dest) override {
        fmt_helper::pad2(tm_time.tm_mon + 1, dest);
        dest.push_back('/');
        fmt_helper::pad2(tm_time.tm_mday, dest);
        dest.push_back('/');
        fmt_helper::pad2(tm_time.tm_year % 100, dest);
    }
};

// year - 4 digit
class Y_formatter ABEL_INHERITANCE_FINAL : public flag_formatter {
    void format (const details::log_msg &, const std::tm &tm_time, fmt::memory_buffer &dest) override {
        fmt_helper::append_int(tm_time.tm_year + 1900, dest);
    }
};

// month 1-12
class m_formatter ABEL_INHERITANCE_FINAL : public flag_formatter {
    void format (const details::log_msg &, const std::tm &tm_time, fmt::memory_buffer &dest) override {
        fmt_helper::pad2(tm_time.tm_mon + 1, dest);
    }
};

// day of month 1-31
class d_formatter ABEL_INHERITANCE_FINAL : public flag_formatter {
    void format (const details::log_msg &, const std::tm &tm_time, fmt::memory_buffer &dest) override {
        fmt_helper::pad2(tm_time.tm_mday, dest);
    }
};

// hours in 24 format 0-23
class H_formatter ABEL_INHERITANCE_FINAL : public flag_formatter {
    void format (const details::log_msg &, const std::tm &tm_time, fmt::memory_buffer &dest) override {
        fmt_helper::pad2(tm_time.tm_hour, dest);
    }
};

// hours in 12 format 1-12
class I_formatter ABEL_INHERITANCE_FINAL : public flag_formatter {
    void format (const details::log_msg &, const std::tm &tm_time, fmt::memory_buffer &dest) override {
        fmt_helper::pad2(to12h(tm_time), dest);
    }
};

// minutes 0-59
class M_formatter ABEL_INHERITANCE_FINAL : public flag_formatter {
    void format (const details::log_msg &, const std::tm &tm_time, fmt::memory_buffer &dest) override {
        fmt_helper::pad2(tm_time.tm_min, dest);
    }
};

// seconds 0-59
class S_formatter ABEL_INHERITANCE_FINAL : public flag_formatter {
    void format (const details::log_msg &, const std::tm &tm_time, fmt::memory_buffer &dest) override {
        fmt_helper::pad2(tm_time.tm_sec, dest);
    }
};

// milliseconds
class e_formatter ABEL_INHERITANCE_FINAL : public flag_formatter {
    void format (const details::log_msg &msg, const std::tm &, fmt::memory_buffer &dest) override {
        static constexpr abel::duration one_sec = abel::seconds(1);
        auto millis = abel::to_duration(msg.time);
        fmt_helper::pad3(static_cast<int>(to_int64_milliseconds(millis % one_sec)), dest);
    }
};

// microseconds
class f_formatter ABEL_INHERITANCE_FINAL : public flag_formatter {
    void format (const details::log_msg &msg, const std::tm &, fmt::memory_buffer &dest) override {
        static constexpr abel::duration one_sec = abel::seconds(1);
        auto micros = abel::to_duration(msg.time);
        fmt_helper::pad6(static_cast<int>(abel::to_int64_microseconds(micros % one_sec)), dest);
    }
};

// nanoseconds
class F_formatter ABEL_INHERITANCE_FINAL : public flag_formatter {
    void format (const details::log_msg &msg, const std::tm &, fmt::memory_buffer &dest) override {
        static constexpr abel::duration one_sec = abel::seconds(1);
        auto ns = abel::to_duration(msg.time);
        fmt::format_to(dest, "{:09}", abel::to_int64_nanoseconds(ns % one_sec));
    }
};

// seconds since epoch
class E_formatter ABEL_INHERITANCE_FINAL : public flag_formatter {
    void format (const details::log_msg &msg, const std::tm &, fmt::memory_buffer &dest) override {
        auto duration = abel::to_duration(msg.time);
        fmt_helper::append_int(abel::to_int64_seconds(duration), dest);
    }
};

// AM/PM
class p_formatter ABEL_INHERITANCE_FINAL : public flag_formatter {
    void format (const details::log_msg &, const std::tm &tm_time, fmt::memory_buffer &dest) override {
        fmt_helper::append_c_str(ampm(tm_time), dest);
    }
};

// 12 hour clock 02:55:02 pm
class r_formatter ABEL_INHERITANCE_FINAL : public flag_formatter {
    void format (const details::log_msg &, const std::tm &tm_time, fmt::memory_buffer &dest) override {
        fmt_helper::pad2(to12h(tm_time), dest);
        dest.push_back(':');
        fmt_helper::pad2(tm_time.tm_min, dest);
        dest.push_back(':');
        fmt_helper::pad2(tm_time.tm_sec, dest);
        dest.push_back(' ');
        fmt_helper::append_c_str(ampm(tm_time), dest);
    }
};

// 24-hour HH:MM time, equivalent to %H:%M
class R_formatter ABEL_INHERITANCE_FINAL : public flag_formatter {
    void format (const details::log_msg &, const std::tm &tm_time, fmt::memory_buffer &dest) override {
        fmt_helper::pad2(tm_time.tm_hour, dest);
        dest.push_back(':');
        fmt_helper::pad2(tm_time.tm_min, dest);
    }
};

// ISO 8601 time format (HH:MM:SS), equivalent to %H:%M:%S
class T_formatter ABEL_INHERITANCE_FINAL : public flag_formatter {
    void format (const details::log_msg &, const std::tm &tm_time, fmt::memory_buffer &dest) override {
        // fmt::format_to(dest, "{:02}:{:02}:{:02}", tm_time.tm_hour,
        // tm_time.tm_min, tm_time.tm_sec);
        fmt_helper::pad2(tm_time.tm_hour, dest);
        dest.push_back(':');
        fmt_helper::pad2(tm_time.tm_min, dest);
        dest.push_back(':');
        fmt_helper::pad2(tm_time.tm_sec, dest);
    }
};

// ISO 8601 offset from UTC in timezone (+-HH:MM)
class z_formatter ABEL_INHERITANCE_FINAL : public flag_formatter {
public:
    const std::chrono::seconds cache_refresh = std::chrono::seconds(5);

    z_formatter () = default;
    z_formatter (const z_formatter &) = delete;
    z_formatter &operator = (const z_formatter &) = delete;

    void format (const details::log_msg &msg, const std::tm &tm_time, fmt::memory_buffer &dest) override {
#ifdef _WIN32
        int total_minutes = get_cached_offset(msg, tm_time);
#else
        // No need to chache under gcc,
        // it is very fast (already stored in tm.tm_gmtoff)
        (void) (msg);
        int total_minutes = abel::utc_minutes_offset(tm_time);
#endif
        bool is_negative = total_minutes < 0;
        if (is_negative) {
            total_minutes = -total_minutes;
            dest.push_back('-');
        } else {
            dest.push_back('+');
        }

        fmt_helper::pad2(total_minutes / 60, dest); // hours
        dest.push_back(':');
        fmt_helper::pad2(total_minutes % 60, dest); // minutes
    }

private:
    abel::abel_time last_update_;
#ifdef _WIN32
    int offset_minutes_{0};

    int get_cached_offset(const log_msg &msg, const std::tm &tm_time)
    {
        if (msg.time - last_update_ >= cache_refresh)
        {
            offset_minutes_ = os::utc_minutes_offset(tm_time);
            last_update_ = msg.time;
        }
        return offset_minutes_;
    }
#endif
};

// Thread id
class t_formatter ABEL_INHERITANCE_FINAL : public flag_formatter {
    void format (const details::log_msg &msg, const std::tm &, fmt::memory_buffer &dest) override {
        fmt_helper::pad6(msg.thread_id, dest);
    }
};

// Current pid
class pid_formatter ABEL_INHERITANCE_FINAL : public flag_formatter {
    void format (const details::log_msg &, const std::tm &, fmt::memory_buffer &dest) override {
        fmt_helper::append_int(abel::pid(), dest);
    }
};

// message counter formatter
class i_formatter ABEL_INHERITANCE_FINAL : public flag_formatter {
    void format (const details::log_msg &msg, const std::tm &, fmt::memory_buffer &dest) override {
        fmt_helper::pad6(msg.msg_id, dest);
    }
};

class v_formatter ABEL_INHERITANCE_FINAL : public flag_formatter {
    void format (const details::log_msg &msg, const std::tm &, fmt::memory_buffer &dest) override {
        fmt_helper::append_buf(msg.raw, dest);
    }
};

class ch_formatter ABEL_INHERITANCE_FINAL : public flag_formatter {
public:
    explicit ch_formatter (char ch)
        : ch_(ch) {
    }
    void format (const details::log_msg &, const std::tm &, fmt::memory_buffer &dest) override {
        dest.push_back(ch_);
    }

private:
    char ch_;
};

// aggregate user chars to display as is
class aggregate_formatter ABEL_INHERITANCE_FINAL : public flag_formatter {
public:
    aggregate_formatter () = default;

    void add_ch (char ch) {
        str_ += ch;
    }
    void format (const details::log_msg &, const std::tm &, fmt::memory_buffer &dest) override {
        fmt_helper::append_str(str_, dest);
    }

private:
    std::string str_;
};

// mark the color range. expect it to be in the form of "%^colored text%$"
class color_start_formatter ABEL_INHERITANCE_FINAL : public flag_formatter {
    void format (const details::log_msg &msg, const std::tm &, fmt::memory_buffer &dest) override {
        msg.color_range_start = dest.size();
    }
};
class color_stop_formatter ABEL_INHERITANCE_FINAL : public flag_formatter {
    void format (const details::log_msg &msg, const std::tm &, fmt::memory_buffer &dest) override {
        msg.color_range_end = dest.size();
    }
};

// Full info formatter
// pattern: [%Y-%m-%d %H:%M:%S.%e] [%n] [%l] %v
class full_formatter ABEL_INHERITANCE_FINAL : public flag_formatter {
    void format (const details::log_msg &msg, const std::tm &tm_time, fmt::memory_buffer &dest) override {
        using namespace std::chrono;
        // cache the date/time part for the next second.
        auto secs = abel::to_unix_seconds(msg.time);

        if (cache_timestamp_ != secs || cached_datetime_.size() == 0) {
            cached_datetime_.resize(0);
            cached_datetime_.push_back('[');
            fmt_helper::append_int(tm_time.tm_year + 1900, cached_datetime_);
            cached_datetime_.push_back('-');

            fmt_helper::pad2(tm_time.tm_mon + 1, cached_datetime_);
            cached_datetime_.push_back('-');

            fmt_helper::pad2(tm_time.tm_mday, cached_datetime_);
            cached_datetime_.push_back(' ');

            fmt_helper::pad2(tm_time.tm_hour, cached_datetime_);
            cached_datetime_.push_back(':');

            fmt_helper::pad2(tm_time.tm_min, cached_datetime_);
            cached_datetime_.push_back(':');

            fmt_helper::pad2(tm_time.tm_sec, cached_datetime_);
            cached_datetime_.push_back('.');

            cache_timestamp_ = secs;
        }
        fmt_helper::append_buf(cached_datetime_, dest);
        static constexpr abel::duration one_sec = abel::seconds(1);
        auto millis = abel::to_duration(msg.time);
        fmt_helper::pad3(static_cast<int>(abel::to_int64_milliseconds(millis % one_sec)), dest);
        dest.push_back(']');
        dest.push_back(' ');
        dest.push_back('[');
        fmt_helper::append_str(*msg.logger_name, dest);
        dest.push_back(']');
        dest.push_back(' ');
        dest.push_back('[');
        // wrap the level name with color
        msg.color_range_start = dest.size();
        fmt_helper::append_c_str(level::to_c_str(msg.level), dest);
        msg.color_range_end = dest.size();
        dest.push_back(']');
        dest.push_back(' ');
        fmt_helper::append_buf(msg.raw, dest);
    }

private:
    int64_t cache_timestamp_ {0};
    fmt::basic_memory_buffer<char, 128> cached_datetime_;
};

} // namespace details

class pattern_formatter ABEL_INHERITANCE_FINAL : public formatter {
public:
    explicit pattern_formatter (
        std::string pattern,
        pattern_time_type time_type = pattern_time_type::local,
        std::string eol = abel::details::default_eol)
        : pattern_(std::move(pattern)), eol_(std::move(eol)), pattern_time_type_(time_type), last_log_secs_(0) {
        std::memset(&cached_tm_, 0, sizeof(cached_tm_));
        compile_pattern_(pattern_);
    }

    pattern_formatter (const pattern_formatter &other) = delete;
    pattern_formatter &operator = (const pattern_formatter &other) = delete;

    virtual std::unique_ptr<formatter> clone () const override {
        return std::unique_ptr<formatter>(new pattern_formatter(pattern_, pattern_time_type_, eol_));
    }

    void format (const details::log_msg &msg, fmt::memory_buffer &dest) override {
        auto secs = abel::to_unix_seconds(msg.time);
        if (secs != last_log_secs_) {
            cached_tm_ = get_time_(msg);
            last_log_secs_ = secs;
        }
        for (auto &f : formatters_) {
            f->format(msg, cached_tm_, dest);
        }
        // write eol
        details::fmt_helper::append_str(eol_, dest);
    }

private:
    std::string pattern_;
    std::string eol_;
    pattern_time_type pattern_time_type_;
    std::tm cached_tm_;
    int64_t last_log_secs_;

    std::vector<std::unique_ptr<details::flag_formatter>> formatters_;

    std::tm get_time_ (const details::log_msg &msg) {
        if (pattern_time_type_ == pattern_time_type::local) {
            return abel::local_tm(msg.time);
        }
        return abel::utc_tm(msg.time);
    }

    void handle_flag_ (char flag) {
        switch (flag) {
            // logger name
        case 'n':formatters_.emplace_back(new details::name_formatter());
            break;

        case 'l':formatters_.emplace_back(new details::level_formatter());
            break;

        case 'L':formatters_.emplace_back(new details::short_level_formatter());
            break;

        case ('t'):formatters_.emplace_back(new details::t_formatter());
            break;

        case ('v'):formatters_.emplace_back(new details::v_formatter());
            break;

        case ('a'):formatters_.emplace_back(new details::a_formatter());
            break;

        case ('A'):formatters_.emplace_back(new details::A_formatter());
            break;

        case ('b'):
        case ('h'):formatters_.emplace_back(new details::b_formatter());
            break;

        case ('B'):formatters_.emplace_back(new details::B_formatter());
            break;
        case ('c'):formatters_.emplace_back(new details::c_formatter());
            break;

        case ('C'):formatters_.emplace_back(new details::C_formatter());
            break;

        case ('Y'):formatters_.emplace_back(new details::Y_formatter());
            break;

        case ('D'):
        case ('x'):formatters_.emplace_back(new details::D_formatter());
            break;

        case ('m'):formatters_.emplace_back(new details::m_formatter());
            break;

        case ('d'):formatters_.emplace_back(new details::d_formatter());
            break;

        case ('H'):formatters_.emplace_back(new details::H_formatter());
            break;

        case ('I'):formatters_.emplace_back(new details::I_formatter());
            break;

        case ('M'):formatters_.emplace_back(new details::M_formatter());
            break;

        case ('S'):formatters_.emplace_back(new details::S_formatter());
            break;

        case ('e'):formatters_.emplace_back(new details::e_formatter());
            break;

        case ('f'):formatters_.emplace_back(new details::f_formatter());
            break;
        case ('F'):formatters_.emplace_back(new details::F_formatter());
            break;

        case ('E'):formatters_.emplace_back(new details::E_formatter());
            break;

        case ('p'):formatters_.emplace_back(new details::p_formatter());
            break;

        case ('r'):formatters_.emplace_back(new details::r_formatter());
            break;

        case ('R'):formatters_.emplace_back(new details::R_formatter());
            break;

        case ('T'):
        case ('X'):formatters_.emplace_back(new details::T_formatter());
            break;

        case ('z'):formatters_.emplace_back(new details::z_formatter());
            break;

        case ('+'):formatters_.emplace_back(new details::full_formatter());
            break;

        case ('P'):formatters_.emplace_back(new details::pid_formatter());
            break;

        case ('i'):formatters_.emplace_back(new details::i_formatter());
            break;

        case ('^'):formatters_.emplace_back(new details::color_start_formatter());
            break;

        case ('$'):formatters_.emplace_back(new details::color_stop_formatter());
            break;

        default: // Unknown flag appears as is
            formatters_.emplace_back(new details::ch_formatter('%'));
            formatters_.emplace_back(new details::ch_formatter(flag));
            break;
        }
    }

    void compile_pattern_ (const std::string &pattern) {
        auto end = pattern.end();
        std::unique_ptr<details::aggregate_formatter> user_chars;
        formatters_.clear();
        for (auto it = pattern.begin(); it != end; ++it) {
            if (*it == '%') {
                if (user_chars) // append user chars found so far
                {
                    formatters_.push_back(std::move(user_chars));
                }
                // if(
                if (++it != end) {
                    handle_flag_(*it);
                } else {
                    break;
                }
            } else // chars not following the % sign should be displayed as is
            {
                if (!user_chars) {
                    user_chars = std::unique_ptr<details::aggregate_formatter>(new details::aggregate_formatter());
                }
                user_chars->add_ch(*it);
            }
        }
        if (user_chars) // append raw chars found so far
        {
            formatters_.push_back(std::move(user_chars));
        }
    }
};
} // namespace abel
