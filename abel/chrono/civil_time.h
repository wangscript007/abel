//
// -----------------------------------------------------------------------------
// File: civil_time.h
// -----------------------------------------------------------------------------
//
// This header file defines abstractions for computing with "civil time".
// The term "civil time" refers to the legally recognized human-scale time
// that is represented by the six fields `YYYY-MM-DD hh:mm:ss`. A "date"
// is perhaps the most common example of a civil time (represented here as
// an `abel::chrono_day`).
//
// Modern-day civil time follows the Gregorian Calendar and is a
// time-zone-independent concept: a civil time of "2015-06-01 12:00:00", for
// example, is not tied to a time zone. Put another way, a civil time does not
// map to a unique point in time; a civil time must be mapped to an absolute
// time *through* a time zone.
//
// Because a civil time is what most people think of as "time," it is common to
// map absolute times to civil times to present to users.
//
// abel_time zones define the relationship between absolute and civil times. Given an
// absolute or civil time and a time zone, you can compute the other time:
//
//   Civil abel_time = F(Absolute abel_time, abel_time Zone)
//   Absolute abel_time = G(Civil abel_time, abel_time Zone)
//
// The abel time library allows you to construct such civil times from
// absolute times; consult time.h for such functionality.
//
// This library provides six classes for constructing civil-time objects, and
// provides several helper functions for rounding, iterating, and performing
// arithmetic on civil-time objects, while avoiding complications like
// daylight-saving time (DST):
//
//   * `abel::chrono_second`
//   * `abel::chrono_minute`
//   * `abel::chrono_hour`
//   * `abel::chrono_day`
//   * `abel::chrono_month`
//   * `abel::chrono_year`
//
// Example:
//
//   // Construct a civil-time object for a specific day
//   const abel::chrono_day cd(1969, 07, 20);
//
//   // Construct a civil-time object for a specific second
//   const abel::chrono_second cd(2018, 8, 1, 12, 0, 1);
//
// Note: In C++14 and later, this library is usable in a constexpr context.
//
// Example:
//
//   // Valid in C++14
//   constexpr abel::chrono_day cd(1969, 07, 20);

#ifndef ABEL_TIME_CIVIL_TIME_H_
#define ABEL_TIME_CIVIL_TIME_H_

#include <string>
#include <abel/strings/string_view.h>
#include <abel/chrono/internal/chrono_time_internal.h>
#include <abel/base/profile.h>

namespace abel {

    namespace chrono_internal {

        struct second_tag : abel::chrono_internal::detail::second_tag {
        };
        struct minute_tag : second_tag, abel::chrono_internal::detail::minute_tag {
        };
        struct hour_tag : minute_tag, abel::chrono_internal::detail::hour_tag {
        };
        struct day_tag : hour_tag, abel::chrono_internal::detail::day_tag {
        };
        struct month_tag : day_tag, abel::chrono_internal::detail::month_tag {
        };
        struct year_tag : month_tag, abel::chrono_internal::detail::year_tag {
        };
    }  // namespace chrono_internal

    // -----------------------------------------------------------------------------
    // chrono_second, chrono_minute, chrono_hour, chrono_day, chrono_month, chrono_year
    // -----------------------------------------------------------------------------
    //
    // Each of these civil-time types is a simple value type with the same
    // interface for construction and the same six accessors for each of the civil
    // time fields (year, month, day, hour, minute, and second, aka YMDHMS). These
    // classes differ only in their alignment, which is indicated by the type name
    // and specifies the field on which arithmetic operates.
    //
    // CONSTRUCTION
    //
    // Each of the civil-time types can be constructed in two ways: by directly
    // passing to the constructor up to six integers representing the YMDHMS fields,
    // or by copying the YMDHMS fields from a differently aligned civil-time type.
    // Omitted fields are assigned their minimum valid value. hours, minutes, and
    // seconds will be set to 0, month and day will be set to 1. Since there is no
    // minimum year, the default is 1970.
    //
    // Examples:
    //
    //   abel::chrono_day default_value;               // 1970-01-01 00:00:00
    //
    //   abel::chrono_day a(2015, 2, 3);               // 2015-02-03 00:00:00
    //   abel::chrono_day b(2015, 2, 3, 4, 5, 6);      // 2015-02-03 00:00:00
    //   abel::chrono_day c(2015);                     // 2015-01-01 00:00:00
    //
    //   abel::chrono_second ss(2015, 2, 3, 4, 5, 6);  // 2015-02-03 04:05:06
    //   abel::chrono_minute mm(ss);                   // 2015-02-03 04:05:00
    //   abel::chrono_hour hh(mm);                     // 2015-02-03 04:00:00
    //   abel::chrono_day d(hh);                       // 2015-02-03 00:00:00
    //   abel::chrono_month m(d);                      // 2015-02-01 00:00:00
    //   abel::chrono_year y(m);                       // 2015-01-01 00:00:00
    //
    //   m = abel::chrono_month(y);                    // 2015-01-01 00:00:00
    //   d = abel::chrono_day(m);                      // 2015-01-01 00:00:00
    //   hh = abel::chrono_hour(d);                    // 2015-01-01 00:00:00
    //   mm = abel::chrono_minute(hh);                 // 2015-01-01 00:00:00
    //   ss = abel::chrono_second(mm);                 // 2015-01-01 00:00:00
    //
    // Each civil-time class is aligned to the civil-time field indicated in the
    // class's name after normalization. Alignment is performed by setting all the
    // inferior fields to their minimum valid value (as described above). The
    // following are examples of how each of the six types would align the fields
    // representing November 22, 2015 at 12:34:56 in the afternoon. (Note: the
    // string format used here is not important; it's just a shorthand way of
    // showing the six YMDHMS fields.)
    //
    //   abel::chrono_second   : 2015-11-22 12:34:56
    //   abel::chrono_minute   : 2015-11-22 12:34:00
    //   abel::chrono_hour     : 2015-11-22 12:00:00
    //   abel::chrono_day      : 2015-11-22 00:00:00
    //   abel::chrono_month    : 2015-11-01 00:00:00
    //   abel::chrono_year     : 2015-01-01 00:00:00
    //
    // Each civil-time type performs arithmetic on the field to which it is
    // aligned. This means that adding 1 to an abel::chrono_day increments the day
    // field (normalizing as necessary), and subtracting 7 from an abel::chrono_month
    // operates on the month field (normalizing as necessary). All arithmetic
    // produces a valid civil time. Difference requires two similarly aligned
    // civil-time objects and returns the scalar answer in units of the objects'
    // alignment. For example, the difference between two abel::chrono_hour objects
    // will give an answer in units of civil hours.
    //
    // ALIGNMENT CONVERSION
    //
    // The alignment of a civil-time object cannot change, but the object may be
    // used to construct a new object with a different alignment. This is referred
    // to as "realigning". When realigning to a type with the same or more
    // precision (e.g., abel::chrono_day -> abel::chrono_second), the conversion may be
    // performed implicitly since no information is lost. However, if information
    // could be discarded (e.g., chrono_second -> chrono_day), the conversion must
    // be explicit at the call site.
    //
    // Examples:
    //
    //   void UseDay(abel::chrono_day day);
    //
    //   abel::chrono_second cs;
    //   UseDay(cs);                  // Won't compile because data may be discarded
    //   UseDay(abel::chrono_day(cs));  // OK: explicit conversion
    //
    //   abel::chrono_day cd;
    //   UseDay(cd);                  // OK: no conversion needed
    //
    //   abel::chrono_month cm;
    //   UseDay(cm);                  // OK: implicit conversion to abel::chrono_day
    //
    // NORMALIZATION
    //
    // Normalization takes invalid values and adjusts them to produce valid values.
    // Within the civil-time library, integer arguments passed to the Civil*
    // constructors may be out-of-range, in which case they are normalized by
    // carrying overflow into a field of courser granularity to produce valid
    // civil-time objects. This normalization enables natural arithmetic on
    // constructor arguments without worrying about the field's range.
    //
    // Examples:
    //
    //   // Out-of-range; normalized to 2016-11-01
    //   abel::chrono_day d(2016, 10, 32);
    //   // Out-of-range, negative: normalized to 2016-10-30T23
    //   abel::chrono_hour h1(2016, 10, 31, -1);
    //   // Normalization is cumulative: normalized to 2016-10-30T23
    //   abel::chrono_hour h2(2016, 10, 32, -25);
    //
    // Note: If normalization is undesired, you can signal an error by comparing
    // the constructor arguments to the normalized values returned by the YMDHMS
    // properties.
    //
    // COMPARISON
    //
    // Comparison between civil-time objects considers all six YMDHMS fields,
    // regardless of the type's alignment. Comparison between differently aligned
    // civil-time types is allowed.
    //
    // Examples:
    //
    //   abel::chrono_day feb_3(2015, 2, 3);  // 2015-02-03 00:00:00
    //   abel::chrono_day mar_4(2015, 3, 4);  // 2015-03-04 00:00:00
    //   // feb_3 < mar_4
    //   // abel::chrono_year(feb_3) == abel::chrono_year(mar_4)
    //
    //   abel::chrono_second feb_3_noon(2015, 2, 3, 12, 0, 0);  // 2015-02-03 12:00:00
    //   // feb_3 < feb_3_noon
    //   // feb_3 == abel::chrono_day(feb_3_noon)
    //
    //   // Iterates all the days of February 2015.
    //   for (abel::chrono_day d(2015, 2, 1); d < abel::chrono_month(2015, 3); ++d) {
    //     // ...
    //   }
    //
    // ARITHMETIC
    //
    // Civil-time types support natural arithmetic operators such as addition,
    // subtraction, and difference. Arithmetic operates on the civil-time field
    // indicated in the type's name. Difference operators require arguments with
    // the same alignment and return the answer in units of the alignment.
    //
    // Example:
    //
    //   abel::chrono_day a(2015, 2, 3);
    //   ++a;                              // 2015-02-04 00:00:00
    //   --a;                              // 2015-02-03 00:00:00
    //   abel::chrono_day b = a + 1;         // 2015-02-04 00:00:00
    //   abel::chrono_day c = 1 + b;         // 2015-02-05 00:00:00
    //   int n = c - a;                    // n = 2 (civil days)
    //   int m = c - abel::chrono_month(c);  // Won't compile: different types.
    //
    // ACCESSORS
    //
    // Each civil-time type has accessors for all six of the civil-time fields:
    // year, month, day, hour, minute, and second.
    //
    // chrono_year_t year()
    // int          month()
    // int          day()
    // int          hour()
    // int          minute()
    // int          second()
    //
    // Recall that fields inferior to the type's alignment will be set to their
    // minimum valid value.
    //
    // Example:
    //
    //   abel::chrono_day d(2015, 6, 28);
    //   // d.year() == 2015
    //   // d.month() == 6
    //   // d.day() == 28
    //   // d.hour() == 0
    //   // d.minute() == 0
    //   // d.second() == 0
    //
    // CASE STUDY: Adding a month to January 31.
    //
    // One of the classic questions that arises when considering a civil time
    // library (or a date library or a date/time library) is this:
    //   "What is the result of adding a month to January 31?"
    // This is an interesting question because it is unclear what is meant by a
    // "month", and several different answers are possible, depending on context:
    //
    //   1. March 3 (or 2 if a leap year), if "add a month" means to add a month to
    //      the current month, and adjust the date to overflow the extra days into
    //      March. In this case the result of "February 31" would be normalized as
    //      within the civil-time library.
    //   2. February 28 (or 29 if a leap year), if "add a month" means to add a
    //      month, and adjust the date while holding the resulting month constant.
    //      In this case, the result of "February 31" would be truncated to the last
    //      day in February.
    //   3. An error. The caller may get some error, an exception, an invalid date
    //      object, or perhaps return `false`. This may make sense because there is
    //      no single unambiguously correct answer to the question.
    //
    // Practically speaking, any answer that is not what the programmer intended
    // is the wrong answer.
    //
    // The abel time library avoids this problem by making it impossible to
    // ask ambiguous questions. All civil-time objects are aligned to a particular
    // civil-field boundary (such as aligned to a year, month, day, hour, minute,
    // or second), and arithmetic operates on the field to which the object is
    // aligned. This means that in order to "add a month" the object must first be
    // aligned to a month boundary, which is equivalent to the first day of that
    // month.
    //
    // Of course, there are ways to compute an answer the question at hand using
    // this abel time library, but they require the programmer to be explicit
    // about the answer they expect. To illustrate, let's see how to compute all
    // three of the above possible answers to the question of "Jan 31 plus 1
    // month":
    //
    // Example:
    //
    //   const abel::chrono_day d(2015, 1, 31);
    //
    //   // Answer 1:
    //   // Add 1 to the month field in the constructor, and rely on normalization.
    //   const auto normalized = abel::chrono_day(d.year(), d.month() + 1, d.day());
    //   // normalized == 2015-03-03 (aka Feb 31)
    //
    //   // Answer 2:
    //   // Add 1 to month field, capping to the end of next month.
    //   const auto next_month = abel::chrono_month(d) + 1;
    //   const auto last_day_of_next_month = abel::chrono_day(next_month + 1) - 1;
    //   const auto capped = std::min(normalized, last_day_of_next_month);
    //   // capped == 2015-02-28
    //
    //   // Answer 3:
    //   // signal an error if the normalized answer is not in next month.
    //   if (abel::chrono_month(normalized) != next_month) {
    //     // error, month overflow
    //   }
    //
    using chrono_second =
    abel::chrono_internal::detail::civil_time<chrono_internal::second_tag>;
    using chrono_minute =
    abel::chrono_internal::detail::civil_time<chrono_internal::minute_tag>;
    using chrono_hour =
    abel::chrono_internal::detail::civil_time<chrono_internal::hour_tag>;
    using chrono_day =
    abel::chrono_internal::detail::civil_time<chrono_internal::day_tag>;
    using chrono_month =
    abel::chrono_internal::detail::civil_time<chrono_internal::month_tag>;
    using chrono_year =
    abel::chrono_internal::detail::civil_time<chrono_internal::year_tag>;

    // chrono_year_t
    //
    // Type alias of a civil-time year value. This type is guaranteed to (at least)
    // support any year value supported by `time_t`.
    //
    // Example:
    //
    //   abel::chrono_second cs = ...;
    //   abel::chrono_year_t y = cs.year();
    //   cs = abel::chrono_second(y, 1, 1, 0, 0, 0);  // chrono_second(chrono_year(cs))
    //
    using chrono_year_t = abel::chrono_internal::year_t;

    // chrono_diff_t
    //
    // Type alias of the difference between two civil-time values.
    // This type is used to indicate arguments that are not
    // normalized (such as parameters to the civil-time constructors), the results
    // of civil-time subtraction, or the operand to civil-time addition.
    //
    // Example:
    //
    //   abel::chrono_diff_t n_sec = cs1 - cs2;             // cs1 == cs2 + n_sec;
    //
    using chrono_diff_t = abel::chrono_internal::diff_t;

    // chrono_weekday::monday, chrono_weekday::tuesday, chrono_weekday::wednesday, chrono_weekday::thursday,
    // chrono_weekday::friday, chrono_weekday::saturday, chrono_weekday::sunday
    //
    // The chrono_weekday enum class represents the civil-time concept of a "weekday" with
    // members for all days of the week.
    //
    //   abel::chrono_weekday wd = abel::chrono_weekday::thursday;
    //
    using chrono_weekday = abel::chrono_internal::weekday;

    // GetWeekday()
    //
    // Returns the abel::chrono_weekday for the given (realigned) civil-time value.
    //
    // Example:
    //
    //   abel::chrono_day a(2015, 8, 13);
    //   abel::chrono_weekday wd = abel::GetWeekday(a);  // wd == abel::chrono_weekday::thursday
    //
    ABEL_FORCE_INLINE chrono_weekday GetWeekday(chrono_second cs) {
        return abel::chrono_internal::get_weekday(cs);
    }

    // NextWeekday()
    // PrevWeekday()
    //
    // Returns the abel::chrono_day that strictly follows or precedes a given
    // abel::chrono_day, and that falls on the given abel::chrono_weekday.
    //
    // Example, given the following month:
    //
    //       August 2015
    //   Su Mo Tu We Th Fr Sa
    //                      1
    //    2  3  4  5  6  7  8
    //    9 10 11 12 13 14 15
    //   16 17 18 19 20 21 22
    //   23 24 25 26 27 28 29
    //   30 31
    //
    //   abel::chrono_day a(2015, 8, 13);
    //   // abel::GetWeekday(a) == abel::chrono_weekday::thursday
    //   abel::chrono_day b = abel::NextWeekday(a, abel::chrono_weekday::thursday);
    //   // b = 2015-08-20
    //   abel::chrono_day c = abel::PrevWeekday(a, abel::chrono_weekday::thursday);
    //   // c = 2015-08-06
    //
    //   abel::chrono_day d = ...
    //   // Gets the following Thursday if d is not already Thursday
    //   abel::chrono_day thurs1 = abel::NextWeekday(d - 1, abel::chrono_weekday::thursday);
    //   // Gets the previous Thursday if d is not already Thursday
    //   abel::chrono_day thurs2 = abel::PrevWeekday(d + 1, abel::chrono_weekday::thursday);
    //
    ABEL_FORCE_INLINE chrono_day NextWeekday(chrono_day cd, chrono_weekday wd) {
        return chrono_day(abel::chrono_internal::next_weekday(cd, wd));
    }

    ABEL_FORCE_INLINE chrono_day PrevWeekday(chrono_day cd, chrono_weekday wd) {
        return chrono_day(abel::chrono_internal::prev_weekday(cd, wd));
    }

    // get_yearday()
    //
    // Returns the day-of-year for the given (realigned) civil-time value.
    //
    // Example:
    //
    //   abel::chrono_day a(2015, 1, 1);
    //   int yd_jan_1 = abel::get_yearday(a);   // yd_jan_1 = 1
    //   abel::chrono_day b(2015, 12, 31);
    //   int yd_dec_31 = abel::get_yearday(b);  // yd_dec_31 = 365
    //
    ABEL_FORCE_INLINE int get_yearday(chrono_second cs) {
        return abel::chrono_internal::get_yearday(cs);
    }

    // format_chrono_time()
    //
    // Formats the given civil-time value into a string value of the following
    // format:
    //
    //  Type        | Format
    //  ---------------------------------
    //  chrono_second | YYYY-MM-DDTHH:MM:SS
    //  chrono_minute | YYYY-MM-DDTHH:MM
    //  chrono_hour   | YYYY-MM-DDTHH
    //  chrono_day    | YYYY-MM-DD
    //  chrono_month  | YYYY-MM
    //  chrono_year   | YYYY
    //
    // Example:
    //
    //   abel::chrono_day d = abel::chrono_day(1969, 7, 20);
    //   std::string day_string = abel::format_chrono_time(d);  // "1969-07-20"
    //
    std::string format_chrono_time(chrono_second c);

    std::string format_chrono_time(chrono_minute c);

    std::string format_chrono_time(chrono_hour c);

    std::string format_chrono_time(chrono_day c);

    std::string format_chrono_time(chrono_month c);

    std::string format_chrono_time(chrono_year c);

// abel::parse_chrono_time()
//
// Parses a civil-time value from the specified `abel::string_view` into the
// passed output parameter. Returns `true` upon successful parsing.
//
// The expected form of the input string is as follows:
//
//  Type        | Format
//  ---------------------------------
//  chrono_second | YYYY-MM-DDTHH:MM:SS
//  chrono_minute | YYYY-MM-DDTHH:MM
//  chrono_hour   | YYYY-MM-DDTHH
//  chrono_day    | YYYY-MM-DD
//  chrono_month  | YYYY-MM
//  chrono_year   | YYYY
//
// Example:
//
//   abel::chrono_day d;
//   bool ok = abel::parse_chrono_time("2018-01-02", &d); // OK
//
// Note that parsing will fail if the string's format does not match the
// expected type exactly. `ParseLenientCivilTime()` below is more lenient.
//
    bool parse_chrono_time(abel::string_view s, chrono_second *c);

    bool parse_chrono_time(abel::string_view s, chrono_minute *c);

    bool parse_chrono_time(abel::string_view s, chrono_hour *c);

    bool parse_chrono_time(abel::string_view s, chrono_day *c);

    bool parse_chrono_time(abel::string_view s, chrono_month *c);

    bool parse_chrono_time(abel::string_view s, chrono_year *c);

// ParseLenientCivilTime()
//
// Parses any of the formats accepted by `abel::parse_chrono_time()`, but is more
// lenient if the format of the string does not exactly match the associated
// type.
//
// Example:
//
//   abel::chrono_day d;
//   bool ok = abel::ParseLenientCivilTime("1969-07-20", &d); // OK
//   ok = abel::ParseLenientCivilTime("1969-07-20T10", &d);   // OK: T10 floored
//   ok = abel::ParseLenientCivilTime("1969-07", &d);   // OK: day defaults to 1
//
    bool ParseLenientCivilTime(abel::string_view s, chrono_second *c);

    bool ParseLenientCivilTime(abel::string_view s, chrono_minute *c);

    bool ParseLenientCivilTime(abel::string_view s, chrono_hour *c);

    bool ParseLenientCivilTime(abel::string_view s, chrono_day *c);

    bool ParseLenientCivilTime(abel::string_view s, chrono_month *c);

    bool ParseLenientCivilTime(abel::string_view s, chrono_year *c);

    namespace chrono_internal {  // For functions found via ADL on civil-time tags.

// Streaming Operators
//
// Each civil-time type may be sent to an output stream using operator<<().
// The result matches the string produced by `format_chrono_time()`.
//
// Example:
//
//   abel::chrono_day d = abel::chrono_day(1969, 7, 20);
//   std::cout << "Date is: " << d << "\n";
//
        std::ostream &operator<<(std::ostream &os, chrono_year y);

        std::ostream &operator<<(std::ostream &os, chrono_month m);

        std::ostream &operator<<(std::ostream &os, chrono_day d);

        std::ostream &operator<<(std::ostream &os, chrono_hour h);

        std::ostream &operator<<(std::ostream &os, chrono_minute m);

        std::ostream &operator<<(std::ostream &os, chrono_second s);

    }  // namespace chrono_internal


}  // namespace abel

#endif  // ABEL_TIME_CIVIL_TIME_H_
