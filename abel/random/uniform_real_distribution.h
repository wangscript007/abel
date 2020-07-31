//
// -----------------------------------------------------------------------------
// File: uniform_real_distribution.h
// -----------------------------------------------------------------------------
//
// This header defines a class for representing a uniform floating-point
// distribution over a half-open interval [a,b). You use this distribution in
// combination with an abel random bit generator to produce random values
// according to the rules of the distribution.
//
// `abel::uniform_real_distribution` is a drop-in replacement for the C++11
// `std::uniform_real_distribution` [rand.dist.uni.real] but is considerably
// faster than the libstdc++ implementation.
//
// Note: the standard-library version may occasionally return `1.0` when
// default-initialized. See https://bugs.llvm.org//show_bug.cgi?id=18767
// `abel::uniform_real_distribution` does not exhibit this behavior.

#ifndef ABEL_RANDOM_UNIFORM_REAL_DISTRIBUTION_H_
#define ABEL_RANDOM_UNIFORM_REAL_DISTRIBUTION_H_

#include <cassert>
#include <cmath>
#include <cstdint>
#include <istream>
#include <limits>
#include <type_traits>

#include <abel/meta/type_traits.h>
#include <abel/random/internal/fast_uniform_bits.h>
#include <abel/random/internal/generate_real.h>
#include <abel/random/internal/iostream_state_saver.h>

namespace abel {


// abel::uniform_real_distribution<T>
//
// This distribution produces random floating-point values uniformly distributed
// over the half-open interval [a, b).
//
// Example:
//
//   abel::bit_gen gen;
//
//   // Use the distribution to produce a value between 0.0 (inclusive)
//   // and 1.0 (exclusive).
//   double value = abel::uniform_real_distribution<double>(0, 1)(gen);
//
    template<typename RealType = double>
    class uniform_real_distribution {
    public:
        using result_type = RealType;

        class param_type {
        public:
            using distribution_type = uniform_real_distribution;

            explicit param_type(result_type lo = 0, result_type hi = 1)
                    : lo_(lo), hi_(hi), range_(hi - lo) {
                // [rand.dist.uni.real] preconditions 2 & 3
                assert(lo <= hi);
                // NOTE: For integral types, we can promote the range to an unsigned type,
                // which gives full width of the range. However for real (fp) types, this
                // is not possible, so value generation cannot use the full range of the
                // real type.
                assert(range_ <= (std::numeric_limits<result_type>::max)());
                assert(std::isfinite(range_));
            }

            result_type a() const { return lo_; }

            result_type b() const { return hi_; }

            friend bool operator==(const param_type &a, const param_type &b) {
                return a.lo_ == b.lo_ && a.hi_ == b.hi_;
            }

            friend bool operator!=(const param_type &a, const param_type &b) {
                return !(a == b);
            }

        private:
            friend class uniform_real_distribution;

            result_type lo_, hi_, range_;

            static_assert(std::is_floating_point<RealType>::value,
                          "Class-template abel::uniform_real_distribution<> must be "
                          "parameterized using a floating-point type.");
        };

        uniform_real_distribution() : uniform_real_distribution(0) {}

        explicit uniform_real_distribution(result_type lo, result_type hi = 1)
                : param_(lo, hi) {}

        explicit uniform_real_distribution(const param_type &param) : param_(param) {}

        // uniform_real_distribution<T>::reset()
        //
        // Resets the uniform real distribution. Note that this function has no effect
        // because the distribution already produces independent values.
        void reset() {}

        template<typename URBG>
        result_type operator()(URBG &gen) {  // NOLINT(runtime/references)
            return operator()(gen, param_);
        }

        template<typename URBG>
        result_type operator()(URBG &gen,  // NOLINT(runtime/references)
                               const param_type &p);

        result_type a() const { return param_.a(); }

        result_type b() const { return param_.b(); }

        param_type param() const { return param_; }

        void param(const param_type &params) { param_ = params; }

        result_type (min)() const { return a(); }

        result_type (max)() const { return b(); }

        friend bool operator==(const uniform_real_distribution &a,
                               const uniform_real_distribution &b) {
            return a.param_ == b.param_;
        }

        friend bool operator!=(const uniform_real_distribution &a,
                               const uniform_real_distribution &b) {
            return a.param_ != b.param_;
        }

    private:
        param_type param_;
        random_internal::fast_uniform_bits <uint64_t> fast_u64_;
    };

// -----------------------------------------------------------------------------
// Implementation details follow
// -----------------------------------------------------------------------------
    template<typename RealType>
    template<typename URBG>
    typename uniform_real_distribution<RealType>::result_type
    uniform_real_distribution<RealType>::operator()(
            URBG &gen, const param_type &p) {  // NOLINT(runtime/references)
        using random_internal::generate_positive_tag;
        using random_internal::generate_real_from_bits;
        using real_type =
        abel::conditional_t<std::is_same<RealType, float>::value, float, double>;

        while (true) {
            const result_type sample =
                    generate_real_from_bits<real_type, generate_positive_tag, true>(
                            fast_u64_(gen));
            const result_type res = p.a() + (sample * p.range_);
            if (res < p.b() || p.range_ <= 0 || !std::isfinite(p.range_)) {
                return res;
            }
            // else sample rejected, try again.
        }
    }

    template<typename CharT, typename Traits, typename RealType>
    std::basic_ostream<CharT, Traits> &operator<<(
            std::basic_ostream<CharT, Traits> &os,  // NOLINT(runtime/references)
            const uniform_real_distribution<RealType> &x) {
        auto saver = random_internal::make_ostream_state_saver(os);
        os.precision(random_internal::stream_precision_helper<RealType>::kPrecision);
        os << x.a() << os.fill() << x.b();
        return os;
    }

    template<typename CharT, typename Traits, typename RealType>
    std::basic_istream<CharT, Traits> &operator>>(
            std::basic_istream<CharT, Traits> &is,     // NOLINT(runtime/references)
            uniform_real_distribution<RealType> &x) {  // NOLINT(runtime/references)
        using param_type = typename uniform_real_distribution<RealType>::param_type;
        using result_type = typename uniform_real_distribution<RealType>::result_type;
        auto saver = random_internal::make_istream_state_saver(is);
        auto a = random_internal::read_floating_point<result_type>(is);
        if (is.fail()) return is;
        auto b = random_internal::read_floating_point<result_type>(is);
        if (!is.fail()) {
            x.param(param_type(a, b));
        }
        return is;
    }

}  // namespace abel

#endif  // ABEL_RANDOM_UNIFORM_REAL_DISTRIBUTION_H_