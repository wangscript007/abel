//

#ifndef ABEL_STRINGS_CHARCONV_H_
#define ABEL_STRINGS_CHARCONV_H_

#include <system_error>  // NOLINT(build/c++11)

#include <abel/base/profile.h>

namespace abel {
ABEL_NAMESPACE_BEGIN

// Workalike compatibilty version of std::chars_format from C++17.
//
// This is an bitfield enumerator which can be passed to abel::from_chars to
// configure the string-to-float conversion.
enum class chars_format {
  scientific = 1,
  fixed = 2,
  hex = 4,
  general = fixed | scientific,
};

// The return result of a string-to-number conversion.
//
// `ec` will be set to `invalid_argument` if a well-formed number was not found
// at the start of the input range, `result_out_of_range` if a well-formed
// number was found, but it was out of the representable range of the requested
// type, or to std::errc() otherwise.
//
// If a well-formed number was found, `ptr` is set to one past the sequence of
// characters that were successfully parsed.  If none was found, `ptr` is set
// to the `first` argument to from_chars.
struct from_chars_result {
  const char* ptr;
  std::errc ec;
};

// Workalike compatibilty version of std::from_chars from C++17.  Currently
// this only supports the `double` and `float` types.
//
// This interface incorporates the proposed resolutions for library issues
// DR 3080 and DR 3081.  If these are adopted with different wording,
// abel's behavior will change to match the standard.  (The behavior most
// likely to change is for DR 3081, which says what `value` will be set to in
// the case of overflow and underflow.  Code that wants to avoid possible
// breaking changes in this area should not depend on `value` when the returned
// from_chars_result indicates a range error.)
//
// Searches the range [first, last) for the longest matching pattern beginning
// at `first` that represents a floating point number.  If one is found, store
// the result in `value`.
//
// The matching pattern format is almost the same as that of strtod(), except
// that C locale is not respected, and an initial '+' character in the input
// range will never be matched.
//
// If `fmt` is set, it must be one of the enumerator values of the chars_format.
// (This is despite the fact that chars_format is a bitmask type.)  If set to
// `scientific`, a matching number must contain an exponent.  If set to `fixed`,
// then an exponent will never match.  (For example, the string "1e5" will be
// parsed as "1".)  If set to `hex`, then a hexadecimal float is parsed in the
// format that strtod() accepts, except that a "0x" prefix is NOT matched.
// (In particular, in `hex` mode, the input "0xff" results in the largest
// matching pattern "0".)
abel::from_chars_result from_chars(const char* first, const char* last,
                                   double& value,  // NOLINT
                                   chars_format fmt = chars_format::general);

abel::from_chars_result from_chars(const char* first, const char* last,
                                   float& value,  // NOLINT
                                   chars_format fmt = chars_format::general);

// std::chars_format is specified as a bitmask type, which means the following
// operations must be provided:
ABEL_FORCE_INLINE constexpr chars_format operator&(chars_format lhs, chars_format rhs) {
  return static_cast<chars_format>(static_cast<int>(lhs) &
                                   static_cast<int>(rhs));
}
ABEL_FORCE_INLINE constexpr chars_format operator|(chars_format lhs, chars_format rhs) {
  return static_cast<chars_format>(static_cast<int>(lhs) |
                                   static_cast<int>(rhs));
}
ABEL_FORCE_INLINE constexpr chars_format operator^(chars_format lhs, chars_format rhs) {
  return static_cast<chars_format>(static_cast<int>(lhs) ^
                                   static_cast<int>(rhs));
}
ABEL_FORCE_INLINE constexpr chars_format operator~(chars_format arg) {
  return static_cast<chars_format>(~static_cast<int>(arg));
}
ABEL_FORCE_INLINE chars_format& operator&=(chars_format& lhs, chars_format rhs) {
  lhs = lhs & rhs;
  return lhs;
}
ABEL_FORCE_INLINE chars_format& operator|=(chars_format& lhs, chars_format rhs) {
  lhs = lhs | rhs;
  return lhs;
}
ABEL_FORCE_INLINE chars_format& operator^=(chars_format& lhs, chars_format rhs) {
  lhs = lhs ^ rhs;
  return lhs;
}

ABEL_NAMESPACE_END
}  // namespace abel

#endif  // ABEL_STRINGS_CHARCONV_H_
