//

#include <new>
#include <type_traits>
#include <utility>

#include <gtest/gtest.h>
#include <abel/meta/type_traits.h>
#include <abel/types/internal/conformance_aliases.h>

namespace {

namespace ti = abel::types_internal;

template <class T>
using DefaultConstructibleWithNewImpl = decltype(::new (std::nothrow) T);

template <class T>
using DefaultConstructibleWithNew =
    abel::type_traits_internal::is_detected<DefaultConstructibleWithNewImpl, T>;

template <class T>
using MoveConstructibleWithNewImpl =
    decltype(::new (std::nothrow) T(std::declval<T>()));

template <class T>
using MoveConstructibleWithNew =
    abel::type_traits_internal::is_detected<MoveConstructibleWithNewImpl, T>;

template <class T>
using CopyConstructibleWithNewImpl =
    decltype(::new (std::nothrow) T(std::declval<const T&>()));

template <class T>
using CopyConstructibleWithNew =
    abel::type_traits_internal::is_detected<CopyConstructibleWithNewImpl, T>;

template <class T,
          class Result =
              std::integral_constant<bool, noexcept(::new (std::nothrow) T)>>
using NothrowDefaultConstructibleWithNewImpl =
    typename std::enable_if<Result::value>::type;

template <class T>
using NothrowDefaultConstructibleWithNew =
    abel::type_traits_internal::is_detected<
        NothrowDefaultConstructibleWithNewImpl, T>;

template <class T,
          class Result = std::integral_constant<
              bool, noexcept(::new (std::nothrow) T(std::declval<T>()))>>
using NothrowMoveConstructibleWithNewImpl =
    typename std::enable_if<Result::value>::type;

template <class T>
using NothrowMoveConstructibleWithNew =
    abel::type_traits_internal::is_detected<NothrowMoveConstructibleWithNewImpl,
                                            T>;

template <class T,
          class Result = std::integral_constant<
              bool, noexcept(::new (std::nothrow) T(std::declval<const T&>()))>>
using NothrowCopyConstructibleWithNewImpl =
    typename std::enable_if<Result::value>::type;

template <class T>
using NothrowCopyConstructibleWithNew =
    abel::type_traits_internal::is_detected<NothrowCopyConstructibleWithNewImpl,
                                            T>;

// NOTE: ?: is used to verify contextually-convertible to bool and not simply
//       implicit or explicit convertibility.
#define ABEL_INTERNAL_COMPARISON_OP_EXPR(op) \
  ((std::declval<const T&>() op std::declval<const T&>()) ? true : true)

#define ABEL_INTERNAL_COMPARISON_OP_TRAIT(name, op)                         \
  template <class T>                                                        \
  using name##Impl = decltype(ABEL_INTERNAL_COMPARISON_OP_EXPR(op));        \
                                                                            \
  template <class T>                                                        \
  using name = abel::type_traits_internal::is_detected<name##Impl, T>;      \
                                                                            \
  template <class T,                                                        \
            class Result = std::integral_constant<                          \
                bool, noexcept(ABEL_INTERNAL_COMPARISON_OP_EXPR(op))>>      \
  using Nothrow##name##Impl = typename std::enable_if<Result::value>::type; \
                                                                            \
  template <class T>                                                        \
  using Nothrow##name =                                                     \
      abel::type_traits_internal::is_detected<Nothrow##name##Impl, T>

ABEL_INTERNAL_COMPARISON_OP_TRAIT(EqualityComparable, ==);
ABEL_INTERNAL_COMPARISON_OP_TRAIT(InequalityComparable, !=);
ABEL_INTERNAL_COMPARISON_OP_TRAIT(LessThanComparable, <);
ABEL_INTERNAL_COMPARISON_OP_TRAIT(LessEqualComparable, <=);
ABEL_INTERNAL_COMPARISON_OP_TRAIT(GreaterEqualComparable, >=);
ABEL_INTERNAL_COMPARISON_OP_TRAIT(GreaterThanComparable, >);

#undef ABEL_INTERNAL_COMPARISON_OP_TRAIT

template <class T>
class ProfileTest : public ::testing::Test {};

TYPED_TEST_SUITE_P(ProfileTest);

TYPED_TEST_P(ProfileTest, HasAppropriateConstructionProperties) {
  using profile = typename TypeParam::profile;
  using arch = typename TypeParam::arch;
  using expected_profile = typename TypeParam::expected_profile;

  using props = ti::PropertiesOfT<profile>;
  using arch_props = ti::PropertiesOfArchetypeT<arch>;
  using expected_props = ti::PropertiesOfT<expected_profile>;

  // Make sure all of the properties are as expected.
  // There are seemingly redundant tests here to make it easier to diagnose
  // the specifics of the failure if something were to go wrong.
  EXPECT_TRUE((std::is_same<props, arch_props>::value));
  EXPECT_TRUE((std::is_same<props, expected_props>::value));
  EXPECT_TRUE((std::is_same<arch_props, expected_props>::value));

  EXPECT_EQ(props::default_constructible_support,
            expected_props::default_constructible_support);

  EXPECT_EQ(props::move_constructible_support,
            expected_props::move_constructible_support);

  EXPECT_EQ(props::copy_constructible_support,
            expected_props::copy_constructible_support);

  EXPECT_EQ(props::destructible_support, expected_props::destructible_support);

  // Avoid additional error message noise when profile and archetype match with
  // each other but were not what was expected.
  if (!std::is_same<props, arch_props>::value) {
    EXPECT_EQ(arch_props::default_constructible_support,
              expected_props::default_constructible_support);

    EXPECT_EQ(arch_props::move_constructible_support,
              expected_props::move_constructible_support);

    EXPECT_EQ(arch_props::copy_constructible_support,
              expected_props::copy_constructible_support);

    EXPECT_EQ(arch_props::destructible_support,
              expected_props::destructible_support);
  }

  //////////////////////////////////////////////////////////////////////////////
  //                       Default constructor checks                         //
  //////////////////////////////////////////////////////////////////////////////
  EXPECT_EQ(props::default_constructible_support,
            expected_props::default_constructible_support);

  switch (expected_props::default_constructible_support) {
    case ti::default_constructible::maybe:
      EXPECT_FALSE(DefaultConstructibleWithNew<arch>::value);
      EXPECT_FALSE(NothrowDefaultConstructibleWithNew<arch>::value);

      // Standard constructible traits depend on the destructor.
      if (std::is_destructible<arch>::value) {
        EXPECT_FALSE(std::is_default_constructible<arch>::value);
        EXPECT_FALSE(std::is_nothrow_default_constructible<arch>::value);
        EXPECT_FALSE(abel::is_trivially_default_constructible<arch>::value);
      }
      break;
    case ti::default_constructible::yes:
      EXPECT_TRUE(DefaultConstructibleWithNew<arch>::value);
      EXPECT_FALSE(NothrowDefaultConstructibleWithNew<arch>::value);

      // Standard constructible traits depend on the destructor.
      if (std::is_destructible<arch>::value) {
        EXPECT_TRUE(std::is_default_constructible<arch>::value);
        EXPECT_FALSE(std::is_nothrow_default_constructible<arch>::value);
        EXPECT_FALSE(abel::is_trivially_default_constructible<arch>::value);
      }
      break;
    case ti::default_constructible::nothrow:
      EXPECT_TRUE(DefaultConstructibleWithNew<arch>::value);
      EXPECT_TRUE(NothrowDefaultConstructibleWithNew<arch>::value);

      // Standard constructible traits depend on the destructor.
      if (std::is_destructible<arch>::value) {
        EXPECT_TRUE(std::is_default_constructible<arch>::value);
        EXPECT_TRUE(std::is_nothrow_default_constructible<arch>::value);
        EXPECT_FALSE(abel::is_trivially_default_constructible<arch>::value);

        // Constructor traits also check the destructor.
        if (std::is_nothrow_destructible<arch>::value) {
          EXPECT_TRUE(std::is_nothrow_default_constructible<arch>::value);
        }
      }
      break;
    case ti::default_constructible::trivial:
      EXPECT_TRUE(DefaultConstructibleWithNew<arch>::value);
      EXPECT_TRUE(NothrowDefaultConstructibleWithNew<arch>::value);

      // Standard constructible traits depend on the destructor.
      if (std::is_destructible<arch>::value) {
        EXPECT_TRUE(std::is_default_constructible<arch>::value);
        EXPECT_TRUE(std::is_nothrow_default_constructible<arch>::value);

        // Constructor triviality traits require trivially destructible types.
        if (abel::is_trivially_destructible<arch>::value) {
          EXPECT_TRUE(abel::is_trivially_default_constructible<arch>::value);
        }
      }
      break;
  }

  //////////////////////////////////////////////////////////////////////////////
  //                         Move constructor checks                          //
  //////////////////////////////////////////////////////////////////////////////
  EXPECT_EQ(props::move_constructible_support,
            expected_props::move_constructible_support);

  switch (expected_props::move_constructible_support) {
    case ti::move_constructible::maybe:
      EXPECT_FALSE(MoveConstructibleWithNew<arch>::value);
      EXPECT_FALSE(NothrowMoveConstructibleWithNew<arch>::value);

      // Standard constructible traits depend on the destructor.
      if (std::is_destructible<arch>::value) {
        EXPECT_FALSE(std::is_move_constructible<arch>::value);
        EXPECT_FALSE(std::is_nothrow_move_constructible<arch>::value);
        EXPECT_FALSE(abel::is_trivially_move_constructible<arch>::value);
      }
      break;
    case ti::move_constructible::yes:
      EXPECT_TRUE(MoveConstructibleWithNew<arch>::value);
      EXPECT_FALSE(NothrowMoveConstructibleWithNew<arch>::value);

      // Standard constructible traits depend on the destructor.
      if (std::is_destructible<arch>::value) {
        EXPECT_TRUE(std::is_move_constructible<arch>::value);
        EXPECT_FALSE(std::is_nothrow_move_constructible<arch>::value);
        EXPECT_FALSE(abel::is_trivially_move_constructible<arch>::value);
      }
      break;
    case ti::move_constructible::nothrow:
      EXPECT_TRUE(MoveConstructibleWithNew<arch>::value);
      EXPECT_TRUE(NothrowMoveConstructibleWithNew<arch>::value);

      // Standard constructible traits depend on the destructor.
      if (std::is_destructible<arch>::value) {
        EXPECT_TRUE(std::is_move_constructible<arch>::value);
        EXPECT_TRUE(std::is_nothrow_move_constructible<arch>::value);
        EXPECT_FALSE(abel::is_trivially_move_constructible<arch>::value);

        // Constructor traits also check the destructor.
        if (std::is_nothrow_destructible<arch>::value) {
          EXPECT_TRUE(std::is_nothrow_move_constructible<arch>::value);
        }
      }
      break;
    case ti::move_constructible::trivial:
      EXPECT_TRUE(MoveConstructibleWithNew<arch>::value);
      EXPECT_TRUE(NothrowMoveConstructibleWithNew<arch>::value);

      // Standard constructible traits depend on the destructor.
      if (std::is_destructible<arch>::value) {
        EXPECT_TRUE(std::is_move_constructible<arch>::value);
        EXPECT_TRUE(std::is_nothrow_move_constructible<arch>::value);

        // Constructor triviality traits require trivially destructible types.
        if (abel::is_trivially_destructible<arch>::value) {
          EXPECT_TRUE(abel::is_trivially_move_constructible<arch>::value);
        }
      }
      break;
  }

  //////////////////////////////////////////////////////////////////////////////
  //                         Copy constructor checks                          //
  //////////////////////////////////////////////////////////////////////////////
  EXPECT_EQ(props::copy_constructible_support,
            expected_props::copy_constructible_support);

  switch (expected_props::copy_constructible_support) {
    case ti::copy_constructible::maybe:
      EXPECT_FALSE(CopyConstructibleWithNew<arch>::value);
      EXPECT_FALSE(NothrowCopyConstructibleWithNew<arch>::value);

      // Standard constructible traits depend on the destructor.
      if (std::is_destructible<arch>::value) {
        EXPECT_FALSE(std::is_copy_constructible<arch>::value);
        EXPECT_FALSE(std::is_nothrow_copy_constructible<arch>::value);
        EXPECT_FALSE(abel::is_trivially_copy_constructible<arch>::value);
      }
      break;
    case ti::copy_constructible::yes:
      EXPECT_TRUE(CopyConstructibleWithNew<arch>::value);
      EXPECT_FALSE(NothrowCopyConstructibleWithNew<arch>::value);

      // Standard constructible traits depend on the destructor.
      if (std::is_destructible<arch>::value) {
        EXPECT_TRUE(std::is_copy_constructible<arch>::value);
        EXPECT_FALSE(std::is_nothrow_copy_constructible<arch>::value);
        EXPECT_FALSE(abel::is_trivially_copy_constructible<arch>::value);
      }
      break;
    case ti::copy_constructible::nothrow:
      EXPECT_TRUE(CopyConstructibleWithNew<arch>::value);
      EXPECT_TRUE(NothrowCopyConstructibleWithNew<arch>::value);

      // Standard constructible traits depend on the destructor.
      if (std::is_destructible<arch>::value) {
        EXPECT_TRUE(std::is_copy_constructible<arch>::value);
        EXPECT_TRUE(std::is_nothrow_copy_constructible<arch>::value);
        EXPECT_FALSE(abel::is_trivially_copy_constructible<arch>::value);

        // Constructor traits also check the destructor.
        if (std::is_nothrow_destructible<arch>::value) {
          EXPECT_TRUE(std::is_nothrow_copy_constructible<arch>::value);
        }
      }
      break;
    case ti::copy_constructible::trivial:
      EXPECT_TRUE(CopyConstructibleWithNew<arch>::value);
      EXPECT_TRUE(NothrowCopyConstructibleWithNew<arch>::value);

      // Standard constructible traits depend on the destructor.
      if (std::is_destructible<arch>::value) {
        EXPECT_TRUE(std::is_copy_constructible<arch>::value);
        EXPECT_TRUE(std::is_nothrow_copy_constructible<arch>::value);

        // Constructor triviality traits require trivially destructible types.
        if (abel::is_trivially_destructible<arch>::value) {
          EXPECT_TRUE(abel::is_trivially_copy_constructible<arch>::value);
        }
      }
      break;
  }

  //////////////////////////////////////////////////////////////////////////////
  //                           Destructible checks                            //
  //////////////////////////////////////////////////////////////////////////////
  EXPECT_EQ(props::destructible_support, expected_props::destructible_support);

  switch (expected_props::destructible_support) {
    case ti::destructible::maybe:
      EXPECT_FALSE(std::is_destructible<arch>::value);
      EXPECT_FALSE(std::is_nothrow_destructible<arch>::value);
      EXPECT_FALSE(abel::is_trivially_destructible<arch>::value);
      break;
    case ti::destructible::yes:
      EXPECT_TRUE(std::is_destructible<arch>::value);
      EXPECT_FALSE(std::is_nothrow_destructible<arch>::value);
      EXPECT_FALSE(abel::is_trivially_destructible<arch>::value);
      break;
    case ti::destructible::nothrow:
      EXPECT_TRUE(std::is_destructible<arch>::value);
      EXPECT_TRUE(std::is_nothrow_destructible<arch>::value);
      EXPECT_FALSE(abel::is_trivially_destructible<arch>::value);
      break;
    case ti::destructible::trivial:
      EXPECT_TRUE(std::is_destructible<arch>::value);
      EXPECT_TRUE(std::is_nothrow_destructible<arch>::value);
      EXPECT_TRUE(abel::is_trivially_destructible<arch>::value);
      break;
  }
}

TYPED_TEST_P(ProfileTest, HasAppropriateAssignmentProperties) {
  using profile = typename TypeParam::profile;
  using arch = typename TypeParam::arch;
  using expected_profile = typename TypeParam::expected_profile;

  using props = ti::PropertiesOfT<profile>;
  using arch_props = ti::PropertiesOfArchetypeT<arch>;
  using expected_props = ti::PropertiesOfT<expected_profile>;

  // Make sure all of the properties are as expected.
  // There are seemingly redundant tests here to make it easier to diagnose
  // the specifics of the failure if something were to go wrong.
  EXPECT_TRUE((std::is_same<props, arch_props>::value));
  EXPECT_TRUE((std::is_same<props, expected_props>::value));
  EXPECT_TRUE((std::is_same<arch_props, expected_props>::value));

  EXPECT_EQ(props::move_assignable_support,
            expected_props::move_assignable_support);

  EXPECT_EQ(props::copy_assignable_support,
            expected_props::copy_assignable_support);

  // Avoid additional error message noise when profile and archetype match with
  // each other but were not what was expected.
  if (!std::is_same<props, arch_props>::value) {
    EXPECT_EQ(arch_props::move_assignable_support,
              expected_props::move_assignable_support);

    EXPECT_EQ(arch_props::copy_assignable_support,
              expected_props::copy_assignable_support);
  }

  //////////////////////////////////////////////////////////////////////////////
  //                          Move assignment checks                          //
  //////////////////////////////////////////////////////////////////////////////
  EXPECT_EQ(props::move_assignable_support,
            expected_props::move_assignable_support);

  switch (expected_props::move_assignable_support) {
    case ti::move_assignable::maybe:
      EXPECT_FALSE(std::is_move_assignable<arch>::value);
      EXPECT_FALSE(std::is_nothrow_move_assignable<arch>::value);
      EXPECT_FALSE(abel::is_trivially_move_assignable<arch>::value);
      break;
    case ti::move_assignable::yes:
      EXPECT_TRUE(std::is_move_assignable<arch>::value);
      EXPECT_FALSE(std::is_nothrow_move_assignable<arch>::value);
      EXPECT_FALSE(abel::is_trivially_move_assignable<arch>::value);
      break;
    case ti::move_assignable::nothrow:
      EXPECT_TRUE(std::is_move_assignable<arch>::value);
      EXPECT_TRUE(std::is_nothrow_move_assignable<arch>::value);
      EXPECT_FALSE(abel::is_trivially_move_assignable<arch>::value);
      break;
    case ti::move_assignable::trivial:
      EXPECT_TRUE(std::is_move_assignable<arch>::value);
      EXPECT_TRUE(std::is_nothrow_move_assignable<arch>::value);
      EXPECT_TRUE(abel::is_trivially_move_assignable<arch>::value);
      break;
  }

  //////////////////////////////////////////////////////////////////////////////
  //                          Copy assignment checks                          //
  //////////////////////////////////////////////////////////////////////////////
  EXPECT_EQ(props::copy_assignable_support,
            expected_props::copy_assignable_support);

  switch (expected_props::copy_assignable_support) {
    case ti::copy_assignable::maybe:
      EXPECT_FALSE(std::is_copy_assignable<arch>::value);
      EXPECT_FALSE(std::is_nothrow_copy_assignable<arch>::value);
      EXPECT_FALSE(abel::is_trivially_copy_assignable<arch>::value);
      break;
    case ti::copy_assignable::yes:
      EXPECT_TRUE(std::is_copy_assignable<arch>::value);
      EXPECT_FALSE(std::is_nothrow_copy_assignable<arch>::value);
      EXPECT_FALSE(abel::is_trivially_copy_assignable<arch>::value);
      break;
    case ti::copy_assignable::nothrow:
      EXPECT_TRUE(std::is_copy_assignable<arch>::value);
      EXPECT_TRUE(std::is_nothrow_copy_assignable<arch>::value);
      EXPECT_FALSE(abel::is_trivially_copy_assignable<arch>::value);
      break;
    case ti::copy_assignable::trivial:
      EXPECT_TRUE(std::is_copy_assignable<arch>::value);
      EXPECT_TRUE(std::is_nothrow_copy_assignable<arch>::value);
      EXPECT_TRUE(abel::is_trivially_copy_assignable<arch>::value);
      break;
  }
}

TYPED_TEST_P(ProfileTest, HasAppropriateComparisonProperties) {
  using profile = typename TypeParam::profile;
  using arch = typename TypeParam::arch;
  using expected_profile = typename TypeParam::expected_profile;

  using props = ti::PropertiesOfT<profile>;
  using arch_props = ti::PropertiesOfArchetypeT<arch>;
  using expected_props = ti::PropertiesOfT<expected_profile>;

  // Make sure all of the properties are as expected.
  // There are seemingly redundant tests here to make it easier to diagnose
  // the specifics of the failure if something were to go wrong.
  EXPECT_TRUE((std::is_same<props, arch_props>::value));
  EXPECT_TRUE((std::is_same<props, expected_props>::value));
  EXPECT_TRUE((std::is_same<arch_props, expected_props>::value));

  EXPECT_EQ(props::equality_comparable_support,
            expected_props::equality_comparable_support);

  EXPECT_EQ(props::inequality_comparable_support,
            expected_props::inequality_comparable_support);

  EXPECT_EQ(props::less_than_comparable_support,
            expected_props::less_than_comparable_support);

  EXPECT_EQ(props::less_equal_comparable_support,
            expected_props::less_equal_comparable_support);

  EXPECT_EQ(props::greater_equal_comparable_support,
            expected_props::greater_equal_comparable_support);

  EXPECT_EQ(props::greater_than_comparable_support,
            expected_props::greater_than_comparable_support);

  // Avoid additional error message noise when profile and archetype match with
  // each other but were not what was expected.
  if (!std::is_same<props, arch_props>::value) {
    EXPECT_EQ(arch_props::equality_comparable_support,
              expected_props::equality_comparable_support);

    EXPECT_EQ(arch_props::inequality_comparable_support,
              expected_props::inequality_comparable_support);

    EXPECT_EQ(arch_props::less_than_comparable_support,
              expected_props::less_than_comparable_support);

    EXPECT_EQ(arch_props::less_equal_comparable_support,
              expected_props::less_equal_comparable_support);

    EXPECT_EQ(arch_props::greater_equal_comparable_support,
              expected_props::greater_equal_comparable_support);

    EXPECT_EQ(arch_props::greater_than_comparable_support,
              expected_props::greater_than_comparable_support);
  }

  //////////////////////////////////////////////////////////////////////////////
  //                        Equality comparable checks                        //
  //////////////////////////////////////////////////////////////////////////////
  switch (expected_props::equality_comparable_support) {
    case ti::equality_comparable::maybe:
      EXPECT_FALSE(EqualityComparable<arch>::value);
      EXPECT_FALSE(NothrowEqualityComparable<arch>::value);
      break;
    case ti::equality_comparable::yes:
      EXPECT_TRUE(EqualityComparable<arch>::value);
      EXPECT_FALSE(NothrowEqualityComparable<arch>::value);
      break;
    case ti::equality_comparable::nothrow:
      EXPECT_TRUE(EqualityComparable<arch>::value);
      EXPECT_TRUE(NothrowEqualityComparable<arch>::value);
      break;
  }

  //////////////////////////////////////////////////////////////////////////////
  //                       Inequality comparable checks                       //
  //////////////////////////////////////////////////////////////////////////////
  switch (expected_props::inequality_comparable_support) {
    case ti::inequality_comparable::maybe:
      EXPECT_FALSE(InequalityComparable<arch>::value);
      EXPECT_FALSE(NothrowInequalityComparable<arch>::value);
      break;
    case ti::inequality_comparable::yes:
      EXPECT_TRUE(InequalityComparable<arch>::value);
      EXPECT_FALSE(NothrowInequalityComparable<arch>::value);
      break;
    case ti::inequality_comparable::nothrow:
      EXPECT_TRUE(InequalityComparable<arch>::value);
      EXPECT_TRUE(NothrowInequalityComparable<arch>::value);
      break;
  }

  //////////////////////////////////////////////////////////////////////////////
  //                       Less than comparable checks                        //
  //////////////////////////////////////////////////////////////////////////////
  switch (expected_props::less_than_comparable_support) {
    case ti::less_than_comparable::maybe:
      EXPECT_FALSE(LessThanComparable<arch>::value);
      EXPECT_FALSE(NothrowLessThanComparable<arch>::value);
      break;
    case ti::less_than_comparable::yes:
      EXPECT_TRUE(LessThanComparable<arch>::value);
      EXPECT_FALSE(NothrowLessThanComparable<arch>::value);
      break;
    case ti::less_than_comparable::nothrow:
      EXPECT_TRUE(LessThanComparable<arch>::value);
      EXPECT_TRUE(NothrowLessThanComparable<arch>::value);
      break;
  }

  //////////////////////////////////////////////////////////////////////////////
  //                      Less equal comparable checks                        //
  //////////////////////////////////////////////////////////////////////////////
  switch (expected_props::less_equal_comparable_support) {
    case ti::less_equal_comparable::maybe:
      EXPECT_FALSE(LessEqualComparable<arch>::value);
      EXPECT_FALSE(NothrowLessEqualComparable<arch>::value);
      break;
    case ti::less_equal_comparable::yes:
      EXPECT_TRUE(LessEqualComparable<arch>::value);
      EXPECT_FALSE(NothrowLessEqualComparable<arch>::value);
      break;
    case ti::less_equal_comparable::nothrow:
      EXPECT_TRUE(LessEqualComparable<arch>::value);
      EXPECT_TRUE(NothrowLessEqualComparable<arch>::value);
      break;
  }

  //////////////////////////////////////////////////////////////////////////////
  //                     Greater equal comparable checks                      //
  //////////////////////////////////////////////////////////////////////////////
  switch (expected_props::greater_equal_comparable_support) {
    case ti::greater_equal_comparable::maybe:
      EXPECT_FALSE(GreaterEqualComparable<arch>::value);
      EXPECT_FALSE(NothrowGreaterEqualComparable<arch>::value);
      break;
    case ti::greater_equal_comparable::yes:
      EXPECT_TRUE(GreaterEqualComparable<arch>::value);
      EXPECT_FALSE(NothrowGreaterEqualComparable<arch>::value);
      break;
    case ti::greater_equal_comparable::nothrow:
      EXPECT_TRUE(GreaterEqualComparable<arch>::value);
      EXPECT_TRUE(NothrowGreaterEqualComparable<arch>::value);
      break;
  }

  //////////////////////////////////////////////////////////////////////////////
  //                     Greater than comparable checks                       //
  //////////////////////////////////////////////////////////////////////////////
  switch (expected_props::greater_than_comparable_support) {
    case ti::greater_than_comparable::maybe:
      EXPECT_FALSE(GreaterThanComparable<arch>::value);
      EXPECT_FALSE(NothrowGreaterThanComparable<arch>::value);
      break;
    case ti::greater_than_comparable::yes:
      EXPECT_TRUE(GreaterThanComparable<arch>::value);
      EXPECT_FALSE(NothrowGreaterThanComparable<arch>::value);
      break;
    case ti::greater_than_comparable::nothrow:
      EXPECT_TRUE(GreaterThanComparable<arch>::value);
      EXPECT_TRUE(NothrowGreaterThanComparable<arch>::value);
      break;
  }
}

TYPED_TEST_P(ProfileTest, HasAppropriateAuxilliaryProperties) {
  using profile = typename TypeParam::profile;
  using arch = typename TypeParam::arch;
  using expected_profile = typename TypeParam::expected_profile;

  using props = ti::PropertiesOfT<profile>;
  using arch_props = ti::PropertiesOfArchetypeT<arch>;
  using expected_props = ti::PropertiesOfT<expected_profile>;

  // Make sure all of the properties are as expected.
  // There are seemingly redundant tests here to make it easier to diagnose
  // the specifics of the failure if something were to go wrong.
  EXPECT_TRUE((std::is_same<props, arch_props>::value));
  EXPECT_TRUE((std::is_same<props, expected_props>::value));
  EXPECT_TRUE((std::is_same<arch_props, expected_props>::value));

  EXPECT_EQ(props::swappable_support, expected_props::swappable_support);

  EXPECT_EQ(props::hashable_support, expected_props::hashable_support);

  // Avoid additional error message noise when profile and archetype match with
  // each other but were not what was expected.
  if (!std::is_same<props, arch_props>::value) {
    EXPECT_EQ(arch_props::swappable_support, expected_props::swappable_support);

    EXPECT_EQ(arch_props::hashable_support, expected_props::hashable_support);
  }

  //////////////////////////////////////////////////////////////////////////////
  //                            Swappable checks                              //
  //////////////////////////////////////////////////////////////////////////////
  switch (expected_props::swappable_support) {
    case ti::swappable::maybe:
      EXPECT_FALSE(abel::type_traits_internal::IsSwappable<arch>::value);
      EXPECT_FALSE(abel::type_traits_internal::IsNothrowSwappable<arch>::value);
      break;
    case ti::swappable::yes:
      EXPECT_TRUE(abel::type_traits_internal::IsSwappable<arch>::value);
      EXPECT_FALSE(abel::type_traits_internal::IsNothrowSwappable<arch>::value);
      break;
    case ti::swappable::nothrow:
      EXPECT_TRUE(abel::type_traits_internal::IsSwappable<arch>::value);
      EXPECT_TRUE(abel::type_traits_internal::IsNothrowSwappable<arch>::value);
      break;
  }

  //////////////////////////////////////////////////////////////////////////////
  //                             Hashable checks                              //
  //////////////////////////////////////////////////////////////////////////////
  switch (expected_props::hashable_support) {
    case ti::hashable::maybe:
#if ABEL_META_INTERNAL_STD_HASH_SFINAE_FRIENDLY_
      EXPECT_FALSE(abel::type_traits_internal::IsHashable<arch>::value);
#endif  // ABEL_META_INTERNAL_STD_HASH_SFINAE_FRIENDLY_
      break;
    case ti::hashable::yes:
      EXPECT_TRUE(abel::type_traits_internal::IsHashable<arch>::value);
      break;
  }
}

REGISTER_TYPED_TEST_SUITE_P(ProfileTest, HasAppropriateConstructionProperties,
                            HasAppropriateAssignmentProperties,
                            HasAppropriateComparisonProperties,
                            HasAppropriateAuxilliaryProperties);

template <class Profile, class Arch, class ExpectedProfile>
struct ProfileAndExpectation {
  using profile = Profile;
  using arch = Arch;
  using expected_profile = ExpectedProfile;
};

using CoreProfilesToTest = ::testing::Types<
    // The terminating case of combine (all properties are "maybe").
    ProfileAndExpectation<ti::CombineProfiles<>,
                          ti::Archetype<ti::CombineProfiles<>>,
                          ti::ConformanceProfile<>>,

    // Core default constructor profiles
    ProfileAndExpectation<
        ti::HasDefaultConstructorProfile, ti::HasDefaultConstructorArchetype,
        ti::ConformanceProfile<ti::default_constructible::yes>>,
    ProfileAndExpectation<
        ti::HasNothrowDefaultConstructorProfile,
        ti::HasNothrowDefaultConstructorArchetype,
        ti::ConformanceProfile<ti::default_constructible::nothrow>>,
    ProfileAndExpectation<
        ti::HasTrivialDefaultConstructorProfile,
        ti::HasTrivialDefaultConstructorArchetype,
        ti::ConformanceProfile<ti::default_constructible::trivial>>,

    // Core move constructor profiles
    ProfileAndExpectation<
        ti::HasMoveConstructorProfile, ti::HasMoveConstructorArchetype,
        ti::ConformanceProfile<ti::default_constructible::maybe,
                               ti::move_constructible::yes>>,
    ProfileAndExpectation<
        ti::HasNothrowMoveConstructorProfile,
        ti::HasNothrowMoveConstructorArchetype,
        ti::ConformanceProfile<ti::default_constructible::maybe,
                               ti::move_constructible::nothrow>>,
    ProfileAndExpectation<
        ti::HasTrivialMoveConstructorProfile,
        ti::HasTrivialMoveConstructorArchetype,
        ti::ConformanceProfile<ti::default_constructible::maybe,
                               ti::move_constructible::trivial>>,

    // Core copy constructor profiles
    ProfileAndExpectation<
        ti::HasCopyConstructorProfile, ti::HasCopyConstructorArchetype,
        ti::ConformanceProfile<ti::default_constructible::maybe,
                               ti::move_constructible::maybe,
                               ti::copy_constructible::yes>>,
    ProfileAndExpectation<
        ti::HasNothrowCopyConstructorProfile,
        ti::HasNothrowCopyConstructorArchetype,
        ti::ConformanceProfile<ti::default_constructible::maybe,
                               ti::move_constructible::maybe,
                               ti::copy_constructible::nothrow>>,
    ProfileAndExpectation<
        ti::HasTrivialCopyConstructorProfile,
        ti::HasTrivialCopyConstructorArchetype,
        ti::ConformanceProfile<ti::default_constructible::maybe,
                               ti::move_constructible::maybe,
                               ti::copy_constructible::trivial>>,

    // Core move assignment profiles
    ProfileAndExpectation<
        ti::HasMoveAssignProfile, ti::HasMoveAssignArchetype,
        ti::ConformanceProfile<
            ti::default_constructible::maybe, ti::move_constructible::maybe,
            ti::copy_constructible::maybe, ti::move_assignable::yes>>,
    ProfileAndExpectation<
        ti::HasNothrowMoveAssignProfile, ti::HasNothrowMoveAssignArchetype,
        ti::ConformanceProfile<
            ti::default_constructible::maybe, ti::move_constructible::maybe,
            ti::copy_constructible::maybe, ti::move_assignable::nothrow>>,
    ProfileAndExpectation<
        ti::HasTrivialMoveAssignProfile, ti::HasTrivialMoveAssignArchetype,
        ti::ConformanceProfile<
            ti::default_constructible::maybe, ti::move_constructible::maybe,
            ti::copy_constructible::maybe, ti::move_assignable::trivial>>,

    // Core copy assignment profiles
    ProfileAndExpectation<
        ti::HasCopyAssignProfile, ti::HasCopyAssignArchetype,
        ti::ConformanceProfile<
            ti::default_constructible::maybe, ti::move_constructible::maybe,
            ti::copy_constructible::maybe, ti::move_assignable::maybe,
            ti::copy_assignable::yes>>,
    ProfileAndExpectation<
        ti::HasNothrowCopyAssignProfile, ti::HasNothrowCopyAssignArchetype,
        ti::ConformanceProfile<
            ti::default_constructible::maybe, ti::move_constructible::maybe,
            ti::copy_constructible::maybe, ti::move_assignable::maybe,
            ti::copy_assignable::nothrow>>,
    ProfileAndExpectation<
        ti::HasTrivialCopyAssignProfile, ti::HasTrivialCopyAssignArchetype,
        ti::ConformanceProfile<
            ti::default_constructible::maybe, ti::move_constructible::maybe,
            ti::copy_constructible::maybe, ti::move_assignable::maybe,
            ti::copy_assignable::trivial>>,

    // Core destructor profiles
    ProfileAndExpectation<
        ti::HasDestructorProfile, ti::HasDestructorArchetype,
        ti::ConformanceProfile<
            ti::default_constructible::maybe, ti::move_constructible::maybe,
            ti::copy_constructible::maybe, ti::move_assignable::maybe,
            ti::copy_assignable::maybe, ti::destructible::yes>>,
    ProfileAndExpectation<
        ti::HasNothrowDestructorProfile, ti::HasNothrowDestructorArchetype,
        ti::ConformanceProfile<
            ti::default_constructible::maybe, ti::move_constructible::maybe,
            ti::copy_constructible::maybe, ti::move_assignable::maybe,
            ti::copy_assignable::maybe, ti::destructible::nothrow>>,
    ProfileAndExpectation<
        ti::HasTrivialDestructorProfile, ti::HasTrivialDestructorArchetype,
        ti::ConformanceProfile<
            ti::default_constructible::maybe, ti::move_constructible::maybe,
            ti::copy_constructible::maybe, ti::move_assignable::maybe,
            ti::copy_assignable::maybe, ti::destructible::trivial>>,

    // Core equality comparable profiles
    ProfileAndExpectation<
        ti::HasEqualityProfile, ti::HasEqualityArchetype,
        ti::ConformanceProfile<
            ti::default_constructible::maybe, ti::move_constructible::maybe,
            ti::copy_constructible::maybe, ti::move_assignable::maybe,
            ti::copy_assignable::maybe, ti::destructible::maybe,
            ti::equality_comparable::yes>>,
    ProfileAndExpectation<
        ti::HasNothrowEqualityProfile, ti::HasNothrowEqualityArchetype,
        ti::ConformanceProfile<
            ti::default_constructible::maybe, ti::move_constructible::maybe,
            ti::copy_constructible::maybe, ti::move_assignable::maybe,
            ti::copy_assignable::maybe, ti::destructible::maybe,
            ti::equality_comparable::nothrow>>,

    // Core inequality comparable profiles
    ProfileAndExpectation<
        ti::HasInequalityProfile, ti::HasInequalityArchetype,
        ti::ConformanceProfile<
            ti::default_constructible::maybe, ti::move_constructible::maybe,
            ti::copy_constructible::maybe, ti::move_assignable::maybe,
            ti::copy_assignable::maybe, ti::destructible::maybe,
            ti::equality_comparable::maybe, ti::inequality_comparable::yes>>,
    ProfileAndExpectation<
        ti::HasNothrowInequalityProfile, ti::HasNothrowInequalityArchetype,
        ti::ConformanceProfile<
            ti::default_constructible::maybe, ti::move_constructible::maybe,
            ti::copy_constructible::maybe, ti::move_assignable::maybe,
            ti::copy_assignable::maybe, ti::destructible::maybe,
            ti::equality_comparable::maybe,
            ti::inequality_comparable::nothrow>>,

    // Core less than comparable profiles
    ProfileAndExpectation<
        ti::HasLessThanProfile, ti::HasLessThanArchetype,
        ti::ConformanceProfile<
            ti::default_constructible::maybe, ti::move_constructible::maybe,
            ti::copy_constructible::maybe, ti::move_assignable::maybe,
            ti::copy_assignable::maybe, ti::destructible::maybe,
            ti::equality_comparable::maybe, ti::inequality_comparable::maybe,
            ti::less_than_comparable::yes>>,
    ProfileAndExpectation<
        ti::HasNothrowLessThanProfile, ti::HasNothrowLessThanArchetype,
        ti::ConformanceProfile<
            ti::default_constructible::maybe, ti::move_constructible::maybe,
            ti::copy_constructible::maybe, ti::move_assignable::maybe,
            ti::copy_assignable::maybe, ti::destructible::maybe,
            ti::equality_comparable::maybe, ti::inequality_comparable::maybe,
            ti::less_than_comparable::nothrow>>,

    // Core less equal comparable profiles
    ProfileAndExpectation<
        ti::HasLessEqualProfile, ti::HasLessEqualArchetype,
        ti::ConformanceProfile<
            ti::default_constructible::maybe, ti::move_constructible::maybe,
            ti::copy_constructible::maybe, ti::move_assignable::maybe,
            ti::copy_assignable::maybe, ti::destructible::maybe,
            ti::equality_comparable::maybe, ti::inequality_comparable::maybe,
            ti::less_than_comparable::maybe, ti::less_equal_comparable::yes>>,
    ProfileAndExpectation<
        ti::HasNothrowLessEqualProfile, ti::HasNothrowLessEqualArchetype,
        ti::ConformanceProfile<
            ti::default_constructible::maybe, ti::move_constructible::maybe,
            ti::copy_constructible::maybe, ti::move_assignable::maybe,
            ti::copy_assignable::maybe, ti::destructible::maybe,
            ti::equality_comparable::maybe, ti::inequality_comparable::maybe,
            ti::less_than_comparable::maybe,
            ti::less_equal_comparable::nothrow>>,

    // Core greater equal comparable profiles
    ProfileAndExpectation<
        ti::HasGreaterEqualProfile, ti::HasGreaterEqualArchetype,
        ti::ConformanceProfile<
            ti::default_constructible::maybe, ti::move_constructible::maybe,
            ti::copy_constructible::maybe, ti::move_assignable::maybe,
            ti::copy_assignable::maybe, ti::destructible::maybe,
            ti::equality_comparable::maybe, ti::inequality_comparable::maybe,
            ti::less_than_comparable::maybe, ti::less_equal_comparable::maybe,
            ti::greater_equal_comparable::yes>>,
    ProfileAndExpectation<
        ti::HasNothrowGreaterEqualProfile, ti::HasNothrowGreaterEqualArchetype,
        ti::ConformanceProfile<
            ti::default_constructible::maybe, ti::move_constructible::maybe,
            ti::copy_constructible::maybe, ti::move_assignable::maybe,
            ti::copy_assignable::maybe, ti::destructible::maybe,
            ti::equality_comparable::maybe, ti::inequality_comparable::maybe,
            ti::less_than_comparable::maybe, ti::less_equal_comparable::maybe,
            ti::greater_equal_comparable::nothrow>>,

    // Core greater than comparable profiles
    ProfileAndExpectation<
        ti::HasGreaterThanProfile, ti::HasGreaterThanArchetype,
        ti::ConformanceProfile<
            ti::default_constructible::maybe, ti::move_constructible::maybe,
            ti::copy_constructible::maybe, ti::move_assignable::maybe,
            ti::copy_assignable::maybe, ti::destructible::maybe,
            ti::equality_comparable::maybe, ti::inequality_comparable::maybe,
            ti::less_than_comparable::maybe, ti::less_equal_comparable::maybe,
            ti::greater_equal_comparable::maybe,
            ti::greater_than_comparable::yes>>,
    ProfileAndExpectation<
        ti::HasNothrowGreaterThanProfile, ti::HasNothrowGreaterThanArchetype,
        ti::ConformanceProfile<
            ti::default_constructible::maybe, ti::move_constructible::maybe,
            ti::copy_constructible::maybe, ti::move_assignable::maybe,
            ti::copy_assignable::maybe, ti::destructible::maybe,
            ti::equality_comparable::maybe, ti::inequality_comparable::maybe,
            ti::less_than_comparable::maybe, ti::less_equal_comparable::maybe,
            ti::greater_equal_comparable::maybe,
            ti::greater_than_comparable::nothrow>>,

    // Core swappable profiles
    ProfileAndExpectation<
        ti::HasSwapProfile, ti::HasSwapArchetype,
        ti::ConformanceProfile<
            ti::default_constructible::maybe, ti::move_constructible::maybe,
            ti::copy_constructible::maybe, ti::move_assignable::maybe,
            ti::copy_assignable::maybe, ti::destructible::maybe,
            ti::equality_comparable::maybe, ti::inequality_comparable::maybe,
            ti::less_than_comparable::maybe, ti::less_equal_comparable::maybe,
            ti::greater_equal_comparable::maybe,
            ti::greater_than_comparable::maybe, ti::swappable::yes>>,
    ProfileAndExpectation<
        ti::HasNothrowSwapProfile, ti::HasNothrowSwapArchetype,
        ti::ConformanceProfile<
            ti::default_constructible::maybe, ti::move_constructible::maybe,
            ti::copy_constructible::maybe, ti::move_assignable::maybe,
            ti::copy_assignable::maybe, ti::destructible::maybe,
            ti::equality_comparable::maybe, ti::inequality_comparable::maybe,
            ti::less_than_comparable::maybe, ti::less_equal_comparable::maybe,
            ti::greater_equal_comparable::maybe,
            ti::greater_than_comparable::maybe, ti::swappable::nothrow>>,

    // Core hashable profiles
    ProfileAndExpectation<
        ti::HasStdHashSpecializationProfile,
        ti::HasStdHashSpecializationArchetype,
        ti::ConformanceProfile<
            ti::default_constructible::maybe, ti::move_constructible::maybe,
            ti::copy_constructible::maybe, ti::move_assignable::maybe,
            ti::copy_assignable::maybe, ti::destructible::maybe,
            ti::equality_comparable::maybe, ti::inequality_comparable::maybe,
            ti::less_than_comparable::maybe, ti::less_equal_comparable::maybe,
            ti::greater_equal_comparable::maybe,
            ti::greater_than_comparable::maybe, ti::swappable::maybe,
            ti::hashable::yes>>>;

using CommonProfilesToTest = ::testing::Types<
    // NothrowMoveConstructible
    ProfileAndExpectation<
        ti::NothrowMoveConstructibleProfile,
        ti::NothrowMoveConstructibleArchetype,
        ti::ConformanceProfile<
            ti::default_constructible::maybe, ti::move_constructible::nothrow,
            ti::copy_constructible::maybe, ti::move_assignable::maybe,
            ti::copy_assignable::maybe, ti::destructible::nothrow>>,

    // CopyConstructible
    ProfileAndExpectation<
        ti::CopyConstructibleProfile, ti::CopyConstructibleArchetype,
        ti::ConformanceProfile<
            ti::default_constructible::maybe, ti::move_constructible::nothrow,
            ti::copy_constructible::yes, ti::move_assignable::maybe,
            ti::copy_assignable::maybe, ti::destructible::nothrow>>,

    // NothrowMovable
    ProfileAndExpectation<
        ti::NothrowMovableProfile, ti::NothrowMovableArchetype,
        ti::ConformanceProfile<
            ti::default_constructible::maybe, ti::move_constructible::nothrow,
            ti::copy_constructible::maybe, ti::move_assignable::nothrow,
            ti::copy_assignable::maybe, ti::destructible::nothrow,
            ti::equality_comparable::maybe, ti::inequality_comparable::maybe,
            ti::less_than_comparable::maybe, ti::less_equal_comparable::maybe,
            ti::greater_equal_comparable::maybe,
            ti::greater_than_comparable::maybe, ti::swappable::nothrow>>,

    // Value
    ProfileAndExpectation<
        ti::ValueProfile, ti::ValueArchetype,
        ti::ConformanceProfile<
            ti::default_constructible::maybe, ti::move_constructible::nothrow,
            ti::copy_constructible::yes, ti::move_assignable::nothrow,
            ti::copy_assignable::yes, ti::destructible::nothrow,
            ti::equality_comparable::maybe, ti::inequality_comparable::maybe,
            ti::less_than_comparable::maybe, ti::less_equal_comparable::maybe,
            ti::greater_equal_comparable::maybe,
            ti::greater_than_comparable::maybe, ti::swappable::nothrow>>,

    ////////////////////////////////////////////////////////////////////////////
    //                  Common but also DefaultConstructible                  //
    ////////////////////////////////////////////////////////////////////////////

    // DefaultConstructibleNothrowMoveConstructible
    ProfileAndExpectation<
        ti::DefaultConstructibleNothrowMoveConstructibleProfile,
        ti::DefaultConstructibleNothrowMoveConstructibleArchetype,
        ti::ConformanceProfile<
            ti::default_constructible::yes, ti::move_constructible::nothrow,
            ti::copy_constructible::maybe, ti::move_assignable::maybe,
            ti::copy_assignable::maybe, ti::destructible::nothrow>>,

    // DefaultConstructibleCopyConstructible
    ProfileAndExpectation<
        ti::DefaultConstructibleCopyConstructibleProfile,
        ti::DefaultConstructibleCopyConstructibleArchetype,
        ti::ConformanceProfile<
            ti::default_constructible::yes, ti::move_constructible::nothrow,
            ti::copy_constructible::yes, ti::move_assignable::maybe,
            ti::copy_assignable::maybe, ti::destructible::nothrow>>,

    // DefaultConstructibleNothrowMovable
    ProfileAndExpectation<
        ti::DefaultConstructibleNothrowMovableProfile,
        ti::DefaultConstructibleNothrowMovableArchetype,
        ti::ConformanceProfile<
            ti::default_constructible::yes, ti::move_constructible::nothrow,
            ti::copy_constructible::maybe, ti::move_assignable::nothrow,
            ti::copy_assignable::maybe, ti::destructible::nothrow,
            ti::equality_comparable::maybe, ti::inequality_comparable::maybe,
            ti::less_than_comparable::maybe, ti::less_equal_comparable::maybe,
            ti::greater_equal_comparable::maybe,
            ti::greater_than_comparable::maybe, ti::swappable::nothrow>>,

    // DefaultConstructibleValue
    ProfileAndExpectation<
        ti::DefaultConstructibleValueProfile,
        ti::DefaultConstructibleValueArchetype,
        ti::ConformanceProfile<
            ti::default_constructible::yes, ti::move_constructible::nothrow,
            ti::copy_constructible::yes, ti::move_assignable::nothrow,
            ti::copy_assignable::yes, ti::destructible::nothrow,
            ti::equality_comparable::maybe, ti::inequality_comparable::maybe,
            ti::less_than_comparable::maybe, ti::less_equal_comparable::maybe,
            ti::greater_equal_comparable::maybe,
            ti::greater_than_comparable::maybe, ti::swappable::nothrow>>>;

using ComparableHelpersProfilesToTest = ::testing::Types<
    // Equatable
    ProfileAndExpectation<
        ti::EquatableProfile, ti::EquatableArchetype,
        ti::ConformanceProfile<
            ti::default_constructible::maybe, ti::move_constructible::maybe,
            ti::copy_constructible::maybe, ti::move_assignable::maybe,
            ti::copy_assignable::maybe, ti::destructible::maybe,
            ti::equality_comparable::yes, ti::inequality_comparable::yes>>,

    // Comparable
    ProfileAndExpectation<
        ti::ComparableProfile, ti::ComparableArchetype,
        ti::ConformanceProfile<
            ti::default_constructible::maybe, ti::move_constructible::maybe,
            ti::copy_constructible::maybe, ti::move_assignable::maybe,
            ti::copy_assignable::maybe, ti::destructible::maybe,
            ti::equality_comparable::yes, ti::inequality_comparable::yes,
            ti::less_than_comparable::yes, ti::less_equal_comparable::yes,
            ti::greater_equal_comparable::yes,
            ti::greater_than_comparable::yes>>,

    // NothrowEquatable
    ProfileAndExpectation<
        ti::NothrowEquatableProfile, ti::NothrowEquatableArchetype,
        ti::ConformanceProfile<
            ti::default_constructible::maybe, ti::move_constructible::maybe,
            ti::copy_constructible::maybe, ti::move_assignable::maybe,
            ti::copy_assignable::maybe, ti::destructible::maybe,
            ti::equality_comparable::nothrow,
            ti::inequality_comparable::nothrow>>,

    // NothrowComparable
    ProfileAndExpectation<
        ti::NothrowComparableProfile, ti::NothrowComparableArchetype,
        ti::ConformanceProfile<
            ti::default_constructible::maybe, ti::move_constructible::maybe,
            ti::copy_constructible::maybe, ti::move_assignable::maybe,
            ti::copy_assignable::maybe, ti::destructible::maybe,
            ti::equality_comparable::nothrow,
            ti::inequality_comparable::nothrow,
            ti::less_than_comparable::nothrow,
            ti::less_equal_comparable::nothrow,
            ti::greater_equal_comparable::nothrow,
            ti::greater_than_comparable::nothrow>>>;

using CommonComparableProfilesToTest = ::testing::Types<
    // ComparableNothrowMoveConstructible
    ProfileAndExpectation<
        ti::ComparableNothrowMoveConstructibleProfile,
        ti::ComparableNothrowMoveConstructibleArchetype,
        ti::ConformanceProfile<
            ti::default_constructible::maybe, ti::move_constructible::nothrow,
            ti::copy_constructible::maybe, ti::move_assignable::maybe,
            ti::copy_assignable::maybe, ti::destructible::nothrow,
            ti::equality_comparable::yes, ti::inequality_comparable::yes,
            ti::less_than_comparable::yes, ti::less_equal_comparable::yes,
            ti::greater_equal_comparable::yes,
            ti::greater_than_comparable::yes>>,

    // ComparableCopyConstructible
    ProfileAndExpectation<
        ti::ComparableCopyConstructibleProfile,
        ti::ComparableCopyConstructibleArchetype,
        ti::ConformanceProfile<
            ti::default_constructible::maybe, ti::move_constructible::nothrow,
            ti::copy_constructible::yes, ti::move_assignable::maybe,
            ti::copy_assignable::maybe, ti::destructible::nothrow,
            ti::equality_comparable::yes, ti::inequality_comparable::yes,
            ti::less_than_comparable::yes, ti::less_equal_comparable::yes,
            ti::greater_equal_comparable::yes,
            ti::greater_than_comparable::yes>>,

    // ComparableNothrowMovable
    ProfileAndExpectation<
        ti::ComparableNothrowMovableProfile,
        ti::ComparableNothrowMovableArchetype,
        ti::ConformanceProfile<
            ti::default_constructible::maybe, ti::move_constructible::nothrow,
            ti::copy_constructible::maybe, ti::move_assignable::nothrow,
            ti::copy_assignable::maybe, ti::destructible::nothrow,
            ti::equality_comparable::yes, ti::inequality_comparable::yes,
            ti::less_than_comparable::yes, ti::less_equal_comparable::yes,
            ti::greater_equal_comparable::yes, ti::greater_than_comparable::yes,
            ti::swappable::nothrow>>,

    // ComparableValue
    ProfileAndExpectation<
        ti::ComparableValueProfile, ti::ComparableValueArchetype,
        ti::ConformanceProfile<
            ti::default_constructible::maybe, ti::move_constructible::nothrow,
            ti::copy_constructible::yes, ti::move_assignable::nothrow,
            ti::copy_assignable::yes, ti::destructible::nothrow,
            ti::equality_comparable::yes, ti::inequality_comparable::yes,
            ti::less_than_comparable::yes, ti::less_equal_comparable::yes,
            ti::greater_equal_comparable::yes, ti::greater_than_comparable::yes,
            ti::swappable::nothrow>>>;

using TrivialProfilesToTest = ::testing::Types<
    ProfileAndExpectation<
        ti::TrivialSpecialMemberFunctionsProfile,
        ti::TrivialSpecialMemberFunctionsArchetype,
        ti::ConformanceProfile<
            ti::default_constructible::trivial, ti::move_constructible::trivial,
            ti::copy_constructible::trivial, ti::move_assignable::trivial,
            ti::copy_assignable::trivial, ti::destructible::trivial,
            ti::equality_comparable::maybe, ti::inequality_comparable::maybe,
            ti::less_than_comparable::maybe, ti::less_equal_comparable::maybe,
            ti::greater_equal_comparable::maybe,
            ti::greater_than_comparable::maybe, ti::swappable::nothrow>>,

    ProfileAndExpectation<
        ti::TriviallyCompleteProfile, ti::TriviallyCompleteArchetype,
        ti::ConformanceProfile<
            ti::default_constructible::trivial, ti::move_constructible::trivial,
            ti::copy_constructible::trivial, ti::move_assignable::trivial,
            ti::copy_assignable::trivial, ti::destructible::trivial,
            ti::equality_comparable::yes, ti::inequality_comparable::yes,
            ti::less_than_comparable::yes, ti::less_equal_comparable::yes,
            ti::greater_equal_comparable::yes, ti::greater_than_comparable::yes,
            ti::swappable::nothrow, ti::hashable::yes>>>;

INSTANTIATE_TYPED_TEST_SUITE_P(Core, ProfileTest, CoreProfilesToTest);
INSTANTIATE_TYPED_TEST_SUITE_P(Common, ProfileTest, CommonProfilesToTest);
INSTANTIATE_TYPED_TEST_SUITE_P(ComparableHelpers, ProfileTest,
                               ComparableHelpersProfilesToTest);
INSTANTIATE_TYPED_TEST_SUITE_P(CommonComparable, ProfileTest,
                               CommonComparableProfilesToTest);
INSTANTIATE_TYPED_TEST_SUITE_P(Trivial, ProfileTest, TrivialProfilesToTest);

// TODO(calabrese) Test runtime results

}  // namespace
