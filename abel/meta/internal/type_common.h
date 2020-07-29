//
// Created by liyinbin on 2020/3/22.
//

#ifndef ABEL_ASL_INTERNAL_TYPE_COMMON_H_
#define ABEL_ASL_INTERNAL_TYPE_COMMON_H_

#include <abel/meta/internal/config.h>
#include <memory>

namespace abel {

    ///////////////////////////////////////////////////////////////////////
    // identity
    //
    // The purpose of this is typically to deal with non-deduced template
    // contexts. See the C++11 Standard, 14.8.2.5 p5.
    // Also: http://cppquiz.org/quiz/question/109?result=CE&answer=&did_answer=Answer
    //
    // Dinkumware has an identity, but adds a member function to it:
    //     const T& operator()(const T& t) const{ return t; }
    //
    // NOTE(rparolin): Use 'abel::type_identity' it was included in the C++20
    // standard. This is a legacy abel type we continue to support for
    // backwards compatibility.
    //

    template <typename T>
    struct identity { using type = T; };

    template <typename T>
    using identity_t = typename identity<T>::type;


    ///////////////////////////////////////////////////////////////////////
    // type_identity
    //
    // The purpose of this is typically to deal with non-deduced template
    // contexts. See the C++11 Standard, 14.8.2.5 p5.
    // Also: http://cppquiz.org/quiz/question/109?result=CE&answer=&did_answer=Answer
    //
    // https://en.cppreference.com/w/cpp/types/type_identity
    //

    template<typename T>
    struct type_identity {
        typedef T type;
    };

    template<typename T>
    using type_identity_t = typename type_identity<T>::type;

    namespace asl_internal {
        template<typename... Ts>
        struct void_t_impl {
            using type = void;
        };


        // This trick to retrieve a default alignment is necessary for our
        // implementation of aligned_storage_t to be consistent with any implementation
        // of std::aligned_storage.

        template<size_t Len, typename T = std::aligned_storage<Len>>
        struct default_alignment_of_aligned_storage;

        template<size_t Len, size_t Align>
        struct default_alignment_of_aligned_storage<Len,
                std::aligned_storage<Len, Align>> {
            static constexpr size_t value = Align;
        };
    }

    // -----------------------------------------------------------------------------
// C++14 "_t" trait aliases
// -----------------------------------------------------------------------------

    template<typename T>
    using remove_cv_t = typename std::remove_cv<T>::type;

    template<typename T>
    using remove_const_t = typename std::remove_const<T>::type;

    template<typename T>
    using remove_volatile_t = typename std::remove_volatile<T>::type;

    template<typename T>
    using add_cv_t = typename std::add_cv<T>::type;

    template<typename T>
    using add_const_t = typename std::add_const<T>::type;

    template<typename T>
    using add_volatile_t = typename std::add_volatile<T>::type;

    template<typename T>
    using remove_reference_t = typename std::remove_reference<T>::type;

    template<typename T>
    using add_lvalue_reference_t = typename std::add_lvalue_reference<T>::type;

    template<typename T>
    using add_rvalue_reference_t = typename std::add_rvalue_reference<T>::type;

    template<typename T>
    using remove_pointer_t = typename std::remove_pointer<T>::type;

    template<typename T>
    using add_pointer_t = typename std::add_pointer<T>::type;

    template<typename T>
    using make_signed_t = typename std::make_signed<T>::type;

    template<typename T>
    using make_unsigned_t = typename std::make_unsigned<T>::type;

    template<typename T>
    using remove_extent_t = typename std::remove_extent<T>::type;

    template<typename T>
    using remove_all_extents_t = typename std::remove_all_extents<T>::type;

    template<size_t Len, size_t Align = asl_internal::
    default_alignment_of_aligned_storage<Len>::value>
    using aligned_storage_t = typename std::aligned_storage<Len, Align>::type;

    template<typename T>
    using decay_t = typename std::decay<T>::type;

    template<bool B, typename T = void>
    using enable_if_t = typename std::enable_if<B, T>::type;

    template<bool B, typename T, typename F>
    using conditional_t = typename std::conditional<B, T, F>::type;

    template<typename... T>
    using common_type_t = typename std::common_type<T...>::type;

    template<typename T>
    using underlying_type_t = typename std::underlying_type<T>::type;

    template<typename T>
    using result_of_t = typename std::result_of<T>::type;

    template <typename T>
    struct add_cr_non_integral {
        typedef typename std::conditional<std::is_integral<T>::value, T,
                typename std::add_lvalue_reference<typename std::add_const<T>::type>::type>::type type;
    };

    template <typename T>
    using add_cr_non_integral_t = typename add_cr_non_integral<T>::type;

    template <typename T>
    using add_const_reference_t = typename std::add_lvalue_reference<typename std::add_const<T>::type>::type;

////////////////////////////////
// Library Fundamentals V2 TS //
////////////////////////////////

    // NOTE: The `is_detected` family of templates here differ from the library
    // fundamentals specification in that for library fundamentals, `Op<Args...>` is
    // evaluated as soon as the type `is_detected<Op, Args...>` undergoes
    // substitution, regardless of whether or not the `::value` is accessed. That
    // is inconsistent with all other standard traits and prevents lazy evaluation
    // in larger contexts (such as if the `is_detected` check is a trailing argument
    // of a `conjunction`. This implementation opts to instead be lazy in the same
    // way that the standard traits are (this "defect" of the detection idiom
    // specifications has been reported).

    template<class Enabler, template<class...> class Op, class... Args>
    struct is_detected_impl {
        using type = std::false_type;
    };

    template<template<class...> class Op, class... Args>
    struct is_detected_impl<typename asl_internal::void_t_impl<Op<Args...>>::type, Op, Args...> {
        using type = std::true_type;
    };

    template<template<class...> class Op, class... Args>
    struct is_detected : is_detected_impl<void, Op, Args...>::type {
    };

    template<class Enabler, class To, template<class...> class Op, class... Args>
    struct is_detected_convertible_impl {
        using type = std::false_type;
    };

    template<class To, template<class...> class Op, class... Args>
    struct is_detected_convertible_impl<
            typename std::enable_if<std::is_convertible<Op<Args...>, To>::value>::type,
            To, Op, Args...> {
        using type = std::true_type;
    };

    template<class To, template<class...> class Op, class... Args>
    struct is_detected_convertible
            : is_detected_convertible_impl<void, To, Op, Args...>::type {
    };


    // conjunction
    //
    // Performs a compile-time logical AND operation on the passed types (which
    // must have  `::value` members convertible to `bool`. Short-circuits if it
    // encounters any `false` members (and does not compare the `::value` members
    // of any remaining arguments).
    //
    // This metafunction is designed to be a drop-in replacement for the C++17
    // `std::conjunction` metafunction.
    template<typename... Ts>
    struct conjunction;

    template<typename T, typename... Ts>
    struct conjunction<T, Ts...>
            : std::conditional<T::value, conjunction<Ts...>, T>::type {
    };

    template<typename T>
    struct conjunction<T> : T {
    };

    template<>
    struct conjunction<> : std::true_type {
    };

    // disjunction
    //
    // Performs a compile-time logical OR operation on the passed types (which
    // must have  `::value` members convertible to `bool`. Short-circuits if it
    // encounters any `true` members (and does not compare the `::value` members
    // of any remaining arguments).
    //
    // This metafunction is designed to be a drop-in replacement for the C++17
    // `std::disjunction` metafunction.
    template<typename... Ts>
    struct disjunction;

    template<typename T, typename... Ts>
    struct disjunction<T, Ts...> :
            std::conditional<T::value, T, disjunction<Ts...>>::type {
    };

    template<typename T>
    struct disjunction<T> : T {
    };

    template<>
    struct disjunction<> : std::false_type {
    };

    // negation
    //
    // Performs a compile-time logical NOT operation on the passed type (which
    // must have  `::value` members convertible to `bool`.
    //
    // This metafunction is designed to be a drop-in replacement for the C++17
    // `std::negation` metafunction.
    template<typename T>
    struct negation : std::integral_constant<bool, !T::value> {
    };

    // is_function()
    //
    // Determines whether the passed type `T` is a function type.
    //
    // This metafunction is designed to be a drop-in replacement for the C++11
    // `std::is_function()` metafunction for platforms that have incomplete C++11
    // support (such as libstdc++ 4.x).
    //
    // This metafunction works because appending `const` to a type does nothing to
    // function types and reference types (and forms a const-qualified type
    // otherwise).
    template<typename T>
    struct is_function
            : std::integral_constant<
                    bool, !(std::is_reference<T>::value ||
                            std::is_const<typename std::add_const<T>::type>::value)> {
    };


} //namespace abel


#endif //ABEL_ASL_INTERNAL_TYPE_COMMON_H_
