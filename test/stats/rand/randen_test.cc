//

#include <abel/stats/random/engine/randen.h>

#include <cstring>

#include <gtest/gtest.h>
#include <abel/meta/type_traits.h>

namespace {

    using abel::random_internal::randen;

// Local state parameters.
    constexpr size_t kStateSizeT = randen::kStateBytes / sizeof(uint64_t);

    TEST(RandenTest, CopyAndMove) {
        static_assert(std::is_copy_constructible<randen>::value,
                      "randen must be copy constructible");

        static_assert(abel::is_copy_assignable<randen>::value,
                      "randen must be copy assignable");

        static_assert(std::is_move_constructible<randen>::value,
                      "randen must be move constructible");

        static_assert(abel::is_move_assignable<randen>::value,
                      "randen must be move assignable");
    }

    TEST(RandenTest, Default) {
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

        alignas(16) uint64_t state[kStateSizeT];
        std::memset(state, 0, sizeof(state));

        randen r;
        r.Generate(state);

        auto id = std::begin(state);
        for (const auto &elem : kGolden) {
            EXPECT_EQ(elem, *id++);
        }
    }

}  // namespace