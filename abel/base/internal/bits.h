//

#ifndef ABEL_BASE_INTERNAL_BITS_H_
#define ABEL_BASE_INTERNAL_BITS_H_

// This file contains bitwise ops which are implementation details of various
// abel libraries.

#include <cstdint>

#include <abel/base/profile.h>

// Clang on Windows has __builtin_clzll; otherwise we need to use the
// windows intrinsic functions.
#if defined(_MSC_VER)
#include <intrin.h>
#if defined(_M_X64)
#pragma intrinsic(_BitScanReverse64)
#pragma intrinsic(_BitScanForward64)
#endif
#pragma intrinsic(_BitScanReverse)
#pragma intrinsic(_BitScanForward)
#endif

#include <abel/base/profile.h>


namespace abel {
ABEL_NAMESPACE_BEGIN
namespace base_internal {

ABEL_FORCE_INLINE int CountLeadingZeros64Slow(uint64_t n) {
  int zeroes = 60;
  if (n >> 32) {
    zeroes -= 32;
    n >>= 32;
  }
  if (n >> 16) {
    zeroes -= 16;
    n >>= 16;
  }
  if (n >> 8) {
    zeroes -= 8;
    n >>= 8;
  }
  if (n >> 4) {
    zeroes -= 4;
    n >>= 4;
  }
  return "\4\3\2\2\1\1\1\1\0\0\0\0\0\0\0"[n] + zeroes;
}

ABEL_FORCE_INLINE int CountLeadingZeros64(uint64_t n) {
#if defined(_MSC_VER) && defined(_M_X64)
  // MSVC does not have __buitin_clzll. Use _BitScanReverse64.
  unsigned long result = 0;  // NOLINT(runtime/int)
  if (_BitScanReverse64(&result, n)) {
    return 63 - result;
  }
  return 64;
#elif defined(_MSC_VER)
  // MSVC does not have __buitin_clzll. Compose two calls to _BitScanReverse
  unsigned long result = 0;  // NOLINT(runtime/int)
  if ((n >> 32) && _BitScanReverse(&result, n >> 32)) {
    return 31 - result;
  }
  if (_BitScanReverse(&result, n)) {
    return 63 - result;
  }
  return 64;
#elif defined(__GNUC__)
  // Use __builtin_clzll, which uses the following instructions:
  //  x86: bsr
  //  ARM64: clz
  //  PPC: cntlzd
  static_assert(sizeof(unsigned long long) == sizeof(n),  // NOLINT(runtime/int)
                "__builtin_clzll does not take 64-bit arg");

  // Handle 0 as a special case because __builtin_clzll(0) is undefined.
  if (n == 0) {
    return 64;
  }
  return __builtin_clzll(n);
#else
  return CountLeadingZeros64Slow(n);
#endif
}

ABEL_FORCE_INLINE int CountLeadingZeros32Slow(uint64_t n) {
  int zeroes = 28;
  if (n >> 16) {
    zeroes -= 16;
    n >>= 16;
  }
  if (n >> 8) {
    zeroes -= 8;
    n >>= 8;
  }
  if (n >> 4) {
    zeroes -= 4;
    n >>= 4;
  }
  return "\4\3\2\2\1\1\1\1\0\0\0\0\0\0\0"[n] + zeroes;
}

ABEL_FORCE_INLINE int CountLeadingZeros32(uint32_t n) {
#if defined(_MSC_VER)
  unsigned long result = 0;  // NOLINT(runtime/int)
  if (_BitScanReverse(&result, n)) {
    return 31 - result;
  }
  return 32;
#elif defined(__GNUC__)
  // Use __builtin_clz, which uses the following instructions:
  //  x86: bsr
  //  ARM64: clz
  //  PPC: cntlzd
  static_assert(sizeof(int) == sizeof(n),
                "__builtin_clz does not take 32-bit arg");

  // Handle 0 as a special case because __builtin_clz(0) is undefined.
  if (n == 0) {
    return 32;
  }
  return __builtin_clz(n);
#else
  return CountLeadingZeros32Slow(n);
#endif
}

ABEL_FORCE_INLINE int CountTrailingZerosNonZero64Slow(uint64_t n) {
  int c = 63;
  n &= ~n + 1;
  if (n & 0x00000000FFFFFFFF) c -= 32;
  if (n & 0x0000FFFF0000FFFF) c -= 16;
  if (n & 0x00FF00FF00FF00FF) c -= 8;
  if (n & 0x0F0F0F0F0F0F0F0F) c -= 4;
  if (n & 0x3333333333333333) c -= 2;
  if (n & 0x5555555555555555) c -= 1;
  return c;
}

ABEL_FORCE_INLINE int CountTrailingZerosNonZero64(uint64_t n) {
#if defined(_MSC_VER) && defined(_M_X64)
  unsigned long result = 0;  // NOLINT(runtime/int)
  _BitScanForward64(&result, n);
  return result;
#elif defined(_MSC_VER)
  unsigned long result = 0;  // NOLINT(runtime/int)
  if (static_cast<uint32_t>(n) == 0) {
    _BitScanForward(&result, n >> 32);
    return result + 32;
  }
  _BitScanForward(&result, n);
  return result;
#elif defined(__GNUC__)
  static_assert(sizeof(unsigned long long) == sizeof(n),  // NOLINT(runtime/int)
                "__builtin_ctzll does not take 64-bit arg");
  return __builtin_ctzll(n);
#else
  return CountTrailingZerosNonZero64Slow(n);
#endif
}

ABEL_FORCE_INLINE int CountTrailingZerosNonZero32Slow(uint32_t n) {
  int c = 31;
  n &= ~n + 1;
  if (n & 0x0000FFFF) c -= 16;
  if (n & 0x00FF00FF) c -= 8;
  if (n & 0x0F0F0F0F) c -= 4;
  if (n & 0x33333333) c -= 2;
  if (n & 0x55555555) c -= 1;
  return c;
}

ABEL_FORCE_INLINE int CountTrailingZerosNonZero32(uint32_t n) {
#if defined(_MSC_VER)
  unsigned long result = 0;  // NOLINT(runtime/int)
  _BitScanForward(&result, n);
  return result;
#elif defined(__GNUC__)
  static_assert(sizeof(int) == sizeof(n),
                "__builtin_ctz does not take 32-bit arg");
  return __builtin_ctz(n);
#else
  return CountTrailingZerosNonZero32Slow(n);
#endif
}

#undef ABEL_BASE_INTERNAL_FORCEINLINE

}  // namespace base_internal
ABEL_NAMESPACE_END
}  // namespace abel

#endif  // ABEL_BASE_INTERNAL_BITS_H_
