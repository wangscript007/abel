//

#include <abel/stats/random/engine/pool_urbg.h>

#include <algorithm>
#include <bitset>
#include <cmath>
#include <cstdint>
#include <iterator>

#include <gtest/gtest.h>
#include <abel/asl/type_traits.h>
#include <abel/types/span.h>

using abel::random_internal::pool_urbg;
using abel::random_internal::randen_pool;

namespace {

// is_randen_pool trait is true when parameterized by an randen_pool
    template<typename T>
    using is_randen_pool = typename abel::disjunction<  //
            std::is_same<T, randen_pool<uint8_t>>,           //
            std::is_same<T, randen_pool<uint16_t>>,          //
            std::is_same<T, randen_pool<uint32_t>>,          //
            std::is_same<T, randen_pool<uint64_t>>>;         //

// MyFill either calls randen_pool::Fill() or std::generate(..., rng)
    template<typename T, typename V>
    typename abel::enable_if_t<abel::negation<is_randen_pool<T>>::value, void>  //
    MyFill(T &rng, abel::span<V> data) {  // NOLINT(runtime/references)
        std::generate(std::begin(data), std::end(data), rng);
    }

    template<typename T, typename V>
    typename abel::enable_if_t<is_randen_pool<T>::value, void>  //
    MyFill(T &rng, abel::span<V> data) {  // NOLINT(runtime/references)
        rng.fill(data);
    }

    template<typename EngineType>
    class PoolURBGTypedTest : public ::testing::Test {
    };

    using EngineTypes = ::testing::Types<  //
            randen_pool<uint8_t>,               //
            randen_pool<uint16_t>,              //
            randen_pool<uint32_t>,              //
            randen_pool<uint64_t>,              //
            pool_urbg<uint8_t, 2>,              //
            pool_urbg<uint16_t, 2>,             //
            pool_urbg<uint32_t, 2>,             //
            pool_urbg<uint64_t, 2>,             //
            pool_urbg<unsigned int, 8>,         // NOLINT(runtime/int)
            pool_urbg<unsigned long, 8>,        // NOLINT(runtime/int)
            pool_urbg<unsigned long int, 4>,    // NOLINT(runtime/int)
            pool_urbg<unsigned long long, 4>>;  // NOLINT(runtime/int)

    TYPED_TEST_SUITE(PoolURBGTypedTest, EngineTypes);

// This test is checks that the engines meet the URBG interface requirements
// defined in [rand.req.urbg].
    TYPED_TEST(PoolURBGTypedTest, URBGInterface) {
        using E = TypeParam;
        using T = typename E::result_type;

        static_assert(std::is_copy_constructible<E>::value,
                      "engine must be copy constructible");

        static_assert(abel::is_copy_assignable<E>::value,
                      "engine must be copy assignable");

        E e;
        const E x;

        e();

        static_assert(std::is_same<decltype(e()), T>::value,
                      "return type of operator() must be result_type");

        E u0(x);
        u0();

        E u1 = e;
        u1();
    }

// This validates that sequences are independent.
    TYPED_TEST(PoolURBGTypedTest, VerifySequences) {
        using E = TypeParam;
        using result_type = typename E::result_type;

        E rng;
        (void) rng();  // Discard one value.

        constexpr int kNumOutputs = 64;
        result_type a[kNumOutputs];
        result_type b[kNumOutputs];
        std::fill(std::begin(b), std::end(b), 0);

        // Fill a using Fill or generate, depending on the engine type.
        {
            E x = rng;
            MyFill(x, abel::make_span(a));
        }

        // Fill b using std::generate().
        {
            E x = rng;
            std::generate(std::begin(b), std::end(b), x);
        }

        // Test that generated sequence changed as sequence of bits, i.e. if about
        // half of the bites were flipped between two non-correlated values.
        size_t changed_bits = 0;
        size_t unchanged_bits = 0;
        size_t total_set = 0;
        size_t total_bits = 0;
        size_t equal_count = 0;
        for (size_t i = 0; i < kNumOutputs; ++i) {
            equal_count += (a[i] == b[i]) ? 1 : 0;
            std::bitset<sizeof(result_type) * 8> bitset(a[i] ^ b[i]);
            changed_bits += bitset.count();
            unchanged_bits += bitset.size() - bitset.count();

            std::bitset<sizeof(result_type) * 8> a_set(a[i]);
            std::bitset<sizeof(result_type) * 8> b_set(b[i]);
            total_set += a_set.count() + b_set.count();
            total_bits += 2 * 8 * sizeof(result_type);
        }
        // On average, half the bits are changed between two calls.
        EXPECT_LE(changed_bits, 0.60 * (changed_bits + unchanged_bits));
        EXPECT_GE(changed_bits, 0.40 * (changed_bits + unchanged_bits));

        // verify using a quick normal-approximation to the binomial.
        EXPECT_NEAR(total_set, total_bits * 0.5, 4 * std::sqrt(total_bits))
                            << "@" << total_set / static_cast<double>(total_bits);

        // Also, A[i] == B[i] with probability (1/range) * N.
        // Give this a pretty wide latitude, though.
        const double kExpected = kNumOutputs / (1.0 * sizeof(result_type) * 8);
        EXPECT_LE(equal_count, 1.0 + kExpected);
    }

}  // namespace

/*
$ nanobenchmarks 1 randen_pool construct
$ nanobenchmarks 1 pool_urbg construct

randen_pool<uint32_t> | 1    | 1000 |    48482.00 ticks | 48.48 ticks | 13.9 ns
randen_pool<uint32_t> | 10   | 2000 |  1028795.00 ticks | 51.44 ticks | 14.7 ns
randen_pool<uint32_t> | 100  | 1000 |  5119968.00 ticks | 51.20 ticks | 14.6 ns
randen_pool<uint32_t> | 1000 |  500 | 25867936.00 ticks | 51.74 ticks | 14.8 ns

randen_pool<uint64_t> | 1    | 1000 |    49921.00 ticks | 49.92 ticks | 14.3 ns
randen_pool<uint64_t> | 10   | 2000 |  1208269.00 ticks | 60.41 ticks | 17.3 ns
randen_pool<uint64_t> | 100  | 1000 |  5844955.00 ticks | 58.45 ticks | 16.7 ns
randen_pool<uint64_t> | 1000 |  500 | 28767404.00 ticks | 57.53 ticks | 16.4 ns

pool_urbg<uint32_t,8> | 1    | 1000 |    86431.00 ticks | 86.43 ticks | 24.7 ns
pool_urbg<uint32_t,8> | 10   | 1000 |   206191.00 ticks | 20.62 ticks |  5.9 ns
pool_urbg<uint32_t,8> | 100  | 1000 |  1516049.00 ticks | 15.16 ticks |  4.3 ns
pool_urbg<uint32_t,8> | 1000 |  500 |  7613936.00 ticks | 15.23 ticks |  4.4 ns

pool_urbg<uint64_t,4> | 1    | 1000 |    96668.00 ticks | 96.67 ticks | 27.6 ns
pool_urbg<uint64_t,4> | 10   | 1000 |   282423.00 ticks | 28.24 ticks |  8.1 ns
pool_urbg<uint64_t,4> | 100  | 1000 |  2609587.00 ticks | 26.10 ticks |  7.5 ns
pool_urbg<uint64_t,4> | 1000 |  500 | 12408757.00 ticks | 24.82 ticks |  7.1 ns

*/
