//
// Created by liyinbin on 2020/3/1.
//

#ifndef ABEL_MATH_ROTR_H_
#define ABEL_MATH_ROTR_H_

#include <abel/base/profile.h>
#include <cstdint>
#include <cstdlib>

namespace abel {


// rotr() - rotate bits right in 32-bit integers

//! rotr - generic implementation
    static ABEL_FORCE_INLINE uint32_t rotr_generic(const uint32_t &x, int i) {
        return (x >> static_cast<uint32_t>(i & 31)) |
               (x << static_cast<uint32_t>((32 - (i & 31)) & 31));
    }

#if (defined(__GNUC__) || defined(__clang__)) && (defined(__i386__) || defined(__x86_64__))

//! rotr - gcc/clang assembler
    static ABEL_FORCE_INLINE uint32_t rotr(const uint32_t &x, int i) {
        uint32_t x1 = x;
        asm ("rorl %%cl,%0" : "=r" (x1) : "0" (x1), "c" (i));
        return x1;
    }

#elif defined(_MSC_VER)

    //! rotr - MSVC intrinsic
    static ABEL_FORCE_INLINE uint32_t rotr(const uint32_t& x, int i) {
        return _rotr(x, i);
    }

#else

    //! rotr - generic
    static ABEL_FORCE_INLINE uint32_t rotr (const uint32_t &x, int i) {
        return rotr_generic(x, i);
    }

#endif


// rotr() - rotate bits right in 64-bit integers

//! rotr - generic implementation
    static ABEL_FORCE_INLINE uint64_t rotr_generic(const uint64_t &x, int i) {
        return (x >> static_cast<uint64_t>(i & 63)) |
               (x << static_cast<uint64_t>((64 - (i & 63)) & 63));
    }

#if (defined(__GNUC__) || defined(__clang__)) && defined(__x86_64__)

//! rotr - gcc/clang assembler
    static ABEL_FORCE_INLINE uint64_t rotr(const uint64_t &x, int i) {
        uint64_t x1 = x;
        asm ("rorq %%cl,%0" : "=r" (x1) : "0" (x1), "c" (i));
        return x1;
    }

#elif defined(_MSC_VER)

    //! rotr - MSVC intrinsic
    static ABEL_FORCE_INLINE uint64_t rotr(const uint64_t& x, int i) {
        return _rotr64(x, i);
    }

#else

    //! rotr - generic
    static ABEL_FORCE_INLINE uint64_t rotr (const uint64_t &x, int i) {
        return rotr_generic(x, i);
    }

#endif


} //namespace abel

#endif //ABEL_MATH_ROTR_H_
