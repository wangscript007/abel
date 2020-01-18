//

#ifndef ABEL_RANDOM_INTERNAL_FASTMATH_H_
#define ABEL_RANDOM_INTERNAL_FASTMATH_H_

// This file contains fast math functions (bitwise ops as well as some others)
// which are implementation details of various abel random number distributions.

#include <cassert>
#include <cmath>
#include <cstdint>

#include <abel/base/internal/bits.h>

namespace abel {
ABEL_NAMESPACE_BEGIN
namespace random_internal {

// Returns the position of the first bit set.
ABEL_FORCE_INLINE int LeadingSetBit(uint64_t n) {
  return 64 - base_internal::CountLeadingZeros64(n);
}

// Compute log2(n) using integer operations.
// While std::log2 is more accurate than std::log(n) / std::log(2), for
// very large numbers--those close to std::numeric_limits<uint64_t>::max() - 2,
// for instance--std::log2 rounds up rather than down, which introduces
// definite skew in the results.
ABEL_FORCE_INLINE int IntLog2Floor(uint64_t n) {
  return (n <= 1) ? 0 : (63 - base_internal::CountLeadingZeros64(n));
}
ABEL_FORCE_INLINE int IntLog2Ceil(uint64_t n) {
  return (n <= 1) ? 0 : (64 - base_internal::CountLeadingZeros64(n - 1));
}

ABEL_FORCE_INLINE double StirlingLogFactorial(double n) {
  assert(n >= 1);
  // Using Stirling's approximation.
  constexpr double kLog2PI = 1.83787706640934548356;
  const double logn = std::log(n);
  const double ninv = 1.0 / static_cast<double>(n);
  return n * logn - n + 0.5 * (kLog2PI + logn) + (1.0 / 12.0) * ninv -
         (1.0 / 360.0) * ninv * ninv * ninv;
}

// Rotate value right.
//
// We only implement the uint32_t / uint64_t versions because
// 1) those are the only ones we use, and
// 2) those are the only ones where clang detects the rotate idiom correctly.
ABEL_FORCE_INLINE constexpr uint32_t rotr(uint32_t value, uint8_t bits) {
  return (value >> (bits & 31)) | (value << ((-bits) & 31));
}
ABEL_FORCE_INLINE constexpr uint64_t rotr(uint64_t value, uint8_t bits) {
  return (value >> (bits & 63)) | (value << ((-bits) & 63));
}

}  // namespace random_internal
ABEL_NAMESPACE_END
}  // namespace abel

#endif  // ABEL_RANDOM_INTERNAL_FASTMATH_H_
