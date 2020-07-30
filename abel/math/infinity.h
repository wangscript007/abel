//
// Created by liyinbin on 2020/7/30.
//

#ifndef ABEL_MATH_INFINITY_H_
#define ABEL_MATH_INFINITY_H_

#include <abel/base/profile.h>
#include <limits>
#include <cmath>

namespace abel {

    // Can't use std::isinfinite() because it doesn't exist on windows.
    ABEL_FORCE_INLINE bool is_finite(double d) {
        if (std::isnan(d)) return false;
        return d != std::numeric_limits<double>::infinity() &&
               d != -std::numeric_limits<double>::infinity();
    }

}
#endif //ABEL_MATH_INFINITY_H_
