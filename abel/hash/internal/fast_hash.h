//
// Created by liyinbin on 2020/3/29.
//

#ifndef ABEL_HASH_INTERNAL_FAST_HASH_H_
#define ABEL_HASH_INTERNAL_FAST_HASH_H_

#include <abel/strings/string_view.h>
#include <cstdint>
#include <cstddef>

namespace abel {

    namespace hash_internal {
        uint32_t fast_hash(const char *src, size_t len);

    } //namespace hash_internal
} //namespace abel

#endif //ABEL_HASH_INTERNAL_FAST_HASH_H_
