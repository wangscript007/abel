//
// -----------------------------------------------------------------------------
// File: civil_time.h
// -----------------------------------------------------------------------------
//
// This header file defines abstractions for computing with "civil time".
// The term "civil time" refers to the legally recognized human-scale time
// that is represented by the six fields `YYYY-MM-DD hh:mm:ss`. A "date"
// is perhaps the most common example of a civil time (represented here as
// an `abel::CivilDay`).
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
//   * `abel::CivilSecond`
//   * `abel::CivilMinute`
//   * `abel::CivilHour`
//   * `abel::CivilDay`
//   * `abel::CivilMonth`
//   * `abel::CivilYear`
//
// Example:
//
//   // Construct a civil-time object for a specific day
//   const abel::CivilDay cd(1969, 07, 20);
//
//   // Construct a civil-time object for a specific second
//   const abel::CivilSecond cd(2018, 8, 1, 12, 0, 1);
//
// Note: In C++14 and later, this library is usable in a constexpr context.
//
// Example:
//
//   // Valid in C++14
//   constexpr abel::CivilDay cd(1969, 07, 20);

#ifndef ABEL_TIME_CIVIL_TIME_H_
#define ABEL_TIME_CIVIL_TIME_H_

#include <string>

#include <abel/strings/string_view.h>
#include <abel/time/internal/cctz/include/cctz/civil_time.h>

namespace abel {
ABEL_NAMESPACE_BEGIN

namespace time_internal {
struct second_tag : cctz::detail::second_tag {};
struct minute_tag : second_tag, cctz::detail::minute_tag {};
struct hour_tag : minute_tag, cctz::detail::hour_tag {};
struct day_tag : hour_tag, cctz::detail::day_tag {};
struct month_tag : day_tag, cctz::detail::month_tag {};
struct year_tag : month_tag, cctz::detail::year_tag {};
}  // namespace time_internal

// -----------------------------------------------------------------------------
// CivilSecond, CivilMinute, CivilHour, CivilDay, CivilMonth, CivilYear
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
//   abel::CivilDay default_value;               // 1970-01-01 00:00:00
//
//   abel::CivilDay a(2015, 2, 3);               // 2015-02-03 00:00:00
//   abel::CivilDay b(2015, 2, 3, 4, 5, 6);      // 2015-02-03 00:00:00
//   abel::CivilDay c(2015);                     // 2015-01-01 00:00:00
//
//   abel::CivilSecond ss(2015, 2, 3, 4, 5, 6);  // 2015-02-03 04:05:06
//   abel::CivilMinute mm(ss);                   // 2015-02-03 04:05:00
//   abel::CivilHour hh(mm);                     // 2015-02-03 04:00:00
//   abel::CivilDay d(hh);                       // 2015-02-03 00:00:00
//   abel::CivilMonth m(d);                      // 2015-02-01 00:00:00
//   abel::CivilYear y(m);                       // 2015-01-01 00:00:00
//
//   m = abel::CivilMonth(y);                    // 2015-01-01 00:00:00
//   d = abel::CivilDay(m);                      // 2015-01-01 00:00:00
//   hh = abel::CivilHour(d);                    // 2015-01-01 00:00:00
//   mm = abel::CivilMinute(hh);                 // 2015-01-01 00:00:00
//   ss = abel::CivilSecond(mm);                 // 2015-01-01 00:00:00
//
// Each civil-time class is aligned to the civil-time field indicated in the
// class's name after normalization. Alignment is performed by setting all the
// inferior fields to their minimum valid value (as described above). The
// following are examples of how each of the six types would align the fields
// representing November 22, 2015 at 12:34:56 in the afternoon. (Note: the
// string format used here is not important; it's just a shorthand way of
// showing the six YMDHMS fields.)
//
//   abel::CivilSecond   : 2015-11-22 12:34:56
//   abel::CivilMinute   : 2015-11-22 12:34:00
//   abel::CivilHour     : 2015-11-22 12:00:00
//   abel::CivilDay      : 2015-11-22 00:00:00
//   abel::CivilMonth    : 2015-11-01 00:00:00
//   abel::CivilYear     : 2015-01-01 00:00:00
//
// Each civil-time type performs arithmetic on the field to which it is
// aligned. This means that adding 1 to an abel::CivilDay increments the day
// field (normalizing as necessary), and subtracting 7 from an abel::CivilMonth
// operates on the month field (normalizing as necessary). All arithmetic
// produces a valid civil time. Difference requires two similarly aligned
// civil-time objects and returns the scalar answer in units of the objects'
// alignment. For example, the difference between two abel::CivilHour objects
// will give an answer in units of civil hours.
//
// ALIGNMENT CONVERSION
//
// The alignment of a civil-time object cannot change, but the object may be
// used to construct a new object with a different alignment. This is referred
// to as "realigning". When realigning to a type with the same or more
// precision (e.g., abel::CivilDay -> abel::CivilSecond), the conversion may be
// performed implicitly since no information is lost. However, if information
// could be discarded (e.g., CivilSecond -> CivilDay), the conversion must
// be explicit at the call site.
//
// Examples:
//
//   void UseDay(abel::CivilDay day);
//
//   abel::CivilSecond cs;
//   UseDay(cs);                  // Won't compile because data may be discarded
//   UseDay(abel::CivilDay(cs));  // OK: explicit conversion
//
//   abel::CivilDay cd;
//   UseDay(cd);                  // OK: no conversion needed
//
//   abel::CivilMonth cm;
//   UseDay(cm);                  // OK: implicit conversion to abel::CivilDay
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
//   abel::CivilDay d(2016, 10, 32);
//   // Out-of-range, negative: normalized to 2016-10-30T23
//   abel::CivilHour h1(2016, 10, 31, -1);
//   // Normalization is cumulative: normalized to 2016-10-30T23
//   abel::CivilHour h2(2016, 10, 32, -25);
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
//   abel::CivilDay feb_3(2015, 2, 3);  // 2015-02-03 00:00:00
//   abel::CivilDay mar_4(2015, 3, 4);  // 2015-03-04 00:00:00
//   // feb_3 < mar_4
//   // abel::CivilYear(feb_3) == abel::CivilYear(mar_4)
//
//   abel::CivilSecond feb_3_noon(2015, 2, 3, 12, 0, 0);  // 2015-02-03 12:00:00
//   // feb_3 < feb_3_noon
//   // feb_3 == abel::CivilDay(feb_3_noon)
//
//   // Iterates all the days of February 2015.
//   for (abel::CivilDay d(2015, 2, 1); d < abel::CivilMonth(2015, 3); ++d) {
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
//   abel::CivilDay a(2015, 2, 3);
//   ++a;                              // 2015-02-04 00:00:00
//   --a;                              // 2015-02-03 00:00:00
//   abel::CivilDay b = a + 1;         // 2015-02-04 00:00:00
//   abel::CivilDay c = 1 + b;         // 2015-02-05 00:00:00
//   int n = c - a;                    // n = 2 (civil days)
//   int m = c - abel::CivilMonth(c);  // Won't compile: different types.
//
// ACCESSORS
//
// Each civil-time type has accessors for all six of the civil-time fields:
// year, month, day, hour, minute, and second.
//
// civil_year_t year()
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
//   abel::CivilDay d(2015, 6, 28);
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
//   const abel::CivilDay d(2015, 1, 31);
//
//   // Answer 1:
//   // Add 1 to the month field in the constructor, and rely on normalization.
//   const auto normalized = abel::CivilDay(d.year(), d.month() + 1, d.day());
//   // normalized == 2015-03-03 (aka Feb 31)
//
//   // Answer 2:
//   // Add 1 to month field, capping to the end of next month.
//   const auto next_month = abel::CivilMonth(d) + 1;
//   const auto last_day_of_next_month = abel::CivilDay(next_month + 1) - 1;
//   const auto capped = std::min(normalized, last_day_of_next_month);
//   // capped == 2015-02-28
//
//   // Answer 3:
//   // signal an error if the normalized answer is not in next month.
//   if (abel::CivilMonth(normalized) != next_month) {
//     // error, month overflow
//   }
//
using CivilSecond =
    time_internal::cctz::detail::civil_time<time_internal::second_tag>;
using CivilMinute =
    time_internal::cctz::detail::civil_time<time_internal::minute_tag>;
using CivilHour =
    time_internal::cctz::detail::civil_time<time_internal::hour_tag>;
using CivilDay =
    time_internal::cctz::detail::civil_time<time_internal::day_tag>;
using CivilMonth =
    time_internal::cctz::detail::civil_time<time_internal::month_tag>;
using CivilYear =
    time_internal::cctz::detail::civil_time<time_internal::year_tag>;

// civil_year_t
//
// Type alias of a civil-time year value. This type is guaranteed to (at least)
// support any year value supported by `time_t`.
//
// Example:
//
//   abel::CivilSecond cs = ...;
//   abel::civil_year_t y = cs.year();
//   cs = abel::CivilSecond(y, 1, 1, 0, 0, 0);  // CivilSecond(CivilYear(cs))
//
using civil_year_t = time_internal::cctz::year_t;

// civil_diff_t
//
// Type alias of the difference between two civil-time values.
// This type is used to indicate arguments that are not
// normalized (such as parameters to the civil-time constructors), the results
// of civil-time subtraction, or the operand to civil-time addition.
//
// Example:
//
//   abel::civil_diff_t n_sec = cs1 - cs2;             // cs1 == cs2 + n_sec;
//
using civil_diff_t = time_internal::cctz::diff_t;

// Weekday::monday, Weekday::tuesday, Weekday::wednesday, Weekday::thursday,
// Weekday::friday, Weekday::saturday, Weekday::sunday
//
// The Weekday enum class represents the civil-time concept of a "weekday" with
// members for all days of the week.
//
//   abel::Weekday wd = abel::Weekday::thursday;
//
using Weekday = time_internal::cctz::weekday;

// GetWeekday()
//
// Returns the abel::Weekday for the given (realigned) civil-time value.
//
// Example:
//
//   abel::CivilDay a(2015, 8, 13);
//   abel::Weekday wd = abel::GetWeekday(a);  // wd == abel::Weekday::thursday
//
ABEL_FORCE_INLINE Weekday GetWeekday(CivilSecond cs) {
  return time_internal::cctz::get_weekday(cs);
}

// NextWeekday()
// PrevWeekday()
//
// Returns the abel::CivilDay that strictly follows or precedes a given
// abel::CivilDay, and that falls on the given abel::Weekday.
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
//   abel::CivilDay a(2015, 8, 13);
//   // abel::GetWeekday(a) == abel::Weekday::thursday
//   abel::CivilDay b = abel::NextWeekday(a, abel::Weekday::thursday);
//   // b = 2015-08-20
//   abel::CivilDay c = abel::PrevWeekday(a, abel::Weekday::thursday);
//   // c = 2015-08-06
//
//   abel::CivilDay d = ...
//   // Gets the following Thursday if d is not already Thursday
//   abel::CivilDay thurs1 = abel::NextWeekday(d - 1, abel::Weekday::thursday);
//   // Gets the previous Thursday if d is not already Thursday
//   abel::CivilDay thurs2 = abel::PrevWeekday(d + 1, abel::Weekday::thursday);
//
ABEL_FORCE_INLINE CivilDay NextWeekday(CivilDay cd, Weekday wd) {
  return CivilDay(time_internal::cctz::next_weekday(cd, wd));
}
ABEL_FORCE_INLINE CivilDay PrevWeekday(CivilDay cd, Weekday wd) {
  return CivilDay(time_internal::cctz::prev_weekday(cd, wd));
}

// GetYearDay()
//
// Returns the day-of-year for the given (realigned) civil-time value.
//
// Example:
//
//   abel::CivilDay a(2015, 1, 1);
//   int yd_jan_1 = abel::GetYearDay(a);   // yd_jan_1 = 1
//   abel::CivilDay b(2015, 12, 31);
//   int yd_dec_31 = abel::GetYearDay(b);  // yd_dec_31 = 365
//
ABEL_FORCE_INLINE int GetYearDay(CivilSecond cs) {
  return time_internal::cctz::get_yearday(cs);
}

// FormatCivilTime()
//
// Formats the given civil-time value into a string value of the following
// format:
//
//  Type        | Format
//  ---------------------------------
//  CivilSecond | YYYY-MM-DDTHH:MM:SS
//  CivilMinute | YYYY-MM-DDTHH:MM
//  CivilHour   | YYYY-MM-DDTHH
//  CivilDay    | YYYY-MM-DD
//  CivilMonth  | YYYY-MM
//  CivilYear   | YYYY
//
// Example:
//
//   abel::CivilDay d = abel::CivilDay(1969, 7, 20);
//   std::string day_string = abel::FormatCivilTime(d);  // "1969-07-20"
//
std::string FormatCivilTime(CivilSecond c);
std::string FormatCivilTime(CivilMinute c);
std::string FormatCivilTime(CivilHour c);
std::string FormatCivilTime(CivilDay c);
std::string FormatCivilTime(CivilMonth c);
std::string FormatCivilTime(CivilYear c);

// abel::ParseCivilTime()
//
// Parses a civil-time value from the specified `abel::string_view` into the
// passed output parameter. Returns `true` upon successful parsing.
//
// The expected form of the input string is as follows:
//
//  Type        | Format
//  ---------------------------------
//  CivilSecond | YYYY-MM-DDTHH:MM:SS
//  CivilMinute | YYYY-MM-DDTHH:MM
//  CivilHour   | YYYY-MM-DDTHH
//  CivilDay    | YYYY-MM-DD
//  CivilMonth  | YYYY-MM
//  CivilYear   | YYYY
//
// Example:
//
//   abel::CivilDay d;
//   bool ok = abel::ParseCivilTime("2018-01-02", &d); // OK
//
// Note that parsing will fail if the string's format does not match the
// expected type exactly. `ParseLenientCivilTime()` below is more lenient.
//
bool ParseCivilTime(abel::string_view s, CivilSecond* c);
bool ParseCivilTime(abel::string_view s, CivilMinute* c);
bool ParseCivilTime(abel::string_view s, CivilHour* c);
bool ParseCivilTime(abel::string_view s, CivilDay* c);
bool ParseCivilTime(abel::string_view s, CivilMonth* c);
bool ParseCivilTime(abel::string_view s, CivilYear* c);

// ParseLenientCivilTime()
//
// Parses any of the formats accepted by `abel::ParseCivilTime()`, but is more
// lenient if the format of the string does not exactly match the associated
// type.
//
// Example:
//
//   abel::CivilDay d;
//   bool ok = abel::ParseLenientCivilTime("1969-07-20", &d); // OK
//   ok = abel::ParseLenientCivilTime("1969-07-20T10", &d);   // OK: T10 floored
//   ok = abel::ParseLenientCivilTime("1969-07", &d);   // OK: day defaults to 1
//
bool ParseLenientCivilTime(abel::string_view s, CivilSecond* c);
bool ParseLenientCivilTime(abel::string_view s, CivilMinute* c);
bool ParseLenientCivilTime(abel::string_view s, CivilHour* c);
bool ParseLenientCivilTime(abel::string_view s, CivilDay* c);
bool ParseLenientCivilTime(abel::string_view s, CivilMonth* c);
bool ParseLenientCivilTime(abel::string_view s, CivilYear* c);

namespace time_internal {  // For functions found via ADL on civil-time tags.

// Streaming Operators
//
// Each civil-time type may be sent to an output stream using operator<<().
// The result matches the string produced by `FormatCivilTime()`.
//
// Example:
//
//   abel::CivilDay d = abel::CivilDay(1969, 7, 20);
//   std::cout << "Date is: " << d << "\n";
//
std::ostream& operator<<(std::ostream& os, CivilYear y);
std::ostream& operator<<(std::ostream& os, CivilMonth m);
std::ostream& operator<<(std::ostream& os, CivilDay d);
std::ostream& operator<<(std::ostream& os, CivilHour h);
std::ostream& operator<<(std::ostream& os, CivilMinute m);
std::ostream& operator<<(std::ostream& os, CivilSecond s);

}  // namespace time_internal

ABEL_NAMESPACE_END
}  // namespace abel

#endif  // ABEL_TIME_CIVIL_TIME_H_
