//

#ifndef ABEL_RANDOM_INTERNAL_SEED_MATERIAL_H_
#define ABEL_RANDOM_INTERNAL_SEED_MATERIAL_H_

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>

#include <abel/base/profile.h>
#include <abel/stats/random/internal/fast_uniform_bits.h>
#include <abel/types/optional.h>
#include <abel/types/span.h>

namespace abel {

    namespace random_internal {

// Returns the number of 32-bit blocks needed to contain the given number of
// bits.
        constexpr size_t seed_bits_to_blocks(size_t seed_size) {
            return (seed_size + 31) / 32;
        }

// Amount of entropy (measured in bits) used to instantiate a Seed Sequence,
// with which to create a URBG.
        constexpr size_t kEntropyBitsNeeded = 256;

// Amount of entropy (measured in 32-bit blocks) used to instantiate a Seed
// Sequence, with which to create a URBG.
        constexpr size_t kEntropyBlocksNeeded =
                random_internal::seed_bits_to_blocks(kEntropyBitsNeeded);

        static_assert(kEntropyBlocksNeeded > 0,
                      "Entropy used to seed URBGs must be nonzero.");

// Attempts to fill a span of uint32_t-values using an OS-provided source of
// true entropy (eg. /dev/urandom) into an array of uint32_t blocks of data. The
// resulting array may be used to initialize an instance of a class conforming
// to the C++ Standard "Seed Sequence" concept [rand.req.seedseq].
//
// If values.data() == nullptr, the behavior is undefined.
        ABEL_MUST_USE_RESULT
        bool read_seed_material_from_os_entropy(abel::span<uint32_t> values);

// Attempts to fill a span of uint32_t-values using variates generated by an
// existing instance of a class conforming to the C++ Standard "uniform Random
// Bit Generator" concept [rand.req.urng]. The resulting data may be used to
// initialize an instance of a class conforming to the C++ Standard
// "Seed Sequence" concept [rand.req.seedseq].
//
// If urbg == nullptr or values.data() == nullptr, the behavior is undefined.
        template<typename URBG>
        ABEL_MUST_USE_RESULT bool read_seed_material_from_urbg(
                URBG *urbg, abel::span<uint32_t> values) {
            random_internal::fast_uniform_bits<uint32_t> distr;

            assert(urbg != nullptr && values.data() != nullptr);
            if (urbg == nullptr || values.data() == nullptr) {
                return false;
            }

            for (uint32_t &seed_value : values) {
                seed_value = distr(*urbg);
            }
            return true;
        }

// Mixes given sequence of values with into given sequence of seed material.
// abel_time complexity of this function is O(sequence.size() *
// seed_material.size()).
//
// Algorithm is based on code available at
// https://gist.github.com/imneme/540829265469e673d045
// by Melissa O'Neill.
        void mix_into_seed_material(abel::span<const uint32_t> sequence,
                                 abel::span<uint32_t> seed_material);

// Returns salt value.
//
// Salt is obtained only once and stored in static variable.
//
// May return empty value if optaining the salt was not possible.
        abel::optional<uint32_t> get_salt_material();

    }  // namespace random_internal

}  // namespace abel

#endif  // ABEL_RANDOM_INTERNAL_SEED_MATERIAL_H_
