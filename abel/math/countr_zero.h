//
// Created by liyinbin on 2020/3/1.
//

#ifndef ABEL_MATH_COUNTR_ZERO_H_
#define ABEL_MATH_COUNTR_ZERO_H_

#include <abel/base/profile.h>

namespace abel {


/**
 * @brief generic implementation
 * @tparam Integral todo
 * @param x todo
 * @return todo
 */
    template<typename Integral>
    static ABEL_FORCE_INLINE unsigned countr_zero_template(Integral x) {
        if (x == 0)
            return 8 * sizeof(x);
        unsigned r = 0;
        while ((x & static_cast<Integral>(1)) == 0)
            x >>= 1, ++r;
        return r;
    }

/******************************************************************************/

    template<typename Integral>
    ABEL_FORCE_INLINE unsigned countr_zero(Integral x);

//! countr_zero (count trailing zeros)
    template<>
    ABEL_FORCE_INLINE unsigned countr_zero<char>(char i) {
        return countr_zero_template(i);
    }

//! countr_zero (count trailing zeros)
    template<>
    ABEL_FORCE_INLINE unsigned countr_zero<unsigned char>(unsigned char i) {
        return countr_zero_template(i);
    }

#if defined(ABEL_COMPILER_GNUC) || defined(ABEL_COMPILER_CLANG)

    //! countr_zero (count trailing zeros)
    template<>
    ABEL_FORCE_INLINE unsigned countr_zero<unsigned>(unsigned i) {
        if (i == 0)
            return 8 * sizeof(i);
        return static_cast<unsigned>(__builtin_ctz(i));
    }

//! countr_zero (count trailing zeros)
    template<>
    ABEL_FORCE_INLINE unsigned countr_zero<int>(int i) {
        return countr_zero(static_cast<unsigned>(i));
    }

//! countr_zero (count trailing zeros)
    template<>
    ABEL_FORCE_INLINE unsigned countr_zero<unsigned long>(unsigned long i) {
        if (i == 0)
            return 8 * sizeof(i);
        return static_cast<unsigned>(__builtin_ctzl(i));
    }

//! countr_zero (count trailing zeros)
    template<>
    ABEL_FORCE_INLINE unsigned countr_zero<long>(long i) {
        return countr_zero(static_cast<unsigned long>(i));
    }

//! countr_zero (count trailing zeros)
    template<>
    ABEL_FORCE_INLINE unsigned countr_zero<unsigned long long>(unsigned long long i) {
        if (i == 0)
            return 8 * sizeof(i);
        return static_cast<unsigned>(__builtin_ctzll(i));
    }

//! countr_zero (count trailing zeros)
    template<>
    ABEL_FORCE_INLINE unsigned countr_zero<long long>(long long i) {
        return countr_zero(static_cast<unsigned long long>(i));
    }

#else

    //! countr_zero (count trailing zeros)
    template<>
    ABEL_FORCE_INLINE unsigned countr_zero<int> (int i) {
        return countr_zero_template(i);
    }

    //! countr_zero (count trailing zeros)
    template<>
    ABEL_FORCE_INLINE unsigned countr_zero<unsigned> (unsigned i) {
        return countr_zero_template(i);
    }

    //! countr_zero (count trailing zeros)
    template<>
    ABEL_FORCE_INLINE unsigned countr_zero<long> (long i) {
        return countr_zero_template(i);
    }

    //! countr_zero (count trailing zeros)
    template<>
    ABEL_FORCE_INLINE unsigned countr_zero<unsigned long> (unsigned long i) {
        return countr_zero_template(i);
    }

    //! countr_zero (count trailing zeros)
    template<>
    ABEL_FORCE_INLINE unsigned countr_zero<long long> (long long i) {
        return countr_zero_template(i);
    }

    //! countr_zero (count trailing zeros)
    template<>
    ABEL_FORCE_INLINE unsigned countr_zero<unsigned long long> (unsigned long long i) {
        return countr_zero_template(i);
    }

#endif
} //namespace abel

#endif //ABEL_MATH_COUNTR_ZERO_H_

