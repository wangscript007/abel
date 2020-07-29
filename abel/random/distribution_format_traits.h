//
//
#ifndef ABEL_RANDOM_DISTRIBUTION_FORMAT_TRAITS_H_
#define ABEL_RANDOM_DISTRIBUTION_FORMAT_TRAITS_H_

#include <string>
#include <tuple>
#include <typeinfo>

#include <abel/meta/type_traits.h>
#include <abel/random/bernoulli_distribution.h>
#include <abel/random/beta_distribution.h>
#include <abel/random/exponential_distribution.h>
#include <abel/random/gaussian_distribution.h>
#include <abel/random/log_uniform_int_distribution.h>
#include <abel/random/poisson_distribution.h>
#include <abel/random/uniform_int_distribution.h>
#include <abel/random/uniform_real_distribution.h>
#include <abel/random/zipf_distribution.h>
#include <abel/strings/str_cat.h>
#include <abel/strings/str_join.h>
#include <abel/strings/string_view.h>
#include <abel/utility/span.h>

namespace abel {


    struct interval_closed_closed_tag;
    struct interval_closed_open_tag;
    struct interval_open_closed_tag;
    struct interval_open_open_tag;

    namespace random_internal {

// scalar_type_name defines a preferred hierarchy of preferred type names for
// scalars, and is evaluated at compile time for the specific type
// specialization.
        template<typename T>
        constexpr const char *scalar_type_name() {
            static_assert(std::is_integral<T>() || std::is_floating_point<T>(), "");
            // clang-format off
            return
                    std::is_same<T, float>::value ? "float" :
                    std::is_same<T, double>::value ? "double" :
                    std::is_same<T, long double>::value ? "long double" :
                    std::is_same<T, bool>::value ? "bool" :
                    std::is_signed<T>::value && sizeof(T) == 1 ? "int8_t" :
                    std::is_signed<T>::value && sizeof(T) == 2 ? "int16_t" :
                    std::is_signed<T>::value && sizeof(T) == 4 ? "int32_t" :
                    std::is_signed<T>::value && sizeof(T) == 8 ? "int64_t" :
                    std::is_unsigned<T>::value && sizeof(T) == 1 ? "uint8_t" :
                    std::is_unsigned<T>::value && sizeof(T) == 2 ? "uint16_t" :
                    std::is_unsigned<T>::value && sizeof(T) == 4 ? "uint32_t" :
                    std::is_unsigned<T>::value && sizeof(T) == 8 ? "uint64_t" :
                    "undefined";
            // clang-format on

            // NOTE: It would be nice to use typeid(T).name(), but that's an
            // implementation-defined attribute which does not necessarily
            // correspond to a name. We could potentially demangle it
            // using, e.g. abi::__cxa_demangle.
        }

// Distribution traits used by distribution_caller and internal implementation
// details of the mocking framework.
/*
struct distribution_format_traits {
   // Returns the parameterized name of the distribution function.
   static constexpr const char* FunctionName()
   // Format DistrT parameters.
   static std::string FormatArgs(DistrT& dist);
   // Format DistrT::result_type results.
   static std::string FormatResults(DistrT& dist);
};
*/
        template<typename DistrT>
        struct distribution_format_traits;

        template<typename R>
        struct distribution_format_traits<abel::uniform_int_distribution<R>> {
            using distribution_t = abel::uniform_int_distribution<R>;
            using result_t = typename distribution_t::result_type;

            static constexpr const char *Name() { return "uniform"; }

            static std::string FunctionName() {
                return abel::string_cat(Name(), "<", scalar_type_name<R>(), ">");
            }

            static std::string FormatArgs(const distribution_t &d) {
                return abel::string_cat("abel::IntervalClosedClosed, ", (d.min)(), ", ",
                                        (d.max)());
            }

            static std::string FormatResults(abel::span<const result_t> results) {
                return abel::string_join(results, ", ");
            }
        };

        template<typename R>
        struct distribution_format_traits<abel::uniform_real_distribution<R>> {
            using distribution_t = abel::uniform_real_distribution<R>;
            using result_t = typename distribution_t::result_type;

            static constexpr const char *Name() { return "uniform"; }

            static std::string FunctionName() {
                return abel::string_cat(Name(), "<", scalar_type_name<R>(), ">");
            }

            static std::string FormatArgs(const distribution_t &d) {
                return abel::string_cat((d.min)(), ", ", (d.max)());
            }

            static std::string FormatResults(abel::span<const result_t> results) {
                return abel::string_join(results, ", ");
            }
        };

        template<typename R>
        struct distribution_format_traits<abel::exponential_distribution<R>> {
            using distribution_t = abel::exponential_distribution<R>;
            using result_t = typename distribution_t::result_type;

            static constexpr const char *Name() { return "Exponential"; }

            static std::string FunctionName() {
                return abel::string_cat(Name(), "<", scalar_type_name<R>(), ">");
            }

            static std::string FormatArgs(const distribution_t &d) {
                return abel::string_cat(d.lambda());
            }

            static std::string FormatResults(abel::span<const result_t> results) {
                return abel::string_join(results, ", ");
            }
        };

        template<typename R>
        struct distribution_format_traits<abel::poisson_distribution<R>> {
            using distribution_t = abel::poisson_distribution<R>;
            using result_t = typename distribution_t::result_type;

            static constexpr const char *Name() { return "Poisson"; }

            static std::string FunctionName() {
                return abel::string_cat(Name(), "<", scalar_type_name<R>(), ">");
            }

            static std::string FormatArgs(const distribution_t &d) {
                return abel::string_cat(d.mean());
            }

            static std::string FormatResults(abel::span<const result_t> results) {
                return abel::string_join(results, ", ");
            }
        };

        template<>
        struct distribution_format_traits<abel::bernoulli_distribution> {
            using distribution_t = abel::bernoulli_distribution;
            using result_t = typename distribution_t::result_type;

            static constexpr const char *Name() { return "Bernoulli"; }

            static constexpr const char *FunctionName() { return Name(); }

            static std::string FormatArgs(const distribution_t &d) {
                return abel::string_cat(d.p());
            }

            static std::string FormatResults(abel::span<const result_t> results) {
                return abel::string_join(results, ", ");
            }
        };

        template<typename R>
        struct distribution_format_traits<abel::beta_distribution<R>> {
            using distribution_t = abel::beta_distribution<R>;
            using result_t = typename distribution_t::result_type;

            static constexpr const char *Name() { return "Beta"; }

            static std::string FunctionName() {
                return abel::string_cat(Name(), "<", scalar_type_name<R>(), ">");
            }

            static std::string FormatArgs(const distribution_t &d) {
                return abel::string_cat(d.alpha(), ", ", d.beta());
            }

            static std::string FormatResults(abel::span<const result_t> results) {
                return abel::string_join(results, ", ");
            }
        };

        template<typename R>
        struct distribution_format_traits<abel::zipf_distribution<R>> {
            using distribution_t = abel::zipf_distribution<R>;
            using result_t = typename distribution_t::result_type;

            static constexpr const char *Name() { return "Zipf"; }

            static std::string FunctionName() {
                return abel::string_cat(Name(), "<", scalar_type_name<R>(), ">");
            }

            static std::string FormatArgs(const distribution_t &d) {
                return abel::string_cat(d.k(), ", ", d.v(), ", ", d.q());
            }

            static std::string FormatResults(abel::span<const result_t> results) {
                return abel::string_join(results, ", ");
            }
        };

        template<typename R>
        struct distribution_format_traits<abel::gaussian_distribution<R>> {
            using distribution_t = abel::gaussian_distribution<R>;
            using result_t = typename distribution_t::result_type;

            static constexpr const char *Name() { return "Gaussian"; }

            static std::string FunctionName() {
                return abel::string_cat(Name(), "<", scalar_type_name<R>(), ">");
            }

            static std::string FormatArgs(const distribution_t &d) {
                return abel::string_join(std::make_tuple(d.mean(), d.stddev()), ", ");
            }

            static std::string FormatResults(abel::span<const result_t> results) {
                return abel::string_join(results, ", ");
            }
        };

        template<typename R>
        struct distribution_format_traits<abel::log_uniform_int_distribution<R>> {
            using distribution_t = abel::log_uniform_int_distribution<R>;
            using result_t = typename distribution_t::result_type;

            static constexpr const char *Name() { return "LogUniform"; }

            static std::string FunctionName() {
                return abel::string_cat(Name(), "<", scalar_type_name<R>(), ">");
            }

            static std::string FormatArgs(const distribution_t &d) {
                return abel::string_join(std::make_tuple((d.min)(), (d.max)(), d.base()), ", ");
            }

            static std::string FormatResults(abel::span<const result_t> results) {
                return abel::string_join(results, ", ");
            }
        };

        template<typename NumType>
        struct uniform_distribution_wrapper;

        template<typename NumType>
        struct distribution_format_traits<uniform_distribution_wrapper<NumType>> {
            using distribution_t = uniform_distribution_wrapper<NumType>;
            using result_t = NumType;

            static constexpr const char *Name() { return "uniform"; }

            static std::string FunctionName() {
                return abel::string_cat(Name(), "<", scalar_type_name<NumType>(), ">");
            }

            static std::string FormatArgs(const distribution_t &d) {
                return abel::string_cat((d.min)(), ", ", (d.max)());
            }

            static std::string FormatResults(abel::span<const result_t> results) {
                return abel::string_join(results, ", ");
            }
        };

    }  // namespace random_internal

}  // namespace abel

#endif  // ABEL_RANDOM_DISTRIBUTION_FORMAT_TRAITS_H_
