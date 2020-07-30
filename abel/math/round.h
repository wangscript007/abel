//
// Created by liyinbin on 2020/7/30.
//

#ifndef ABEL_MATH_ROUND_H_
#define ABEL_MATH_ROUND_H_

#include <abel/base/profile.h>
#include <cmath>

namespace abel {

    // Can't use std::round() because it is only available in C++11.
    // Note that we ignore the possibility of floating-point over/underflow.
    template<typename Double>
    ABEL_FORCE_INLINE double round(Double d) {
        return d < 0 ? std::ceil(d - 0.5) : std::floor(d + 0.5);
    }
} //namespace abel

#endif //ABEL_MATH_ROUND_H_
