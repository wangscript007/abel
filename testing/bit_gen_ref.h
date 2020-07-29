//
//
// -----------------------------------------------------------------------------
// File: bit_gen_ref.h
// -----------------------------------------------------------------------------
//
// This header defines a bit generator "reference" class, for use in interfaces
// that take both abel (e.g. `abel::bit_gen`) and standard library (e.g.
// `std::mt19937`) bit generators.

#ifndef TEST_TESTING_BIT_GEN_REF_H_
#define TEST_TESTING_BIT_GEN_REF_H_

#include <abel/base/profile.h>
#include <abel/meta/type_traits.h>
#include <abel/random/internal/distribution_caller.h>
#include <abel/random/internal/fast_uniform_bits.h>
#include <testing/mocking_bit_gen_base.h>

namespace abel {

    namespace random_internal {

        template<typename URBG, typename = void, typename = void, typename = void>
        struct is_urbg : std::false_type {
        };

        template<typename URBG>
        struct is_urbg<
                URBG,
                abel::enable_if_t<std::is_same<
                        typename URBG::result_type,
                        typename std::decay<decltype((URBG::min)())>::type>::value>,
                abel::enable_if_t<std::is_same<
                        typename URBG::result_type,
                        typename std::decay<decltype((URBG::max)())>::type>::value>,
                abel::enable_if_t<std::is_same<
                        typename URBG::result_type,
                        typename std::decay<decltype(std::declval<URBG>()())>::type>::value>>
                : std::true_type {
        };

    }  // namespace random_internal

// -----------------------------------------------------------------------------
// abel::BitGenRef
// -----------------------------------------------------------------------------
//
// `abel::BitGenRef` is a type-erasing class that provides a generator-agnostic
// non-owning "reference" interface for use in place of any specific uniform
// random bit generator (URBG). This class may be used for both abel
// (e.g. `abel::bit_gen`, `abel::insecure_bit_gen`) and Standard library (e.g
// `std::mt19937`, `std::minstd_rand`) bit generators.
//
// Like other reference classes, `abel::BitGenRef` does not own the
// underlying bit generator, and the underlying instance must outlive the
// `abel::BitGenRef`.
//
// `abel::BitGenRef` is particularly useful when used with an
// `abel::MockingBitGen` to test specific paths in functions which use random
// values.
//
// Example:
//    void TakesBitGenRef(abel::BitGenRef gen) {
//      int x = abel::uniform<int>(gen, 0, 1000);
//    }
//
    class BitGenRef {
    public:
        using result_type = uint64_t;

        BitGenRef(const abel::BitGenRef &) = default;

        BitGenRef(abel::BitGenRef &&) = default;

        BitGenRef &operator=(const abel::BitGenRef &) = default;

        BitGenRef &operator=(abel::BitGenRef &&) = default;

        template<typename URBG,
                typename abel::enable_if_t<
                        (!std::is_same<URBG, BitGenRef>::value &&
                         random_internal::is_urbg<URBG>::value)> * = nullptr>
        BitGenRef(URBG &gen)  // NOLINT
                : mocked_gen_ptr_(MakeMockPointer(&gen)),
                  t_erased_gen_ptr_(reinterpret_cast<uintptr_t>(&gen)),
                  generate_impl_fn_(ImplFn < URBG > ) {
        }

        static constexpr result_type (min)() {
            return (std::numeric_limits<result_type>::min)();
        }

        static constexpr result_type (max)() {
            return (std::numeric_limits<result_type>::max)();
        }

        result_type operator()() { return generate_impl_fn_(t_erased_gen_ptr_); }

    private:
        friend struct abel::random_internal::distribution_caller<abel::BitGenRef>;
        using impl_fn = result_type (*)(uintptr_t);
        using mocker_base_t = abel::random_internal::MockingBitGenBase;

        // Convert an arbitrary URBG pointer into either a valid mocker_base_t
        // pointer or a nullptr.
        static ABEL_FORCE_INLINE mocker_base_t *MakeMockPointer(mocker_base_t *t) { return t; }

        static ABEL_FORCE_INLINE mocker_base_t *MakeMockPointer(void *) { return nullptr; }

        template<typename URBG>
        static result_type ImplFn(uintptr_t ptr) {
            // Ensure that the return values from operator() fill the entire
            // range promised by result_type, min() and max().
            abel::random_internal::fast_uniform_bits<result_type> fast_uniform_bits;
            return fast_uniform_bits(*reinterpret_cast<URBG *>(ptr));
        }

        mocker_base_t *mocked_gen_ptr_;
        uintptr_t t_erased_gen_ptr_;
        impl_fn generate_impl_fn_;
    };

    namespace random_internal {

        template<>
        struct distribution_caller<abel::BitGenRef> {
            template<typename DistrT, typename FormatT, typename... Args>
            static typename DistrT::result_type call(abel::BitGenRef *gen_ref,
                                                     Args &&... args) {
                auto *mock_ptr = gen_ref->mocked_gen_ptr_;
                if (mock_ptr == nullptr) {
                    DistrT dist(std::forward<Args>(args)...);
                    return dist(*gen_ref);
                } else {
                    return mock_ptr->template call<DistrT, FormatT>(
                            std::forward<Args>(args)...);
                }
            }
        };

    }  // namespace random_internal

}  // namespace abel

#endif  // TEST_TESTING_BIT_GEN_REF_H_
