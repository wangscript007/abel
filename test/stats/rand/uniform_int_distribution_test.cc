//

#include <abel/stats/random/uniform_int_distribution.h>

#include <cmath>
#include <cstdint>
#include <iterator>
#include <random>
#include <sstream>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <abel/log/raw_logging.h>
#include <test/testing/chi_square.h>
#include <test/testing/distribution_test_util.h>
#include <abel/stats/random/engine/sequence_urbg.h>
#include <abel/stats/random/random.h>
#include <abel/strings/str_cat.h>

namespace {

    template<typename IntType>
    class UniformIntDistributionTest : public ::testing::Test {
    };

    using IntTypes = ::testing::Types<int8_t, uint8_t, int16_t, uint16_t, int32_t,
            uint32_t, int64_t, uint64_t>;
    TYPED_TEST_SUITE(UniformIntDistributionTest, IntTypes);

    TYPED_TEST(UniformIntDistributionTest, ParamSerializeTest) {
        // This test essentially ensures that the parameters serialize,
        // not that the values generated cover the full range.
        using Limits = std::numeric_limits<TypeParam>;
        using param_type =
        typename abel::uniform_int_distribution<TypeParam>::param_type;
        const TypeParam kMin = std::is_unsigned<TypeParam>::value ? 37 : -105;
        const TypeParam kNegOneOrZero = std::is_unsigned<TypeParam>::value ? 0 : -1;

        constexpr int kCount = 1000;
        abel::insecure_bit_gen gen;
        for (const auto &param : {
                param_type(),
                param_type(2, 2),  // Same
                param_type(9, 32),
                param_type(kMin, 115),
                param_type(kNegOneOrZero, Limits::max()),
                param_type(Limits::min(), Limits::max()),
                param_type(Limits::lowest(), Limits::max()),
                param_type(Limits::min() + 1, Limits::max() - 1),
        }) {
            const auto a = param.a();
            const auto b = param.b();
            abel::uniform_int_distribution<TypeParam> before(a, b);
            EXPECT_EQ(before.a(), param.a());
            EXPECT_EQ(before.b(), param.b());

            {
                // Initialize via param_type
                abel::uniform_int_distribution<TypeParam> via_param(param);
                EXPECT_EQ(via_param, before);
            }

            // Initialize via iostreams
            std::stringstream ss;
            ss << before;

            abel::uniform_int_distribution<TypeParam> after(Limits::min() + 3,
                                                            Limits::max() - 5);

            EXPECT_NE(before.a(), after.a());
            EXPECT_NE(before.b(), after.b());
            EXPECT_NE(before.param(), after.param());
            EXPECT_NE(before, after);

            ss >> after;

            EXPECT_EQ(before.a(), after.a());
            EXPECT_EQ(before.b(), after.b());
            EXPECT_EQ(before.param(), after.param());
            EXPECT_EQ(before, after);

            // Smoke test.
            auto sample_min = after.max();
            auto sample_max = after.min();
            for (int i = 0; i < kCount; i++) {
                auto sample = after(gen);
                EXPECT_GE(sample, after.min());
                EXPECT_LE(sample, after.max());
                if (sample > sample_max) {
                    sample_max = sample;
                }
                if (sample < sample_min) {
                    sample_min = sample;
                }
            }
            std::string msg = abel::string_cat("Range: ", +sample_min, ", ", +sample_max);
            ABEL_RAW_LOG(INFO, "%s", msg.c_str());
        }
    }

    TYPED_TEST(UniformIntDistributionTest, ViolatesPreconditionsDeathTest) {
#if GTEST_HAS_DEATH_TEST
        // Hi < Lo
        EXPECT_DEBUG_DEATH({ abel::uniform_int_distribution<TypeParam> dist(10, 1); },
                           "");
#endif  // GTEST_HAS_DEATH_TEST
#if defined(NDEBUG)
        // opt-mode, for invalid parameters, will generate a garbage value,
        // but should not enter an infinite loop.
        abel::insecure_bit_gen gen;
        abel::uniform_int_distribution<TypeParam> dist(10, 1);
        auto x = dist(gen);

        // Any value will generate a non-empty std::string.
        EXPECT_FALSE(abel::string_cat(+x).empty()) << x;
#endif  // NDEBUG
    }

    TYPED_TEST(UniformIntDistributionTest, TestMoments) {
        constexpr int kSize = 100000;
        using Limits = std::numeric_limits<TypeParam>;
        using param_type =
        typename abel::uniform_int_distribution<TypeParam>::param_type;

        abel::insecure_bit_gen rng;
        std::vector<double> values(kSize);
        for (const auto &param :
                {param_type(0, Limits::max()), param_type(13, 127)}) {
            abel::uniform_int_distribution<TypeParam> dist(param);
            for (int i = 0; i < kSize; i++) {
                const auto sample = dist(rng);
                ASSERT_LE(dist.param().a(), sample);
                ASSERT_GE(dist.param().b(), sample);
                values[i] = sample;
            }

            auto moments = abel::random_internal::ComputeDistributionMoments(values);
            const double a = dist.param().a();
            const double b = dist.param().b();
            const double n = (b - a + 1);
            const double mean = (a + b) / 2;
            const double var = ((b - a + 1) * (b - a + 1) - 1) / 12;
            const double kurtosis = 3 - 6 * (n * n + 1) / (5 * (n * n - 1));

            // TODO(ahh): this is not the right bound
            // empirically validated with --runs_per_test=10000.
            EXPECT_NEAR(mean, moments.mean, 0.01 * var);
            EXPECT_NEAR(var, moments.variance, 0.015 * var);
            EXPECT_NEAR(0.0, moments.skewness, 0.025);
            EXPECT_NEAR(kurtosis, moments.kurtosis, 0.02 * kurtosis);
        }
    }

    TYPED_TEST(UniformIntDistributionTest, ChiSquaredTest50) {
        using abel::random_internal::kChiSquared;

        constexpr size_t kTrials = 1000;
        constexpr int kBuckets = 50;  // inclusive, so actally +1
        constexpr double kExpected =
                static_cast<double>(kTrials) / static_cast<double>(kBuckets);

        // Empirically validated with --runs_per_test=10000.
        const int kThreshold =
                abel::random_internal::chi_square_value(kBuckets, 0.999999);

        const TypeParam min = std::is_unsigned<TypeParam>::value ? 37 : -37;
        const TypeParam max = min + kBuckets;

        abel::insecure_bit_gen rng;
        abel::uniform_int_distribution<TypeParam> dist(min, max);

        std::vector<int32_t> counts(kBuckets + 1, 0);
        for (size_t i = 0; i < kTrials; i++) {
            auto x = dist(rng);
            counts[x - min]++;
        }
        double chi_square = abel::random_internal::chi_square_with_expected(
                std::begin(counts), std::end(counts), kExpected);
        if (chi_square > kThreshold) {
            double p_value =
                    abel::random_internal::chi_square_p_value(chi_square, kBuckets);

            // Chi-squared test failed. Output does not appear to be uniform.
            std::string msg;
            for (const auto &a : counts) {
                abel::string_append(&msg, a, "\n");
            }
            abel::string_append(&msg, kChiSquared, " p-value ", p_value, "\n");
            abel::string_append(&msg, "High ", kChiSquared, " value: ", chi_square, " > ",
                                kThreshold);
            ABEL_RAW_LOG(INFO, "%s", msg.c_str());
            FAIL() << msg;
        }
    }

    TEST(UniformIntDistributionTest, StabilityTest) {
        // abel::uniform_int_distribution stability relies only on integer operations.
        abel::random_internal::sequence_urbg urbg(
                {0x0003eb76f6f7f755ull, 0xFFCEA50FDB2F953Bull, 0xC332DDEFBE6C5AA5ull,
                 0x6558218568AB9702ull, 0x2AEF7DAD5B6E2F84ull, 0x1521B62829076170ull,
                 0xECDD4775619F1510ull, 0x13CCA830EB61BD96ull, 0x0334FE1EAA0363CFull,
                 0xB5735C904C70A239ull, 0xD59E9E0BCBAADE14ull, 0xEECC86BC60622CA7ull});

        std::vector<int> output(12);

        {
            abel::uniform_int_distribution<int32_t> dist(0, 4);
            for (auto &v : output) {
                v = dist(urbg);
            }
        }
        EXPECT_EQ(12, urbg.invocations());
        EXPECT_THAT(output, testing::ElementsAre(4, 4, 3, 2, 1, 0, 1, 4, 3, 1, 3, 1));

        {
            urbg.reset();
            abel::uniform_int_distribution<int32_t> dist(0, 100);
            for (auto &v : output) {
                v = dist(urbg);
            }
        }
        EXPECT_EQ(12, urbg.invocations());
        EXPECT_THAT(output, testing::ElementsAre(97, 86, 75, 41, 36, 16, 38, 92, 67,
                                                 30, 80, 38));

        {
            urbg.reset();
            abel::uniform_int_distribution<int32_t> dist(0, 10000);
            for (auto &v : output) {
                v = dist(urbg);
            }
        }
        EXPECT_EQ(12, urbg.invocations());
        EXPECT_THAT(output, testing::ElementsAre(9648, 8562, 7439, 4089, 3571, 1602,
                                                 3813, 9195, 6641, 2986, 7956, 3765));
    }

}  // namespace
