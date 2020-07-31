//
// Created by liyinbin on 2020/7/31.
//

#ifndef ABRL_UTILITY_EVERY_H_
#define ABRL_UTILITY_EVERY_H_

#include <atomic>
#include <abel/base/profile.h>

namespace abel {

    struct every_n {
        every_n(size_t n) : mod(n) {
            ABEL_ASSERT(n > 1);
        }

        bool feed() {
            size_t n = num.fetch_add(1);
            return n % mod == 0 ? true : false;
        }

        std::atomic<size_t> num;
        const size_t mod;
    };

    struct first_n {
        first_n(size_t n) : max_count(n){

        }

        bool feed() {
            size_t n = num.fetch_add(1);
            return n <= max_count ? true : false;
        }

        std::atomic<size_t> num;
        const size_t max_count;
    };
}
#endif //ABRL_UTILITY_EVERY_H_
