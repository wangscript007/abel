//
// -----------------------------------------------------------------------------
// variant.h
// -----------------------------------------------------------------------------
//
// This header file defines an `abel::variant` type for holding a type-safe
// value of some prescribed set of types (noted as alternative types), and
// associated functions for managing variants.
//
// The `abel::variant` type is a form of type-safe union. An `abel::variant`
// should always hold a value of one of its alternative types (except in the
// "valueless by exception state" -- see below). A default-constructed
// `abel::variant` will hold the value of its first alternative type, provided
// it is default-constructible.
//
// In exceptional cases due to error, an `abel::variant` can hold no
// value (known as a "valueless by exception" state), though this is not the
// norm.
//
// As with `abel::optional`, an `abel::variant` -- when it holds a value --
// allocates a value of that type directly within the `variant` itself; it
// cannot hold a reference, array, or the type `void`; it can, however, hold a
// pointer to externally managed memory.
//
// `abel::variant` is a C++11 compatible version of the C++17 `std::variant`
// abstraction and is designed to be a drop-in replacement for code compliant
// with C++17.

#ifndef ABEL_ASL_VARIANT_H_
#define ABEL_ASL_VARIANT_H_

#include <abel/base/profile.h>
#include <abel/utility/utility.h>

#ifdef ABEL_USES_STD_VARIANT

#include <variant>  // IWYU pragma: export

namespace abel {

using std::bad_variant_access;
using std::get;
using std::get_if;
using std::holds_alternative;
using std::monostate;
using std::variant;
using std::variant_alternative;
using std::variant_alternative_t;
using std::variant_npos;
using std::variant_size;
using std::variant_size_v;
using std::visit;

}  // namespace abel

#else  // ABEL_USES_STD_VARIANT

#include <functional>
#include <new>
#include <type_traits>
#include <utility>
#include <abel/base/profile.h>
#include <abel/meta/type_traits.h>
#include <abel/utility/internal/variant.h>

namespace abel {


// -----------------------------------------------------------------------------
// abel::variant
// -----------------------------------------------------------------------------
//
// An `abel::variant` type is a form of type-safe union. An `abel::variant` --
// except in exceptional cases -- always holds a value of one of its alternative
// types.
//
// Example:
//
//   // Construct a variant that holds either an integer or a std::string and
//   // assign it to a std::string.
//   abel::variant<int, std::string> v = std::string("abc");
//
//   // A default-constructed variant will hold a value-initialized value of
//   // the first alternative type.
//   auto a = abel::variant<int, std::string>();   // Holds an int of value '0'.
//
//   // variants are assignable.
//
//   // copy assignment
//   auto v1 = abel::variant<int, std::string>("abc");
//   auto v2 = abel::variant<int, std::string>(10);
//   v2 = v1;  // copy assign
//
//   // move assignment
//   auto v1 = abel::variant<int, std::string>("abc");
//   v1 = abel::variant<int, std::string>(10);
//
//   // assignment through type conversion
//   a = 128;         // variant contains int
//   a = "128";       // variant contains std::string
//
// An `abel::variant` holding a value of one of its alternative types `T` holds
// an allocation of `T` directly within the variant itself. An `abel::variant`
// is not allowed to allocate additional storage, such as dynamic memory, to
// allocate the contained value. The contained value shall be allocated in a
// region of the variant storage suitably aligned for all alternative types.
    template<typename... Ts>
    class variant;

// swap()
//
// Swaps two `abel::variant` values. This function is equivalent to `v.swap(w)`
// where `v` and `w` are `abel::variant` types.
//
// Note that this function requires all alternative types to be both swappable
// and move-constructible, because any two variants may refer to either the same
// type (in which case, they will be swapped) or to two different types (in
// which case the values will need to be moved).
//
    template<
            typename... Ts,
            abel::enable_if_t<
                    abel::conjunction<std::is_move_constructible<Ts>...,
                            abel::is_swappable<Ts>...>::value,
                    int> = 0>
    void swap(variant<Ts...> &v, variant<Ts...> &w) noexcept(noexcept(v.swap(w))) {
        v.swap(w);
    }

// variant_size
//
// Returns the number of alternative types available for a given `abel::variant`
// type as a compile-time constant expression. As this is a class template, it
// is not generally useful for accessing the number of alternative types of
// any given `abel::variant` instance.
//
// Example:
//
//   auto a = abel::variant<int, std::string>;
//   constexpr int num_types =
//       abel::variant_size<abel::variant<int, std::string>>();
//
//   // You can also use the member constant `value`.
//   constexpr int num_types =
//       abel::variant_size<abel::variant<int, std::string>>::value;
//
//   // `abel::variant_size` is more valuable for use in generic code:
//   template <typename Variant>
//   constexpr bool IsVariantMultivalue() {
//       return abel::variant_size<Variant>() > 1;
//   }
//
// Note that the set of cv-qualified specializations of `variant_size` are
// provided to ensure that those specializations compile (especially when passed
// within template logic).
    template<class T>
    struct variant_size;

    template<class... Ts>
    struct variant_size<variant<Ts...>>
            : std::integral_constant<std::size_t, sizeof...(Ts)> {
    };

// Specialization of `variant_size` for const qualified variants.
    template<class T>
    struct variant_size<const T> : variant_size<T>::type {
    };

// Specialization of `variant_size` for volatile qualified variants.
    template<class T>
    struct variant_size<volatile T> : variant_size<T>::type {
    };

// Specialization of `variant_size` for const volatile qualified variants.
    template<class T>
    struct variant_size<const volatile T> : variant_size<T>::type {
    };

// variant_alternative
//
// Returns the alternative type for a given `abel::variant` at the passed
// index value as a compile-time constant expression. As this is a class
// template resulting in a type, it is not useful for access of the run-time
// value of any given `abel::variant` variable.
//
// Example:
//
//   // The type of the 0th alternative is "int".
//   using alternative_type_0
//     = abel::variant_alternative<0, abel::variant<int, std::string>>::type;
//
//   static_assert(std::is_same<alternative_type_0, int>::value, "");
//
//   // `abel::variant_alternative` is more valuable for use in generic code:
//   template <typename Variant>
//   constexpr bool IsFirstElementTrivial() {
//       return std::is_trivial_v<variant_alternative<0, Variant>::type>;
//   }
//
// Note that the set of cv-qualified specializations of `variant_alternative`
// are provided to ensure that those specializations compile (especially when
// passed within template logic).
    template<std::size_t I, class T>
    struct variant_alternative;

    template<std::size_t I, class... Types>
    struct variant_alternative<I, variant<Types...>> {
        using type =
        variant_internal::VariantAlternativeSfinaeT<I, variant<Types...>>;
    };

// Specialization of `variant_alternative` for const qualified variants.
    template<std::size_t I, class T>
    struct variant_alternative<I, const T> {
        using type = const typename variant_alternative<I, T>::type;
    };

// Specialization of `variant_alternative` for volatile qualified variants.
    template<std::size_t I, class T>
    struct variant_alternative<I, volatile T> {
        using type = volatile typename variant_alternative<I, T>::type;
    };

// Specialization of `variant_alternative` for const volatile qualified
// variants.
    template<std::size_t I, class T>
    struct variant_alternative<I, const volatile T> {
        using type = const volatile typename variant_alternative<I, T>::type;
    };

// Template type alias for variant_alternative<I, T>::type.
//
// Example:
//
//   using alternative_type_0
//     = abel::variant_alternative_t<0, abel::variant<int, std::string>>;
//   static_assert(std::is_same<alternative_type_0, int>::value, "");
    template<std::size_t I, class T>
    using variant_alternative_t = typename variant_alternative<I, T>::type;

// holds_alternative()
//
// Checks whether the given variant currently holds a given alternative type,
// returning `true` if so.
//
// Example:
//
//   abel::variant<int, std::string> foo = 42;
//   if (abel::holds_alternative<int>(foo)) {
//       std::cout << "The variant holds an integer";
//   }
    template<class T, class... Types>
    constexpr bool holds_alternative(const variant<Types...> &v) noexcept {
        static_assert(
                variant_internal::UnambiguousIndexOfImpl<variant<Types...>, T,
                        0>::value != sizeof...(Types),
                "The type T must occur exactly once in Types...");
        return v.index() ==
               variant_internal::UnambiguousIndexOf<variant<Types...>, T>::value;
    }

// get()
//
// Returns a reference to the value currently within a given variant, using
// either a unique alternative type amongst the variant's set of alternative
// types, or the variant's index value. Attempting to get a variant's value
// using a type that is not unique within the variant's set of alternative types
// is a compile-time error. If the index of the alternative being specified is
// different from the index of the alternative that is currently stored, throws
// `abel::bad_variant_access`.
//
// Example:
//
//   auto a = abel::variant<int, std::string>;
//
//   // Get the value by type (if unique).
//   int i = abel::get<int>(a);
//
//   auto b = abel::variant<int, int>;
//
//   // Getting the value by a type that is not unique is ill-formed.
//   int j = abel::get<int>(b);     // Compile Error!
//
//   // Getting value by index not ambiguous and allowed.
//   int k = abel::get<1>(b);

// Overload for getting a variant's lvalue by type.
    template<class T, class... Types>
    constexpr T &get(variant<Types...> &v) {  // NOLINT
        return variant_internal::VariantCoreAccess::CheckedAccess<
                variant_internal::IndexOf<T, Types...>::value>(v);
    }

// Overload for getting a variant's rvalue by type.
// Note: `abel::move()` is required to allow use of constexpr in C++11.
    template<class T, class... Types>
    constexpr T &&get(variant<Types...> &&v) {
        return variant_internal::VariantCoreAccess::CheckedAccess<
                variant_internal::IndexOf<T, Types...>::value>(abel::move(v));
    }

// Overload for getting a variant's const lvalue by type.
    template<class T, class... Types>
    constexpr const T &get(const variant<Types...> &v) {
        return variant_internal::VariantCoreAccess::CheckedAccess<
                variant_internal::IndexOf<T, Types...>::value>(v);
    }

// Overload for getting a variant's const rvalue by type.
// Note: `abel::move()` is required to allow use of constexpr in C++11.
    template<class T, class... Types>
    constexpr const T &&get(const variant<Types...> &&v) {
        return variant_internal::VariantCoreAccess::CheckedAccess<
                variant_internal::IndexOf<T, Types...>::value>(abel::move(v));
    }

// Overload for getting a variant's lvalue by index.
    template<std::size_t I, class... Types>
    constexpr variant_alternative_t<I, variant<Types...>> &get(
            variant<Types...> &v) {  // NOLINT
        return variant_internal::VariantCoreAccess::CheckedAccess<I>(v);
    }

// Overload for getting a variant's rvalue by index.
// Note: `abel::move()` is required to allow use of constexpr in C++11.
    template<std::size_t I, class... Types>
    constexpr variant_alternative_t<I, variant<Types...>> &&get(
            variant<Types...> &&v) {
        return variant_internal::VariantCoreAccess::CheckedAccess<I>(abel::move(v));
    }

// Overload for getting a variant's const lvalue by index.
    template<std::size_t I, class... Types>
    constexpr const variant_alternative_t<I, variant<Types...>> &get(
            const variant<Types...> &v) {
        return variant_internal::VariantCoreAccess::CheckedAccess<I>(v);
    }

// Overload for getting a variant's const rvalue by index.
// Note: `abel::move()` is required to allow use of constexpr in C++11.
    template<std::size_t I, class... Types>
    constexpr const variant_alternative_t<I, variant<Types...>> &&get(
            const variant<Types...> &&v) {
        return variant_internal::VariantCoreAccess::CheckedAccess<I>(abel::move(v));
    }

// get_if()
//
// Returns a pointer to the value currently stored within a given variant, if
// present, using either a unique alternative type amongst the variant's set of
// alternative types, or the variant's index value. If such a value does not
// exist, returns `nullptr`.
//
// As with `get`, attempting to get a variant's value using a type that is not
// unique within the variant's set of alternative types is a compile-time error.

// Overload for getting a pointer to the value stored in the given variant by
// index.
    template<std::size_t I, class... Types>
    constexpr abel::add_pointer_t<variant_alternative_t<I, variant<Types...>>>
    get_if(variant<Types...> *v) noexcept {
        return (v != nullptr && v->index() == I)
               ? std::addressof(
                        variant_internal::VariantCoreAccess::Access<I>(*v))
               : nullptr;
    }

// Overload for getting a pointer to the const value stored in the given
// variant by index.
    template<std::size_t I, class... Types>
    constexpr abel::add_pointer_t<const variant_alternative_t<I, variant<Types...>>>
    get_if(const variant<Types...> *v) noexcept {
        return (v != nullptr && v->index() == I)
               ? std::addressof(
                        variant_internal::VariantCoreAccess::Access<I>(*v))
               : nullptr;
    }

// Overload for getting a pointer to the value stored in the given variant by
// type.
    template<class T, class... Types>
    constexpr abel::add_pointer_t<T> get_if(variant<Types...> *v) noexcept {
        return abel::get_if<variant_internal::IndexOf<T, Types...>::value>(v);
    }

// Overload for getting a pointer to the const value stored in the given variant
// by type.
    template<class T, class... Types>
    constexpr abel::add_pointer_t<const T> get_if(
            const variant<Types...> *v) noexcept {
        return abel::get_if<variant_internal::IndexOf<T, Types...>::value>(v);
    }

// visit()
//
// Calls a provided functor on a given set of variants. `abel::visit()` is
// commonly used to conditionally inspect the state of a given variant (or set
// of variants).
//
// The functor must return the same type when called with any of the variants'
// alternatives.
//
// Example:
//
//   // Define a visitor functor
//   struct GetVariant {
//       template<typename T>
//       void operator()(const T& i) const {
//         std::cout << "The variant's value is: " << i;
//       }
//   };
//
//   // Declare our variant, and call `abel::visit()` on it.
//   // Note that `GetVariant()` returns void in either case.
//   abel::variant<int, std::string> foo = std::string("foo");
//   GetVariant visitor;
//   abel::visit(visitor, foo);  // Prints `The variant's value is: foo'
    template<typename Visitor, typename... Variants>
    variant_internal::VisitResult<Visitor, Variants...> visit(Visitor &&vis,
                                                              Variants &&... vars) {
        return variant_internal::
        VisitIndices<variant_size<abel::decay_t<Variants> >::value...>::Run(
                variant_internal::PerformVisitation<Visitor, Variants...>{
                        std::forward_as_tuple(abel::forward<Variants>(vars)...),
                        abel::forward<Visitor>(vis)},
                vars.index()...);
    }

// monostate
//
// The monostate class serves as a first alternative type for a variant for
// which the first variant type is otherwise not default-constructible.
    struct monostate {
    };

// `abel::monostate` Relational Operators

    constexpr bool operator<(monostate, monostate) noexcept { return false; }

    constexpr bool operator>(monostate, monostate) noexcept { return false; }

    constexpr bool operator<=(monostate, monostate) noexcept { return true; }

    constexpr bool operator>=(monostate, monostate) noexcept { return true; }

    constexpr bool operator==(monostate, monostate) noexcept { return true; }

    constexpr bool operator!=(monostate, monostate) noexcept { return false; }


//------------------------------------------------------------------------------
// `abel::variant` Template Definition
//------------------------------------------------------------------------------
    template<typename T0, typename... Tn>
    class variant<T0, Tn...> : private variant_internal::VariantBase<T0, Tn...> {
        static_assert(abel::conjunction<std::is_object<T0>,
                              std::is_object<Tn>...>::value,
                      "Attempted to instantiate a variant containing a non-object "
                      "type.");
        // Intentionally not qualifying `negation` with `abel::` to work around a bug
        // in MSVC 2015 with inline namespace and variadic template.
        static_assert(abel::conjunction<negation<std::is_array<T0> >,
                              negation<std::is_array<Tn> >...>::value,
                      "Attempted to instantiate a variant containing an array type.");
        static_assert(abel::conjunction<std::is_nothrow_destructible<T0>,
                              std::is_nothrow_destructible<Tn>...>::value,
                      "Attempted to instantiate a variant containing a non-nothrow "
                      "destructible type.");

        friend struct variant_internal::VariantCoreAccess;

    private:
        using Base = variant_internal::VariantBase<T0, Tn...>;

    public:
        // Constructors

        // Constructs a variant holding a default-initialized value of the first
        // alternative type.
        constexpr variant() /*noexcept(see 111above)*/ = default;

        // Copy constructor, standard semantics
        variant(const variant &other) = default;

        // Move constructor, standard semantics
        variant(variant &&other) /*noexcept(see above)*/ = default;

        // Constructs a variant of an alternative type specified by overload
        // resolution of the provided forwarding arguments through
        // direct-initialization.
        //
        // Note: If the selected constructor is a constexpr constructor, this
        // constructor shall be a constexpr constructor.
        //
        // NOTE: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0608r1.html
        // has been voted passed the design phase in the C++ standard meeting in Mar
        // 2018. It will be implemented and integrated into `abel::variant`.
        template<
                class T,
                std::size_t I = std::enable_if<
                        variant_internal::IsNeitherSelfNorInPlace<variant,
                                abel::decay_t<T>>::value,
                        variant_internal::IndexOfConstructedType<variant, T>>::type::value,
                class Tj = abel::variant_alternative_t<I, variant>,
                abel::enable_if_t<std::is_constructible<Tj, T>::value> * =
                nullptr>
        constexpr variant(T &&t) noexcept(std::is_nothrow_constructible<Tj, T>::value)
                : Base(variant_internal::EmplaceTag<I>(), abel::forward<T>(t)) {}

        // Constructs a variant of an alternative type from the arguments through
        // direct-initialization.
        //
        // Note: If the selected constructor is a constexpr constructor, this
        // constructor shall be a constexpr constructor.
        template<class T, class... Args,
                typename std::enable_if<std::is_constructible<
                        variant_internal::UnambiguousTypeOfT<variant, T>,
                        Args...>::value>::type * = nullptr>
        constexpr explicit variant(in_place_type_t<T>, Args &&... args)
                : Base(variant_internal::EmplaceTag<
                               variant_internal::UnambiguousIndexOf<variant, T>::value>(),
                       abel::forward<Args>(args)...) {}

        // Constructs a variant of an alternative type from an initializer list
        // and other arguments through direct-initialization.
        //
        // Note: If the selected constructor is a constexpr constructor, this
        // constructor shall be a constexpr constructor.
        template<class T, class U, class... Args,
                typename std::enable_if<std::is_constructible<
                        variant_internal::UnambiguousTypeOfT<variant, T>,
                        std::initializer_list<U> &, Args...>::value>::type * = nullptr>
        constexpr explicit variant(in_place_type_t<T>, std::initializer_list<U> il,
                                   Args &&... args)
                : Base(variant_internal::EmplaceTag<
                               variant_internal::UnambiguousIndexOf<variant, T>::value>(),
                       il, abel::forward<Args>(args)...) {}

        // Constructs a variant of an alternative type from a provided index,
        // through value-initialization using the provided forwarded arguments.
        template<std::size_t I, class... Args,
                typename std::enable_if<std::is_constructible<
                        variant_internal::VariantAlternativeSfinaeT<I, variant>,
                        Args...>::value>::type * = nullptr>
        constexpr explicit variant(in_place_index_t<I>, Args &&... args)
                : Base(variant_internal::EmplaceTag<I>(), abel::forward<Args>(args)...) {}

        // Constructs a variant of an alternative type from a provided index,
        // through value-initialization of an initializer list and the provided
        // forwarded arguments.
        template<std::size_t I, class U, class... Args,
                typename std::enable_if<std::is_constructible<
                        variant_internal::VariantAlternativeSfinaeT<I, variant>,
                        std::initializer_list<U> &, Args...>::value>::type * = nullptr>
        constexpr explicit variant(in_place_index_t<I>, std::initializer_list<U> il,
                                   Args &&... args)
                : Base(variant_internal::EmplaceTag<I>(), il,
                       abel::forward<Args>(args)...) {}

        // Destructors

        // Destroys the variant's currently contained value, provided that
        // `abel::valueless_by_exception()` is false.
        ~variant() = default;

        // Assignment Operators

        // Copy assignment operator
        variant &operator=(const variant &other) = default;

        // Move assignment operator
        variant &operator=(variant &&other) /*noexcept(see above)*/ = default;

        // Converting assignment operator
        //
        // NOTE: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0608r1.html
        // has been voted passed the design phase in the C++ standard meeting in Mar
        // 2018. It will be implemented and integrated into `abel::variant`.
        template<
                class T,
                std::size_t I = std::enable_if<
                        !std::is_same<abel::decay_t<T>, variant>::value,
                        variant_internal::IndexOfConstructedType<variant, T>>::type::value,
                class Tj = abel::variant_alternative_t<I, variant>,
                typename std::enable_if<std::is_assignable<Tj &, T>::value &&
                                        std::is_constructible<Tj, T>::value>::type * =
                nullptr>
        variant &operator=(T &&t) noexcept(
        std::is_nothrow_assignable<Tj &, T>::value &&
        std::is_nothrow_constructible<Tj, T>::value) {
            variant_internal::VisitIndices<sizeof...(Tn) + 1>::Run(
                    variant_internal::VariantCoreAccess::MakeConversionAssignVisitor(
                            this, abel::forward<T>(t)),
                    index());

            return *this;
        }


        // emplace() Functions

        // Constructs a value of the given alternative type T within the variant.
        //
        // Example:
        //
        //   abel::variant<std::vector<int>, int, std::string> v;
        //   v.emplace<int>(99);
        //   v.emplace<std::string>("abc");
        template<
                class T, class... Args,
                typename std::enable_if<std::is_constructible<
                        abel::variant_alternative_t<
                                variant_internal::UnambiguousIndexOf<variant, T>::value, variant>,
                        Args...>::value>::type * = nullptr>
        T &emplace(Args &&... args) {
            return variant_internal::VariantCoreAccess::Replace<
                    variant_internal::UnambiguousIndexOf<variant, T>::value>(
                    this, abel::forward<Args>(args)...);
        }

        // Constructs a value of the given alternative type T within the variant using
        // an initializer list.
        //
        // Example:
        //
        //   abel::variant<std::vector<int>, int, std::string> v;
        //   v.emplace<std::vector<int>>({0, 1, 2});
        template<
                class T, class U, class... Args,
                typename std::enable_if<std::is_constructible<
                        abel::variant_alternative_t<
                                variant_internal::UnambiguousIndexOf<variant, T>::value, variant>,
                        std::initializer_list<U> &, Args...>::value>::type * = nullptr>
        T &emplace(std::initializer_list<U> il, Args &&... args) {
            return variant_internal::VariantCoreAccess::Replace<
                    variant_internal::UnambiguousIndexOf<variant, T>::value>(
                    this, il, abel::forward<Args>(args)...);
        }

        // Destroys the current value of the variant (provided that
        // `abel::valueless_by_exception()` is false, and constructs a new value at
        // the given index.
        //
        // Example:
        //
        //   abel::variant<std::vector<int>, int, int> v;
        //   v.emplace<1>(99);
        //   v.emplace<2>(98);
        //   v.emplace<int>(99);  // Won't compile. 'int' isn't a unique type.
        template<std::size_t I, class... Args,
                typename std::enable_if<
                        std::is_constructible<abel::variant_alternative_t<I, variant>,
                                Args...>::value>::type * = nullptr>
        abel::variant_alternative_t<I, variant> &emplace(Args &&... args) {
            return variant_internal::VariantCoreAccess::Replace<I>(
                    this, abel::forward<Args>(args)...);
        }

        // Destroys the current value of the variant (provided that
        // `abel::valueless_by_exception()` is false, and constructs a new value at
        // the given index using an initializer list and the provided arguments.
        //
        // Example:
        //
        //   abel::variant<std::vector<int>, int, int> v;
        //   v.emplace<0>({0, 1, 2});
        template<std::size_t I, class U, class... Args,
                typename std::enable_if<std::is_constructible<
                        abel::variant_alternative_t<I, variant>,
                        std::initializer_list<U> &, Args...>::value>::type * = nullptr>
        abel::variant_alternative_t<I, variant> &emplace(std::initializer_list<U> il,
                                                         Args &&... args) {
            return variant_internal::VariantCoreAccess::Replace<I>(
                    this, il, abel::forward<Args>(args)...);
        }

        // variant::valueless_by_exception()
        //
        // Returns false if and only if the variant currently holds a valid value.
        constexpr bool valueless_by_exception() const noexcept {
            return this->index_ == abel::variant_npos;
        }

        // variant::index()
        //
        // Returns the index value of the variant's currently selected alternative
        // type.
        constexpr std::size_t index() const noexcept { return this->index_; }

        // variant::swap()
        //
        // Swaps the values of two variant objects.
        //
        void swap(variant &rhs) noexcept(
        abel::conjunction<
                std::is_nothrow_move_constructible<T0>,
                std::is_nothrow_move_constructible<Tn>...,
                abel::is_nothrow_swappable<T0>,
                abel::is_nothrow_swappable<Tn>...>::value) {
            return variant_internal::VisitIndices<sizeof...(Tn) + 1>::Run(
                    variant_internal::Swap<T0, Tn...>{this, &rhs}, rhs.index());
        }
    };

// We need a valid declaration of variant<> for SFINAE and overload resolution
// to work properly above, but we don't need a full declaration since this type
// will never be constructed. This declaration, though incomplete, suffices.
    template<>
    class variant<>;

    //------------------------------------------------------------------------------
    // Relational Operators
    //------------------------------------------------------------------------------
    //
    // If neither operand is in the `variant::valueless_by_exception` state:
    //
    //   * If the index of both variants is the same, the relational operator
    //     returns the result of the corresponding relational operator for the
    //     corresponding alternative type.
    //   * If the index of both variants is not the same, the relational operator
    //     returns the result of that operation applied to the value of the left
    //     operand's index and the value of the right operand's index.
    //   * If at least one operand is in the valueless_by_exception state:
    //     - A variant in the valueless_by_exception state is only considered equal
    //       to another variant in the valueless_by_exception state.
    //     - If exactly one operand is in the valueless_by_exception state, the
    //       variant in the valueless_by_exception state is less than the variant
    //       that is not in the valueless_by_exception state.
    //
    // Note: The value 1 is added to each index in the relational comparisons such
    // that the index corresponding to the valueless_by_exception state wraps around
    // to 0 (the lowest value for the index type), and the remaining indices stay in
    // the same relative order.

    // Equal-to operator
    template<typename... Types>
    constexpr variant_internal::RequireAllHaveEqualT<Types...> operator==(
            const variant<Types...> &a, const variant<Types...> &b) {
        return (a.index() == b.index()) &&
               variant_internal::VisitIndices<sizeof...(Types)>::Run(
                       variant_internal::EqualsOp<Types...>{&a, &b}, a.index());
    }

// Not equal operator
    template<typename... Types>
    constexpr variant_internal::RequireAllHaveNotEqualT<Types...> operator!=(
            const variant<Types...> &a, const variant<Types...> &b) {
        return (a.index() != b.index()) ||
               variant_internal::VisitIndices<sizeof...(Types)>::Run(
                       variant_internal::NotEqualsOp<Types...>{&a, &b}, a.index());
    }

// Less-than operator
    template<typename... Types>
    constexpr variant_internal::RequireAllHaveLessThanT<Types...> operator<(
            const variant<Types...> &a, const variant<Types...> &b) {
        return (a.index() != b.index())
               ? (a.index() + 1) < (b.index() + 1)
               : variant_internal::VisitIndices<sizeof...(Types)>::Run(
                        variant_internal::LessThanOp<Types...>{&a, &b}, a.index());
    }

// Greater-than operator
    template<typename... Types>
    constexpr variant_internal::RequireAllHaveGreaterThanT<Types...> operator>(
            const variant<Types...> &a, const variant<Types...> &b) {
        return (a.index() != b.index())
               ? (a.index() + 1) > (b.index() + 1)
               : variant_internal::VisitIndices<sizeof...(Types)>::Run(
                        variant_internal::GreaterThanOp<Types...>{&a, &b},
                        a.index());
    }

// Less-than or equal-to operator
    template<typename... Types>
    constexpr variant_internal::RequireAllHaveLessThanOrEqualT<Types...> operator<=(
            const variant<Types...> &a, const variant<Types...> &b) {
        return (a.index() != b.index())
               ? (a.index() + 1) < (b.index() + 1)
               : variant_internal::VisitIndices<sizeof...(Types)>::Run(
                        variant_internal::LessThanOrEqualsOp<Types...>{&a, &b},
                        a.index());
    }

// Greater-than or equal-to operator
    template<typename... Types>
    constexpr variant_internal::RequireAllHaveGreaterThanOrEqualT<Types...>
    operator>=(const variant<Types...> &a, const variant<Types...> &b) {
        return (a.index() != b.index())
               ? (a.index() + 1) > (b.index() + 1)
               : variant_internal::VisitIndices<sizeof...(Types)>::Run(
                        variant_internal::GreaterThanOrEqualsOp<Types...>{&a, &b},
                        a.index());
    }


}  // namespace abel

namespace std {

// hash()
    template<>  // NOLINT
    struct hash<abel::monostate> {
        std::size_t operator()(abel::monostate) const { return 0; }
    };

    template<class... T>  // NOLINT
    struct hash<abel::variant<T...>>
            : abel::variant_internal::VariantHashBase<abel::variant<T...>, void,
                    abel::remove_const_t<T>...> {
    };

}  // namespace std

#endif  // ABEL_USES_STD_VARIANT

namespace abel {

    namespace variant_internal {

// Helper visitor for converting a variant<Ts...>` into another type (mostly
// variant) that can be constructed from any type.
        template<typename To>
        struct conversion_visitor {
            template<typename T>
            To operator()(T &&v) const {
                return To(std::forward<T>(v));
            }
        };

    }  // namespace variant_internal

    // convert_variant_to()
    //
    // Helper functions to convert an `abel::variant` to a variant of another set of
    // types, provided that the alternative type of the new variant type can be
    // converted from any type in the source variant.
    //
    // Example:
    //
    //   abel::variant<name1, name2, float> internal_req(const Req&);
    //
    //   // name1 and name2 are convertible to name
    //   abel::variant<name, float> external_req(const Req& req) {
    //     return abel::convert_variant_to<abel::variant<name, float>>(
    //              InternalReq(req));
    //   }
    template<typename To, typename Variant>
    To convert_variant_to(Variant &&variant) {
        return abel::visit(variant_internal::conversion_visitor<To>{},
                           std::forward<Variant>(variant));
    }


}  // namespace abel

#endif  // ABEL_ASL_VARIANT_H_
