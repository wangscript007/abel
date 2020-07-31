//

#include <abel/random/engine/randen_hwaes.h>
#include <abel/base/profile.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <abel/log/logging.h>
#include <abel/hardware/aes_detect.h>
#include <abel/random/engine/randen_traits.h>
#include <abel/strings/fmt/printf.h>

namespace {

    using abel::random_internal::randen_hw_aes;
    using abel::random_internal::randen_traits;

    struct randen {
        static constexpr size_t kStateSizeT =
                randen_traits::kStateBytes / sizeof(uint64_t);
        uint64_t state[kStateSizeT];
        static constexpr size_t kSeedSizeT =
                randen_traits::kSeedBytes / sizeof(uint32_t);
        uint32_t seed[kSeedSizeT];
    };

    TEST(RandenHwAesTest, Default) {
        EXPECT_TRUE(abel::is_supports_aes());

        constexpr uint64_t kGolden[] = {
                0x6c6534090ee6d3ee, 0x044e2b9b9d5333c6, 0xc3c14f134e433977,
                0xdda9f47cd90410ee, 0x887bf3087fd8ca10, 0xf0b780f545c72912,
                0x15dbb1d37696599f, 0x30ec63baff3c6d59, 0xb29f73606f7f20a6,
                0x02808a316f49a54c, 0x3b8feaf9d5c8e50e, 0x9cbf605e3fd9de8a,
                0xc970ae1a78183bbb, 0xd8b2ffd356301ed5, 0xf4b327fe0fc73c37,
                0xcdfd8d76eb8f9a19, 0xc3a506eb91420c9d, 0xd5af05dd3eff9556,
                0x48db1bb78f83c4a1, 0x7023920e0d6bfe8c, 0x58d3575834956d42,
                0xed1ef4c26b87b840, 0x8eef32a23e0b2df3, 0x497cabf3431154fc,
                0x4e24370570029a8b, 0xd88b5749f090e5ea, 0xc651a582a970692f,
                0x78fcec2cbb6342f5, 0x463cb745612f55db, 0x352ee4ad1816afe3,
                0x026ff374c101da7e, 0x811ef0821c3de851,
        };

        alignas(16) randen d;
        memset(d.state, 0, sizeof(d.state));
        randen_hw_aes::generate(randen_hw_aes::get_keys(), d.state);

        uint64_t *id = d.state;
        for (const auto &elem : kGolden) {
            auto a = fmt::sprintf("%#x", elem);
            auto b = fmt::sprintf("%#x", *id++);
            EXPECT_EQ(a, b);
        }
    }

}  // namespace

int main(int argc, char *argv[]) {
    testing::InitGoogleTest(&argc, argv);

    DLOG_INFO("ABEL_HAVE_ACCELERATED_AES={}", ABEL_HAVE_ACCELERATED_AES);
    DLOG_INFO("ABEL_AES_DISPATCH={}",
                 ABEL_AES_DISPATCH);

#if defined(ABEL_ARCH_X86_64)
    DLOG_INFO("ABEL_ARCH_X86_64");
#elif defined(ABEL_ARCH_X86_32)
    DLOG_INFO("ABEL_ARCH_X86_32");
#elif defined(ABEL_ARCH_AARCH64)
    DLOG_INFO("ABEL_ARCH_AARCH64");
#elif defined(ABEL_ARCH_ARM)
    DLOG_INFO("ABEL_ARCH_ARM");
#elif defined(ABEL_ARCH_PPC)
    DLOG_INFO("ABEL_ARCH_PPC");
#else
    DLOG_INFO("ARCH Unknown");
#endif

    int x = abel::random_internal::has_randen_hw_aes_implementation();
    DLOG_INFO("has_randen_hw_aes_implementation = {}", x);

    int y = abel::is_supports_aes();
    DLOG_INFO("cpu_supports_randen_hw_aes = {}", x);

    if (!x || !y) {
        DLOG_INFO("Skipping randen HWAES tests.");
        return 0;
    }
    return RUN_ALL_TESTS();
}
