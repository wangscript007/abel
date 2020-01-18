//
// -----------------------------------------------------------------------------
// mocking_bit_gen.h
// -----------------------------------------------------------------------------
//
// This file includes an `abel::MockingBitGen` class to use as a mock within the
// Googletest testing framework. Such a mock is useful to provide deterministic
// values as return values within (otherwise random) abel distribution
// functions. Such determinism within a mock is useful within testing frameworks
// to test otherwise indeterminate APIs.
//
// More information about the Googletest testing framework is available at
// https://github.com/google/googletest

#ifndef ABEL_RANDOM_MOCKING_BIT_GEN_H_
#define ABEL_RANDOM_MOCKING_BIT_GEN_H_

#include <iterator>
#include <limits>
#include <memory>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <utility>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <abel/container/flat_hash_map.h>
#include <abel/meta/type_traits.h>
#include <abel/random/distributions.h>
#include <abel/random/internal/distribution_caller.h>
#include <testing/mocking_bit_gen_base.h>
#include <abel/strings/str_cat.h>
#include <abel/strings/str_join.h>
#include <abel/types/span.h>
#include <abel/types/variant.h>
#include <abel/utility/utility.h>

namespace abel {
ABEL_NAMESPACE_BEGIN

namespace random_internal {

template <typename, typename>
struct MockSingleOverload;

}  // namespace random_internal

// MockingBitGen
//
// `abel::MockingBitGen` is a mock Uniform Random Bit Generator (URBG) class
// which can act in place of an `abel::BitGen` URBG within tests using the
// Googletest testing framework.
//
// Usage:
//
// Use an `abel::MockingBitGen` along with a mock distribution object (within
// mock_distributions.h) inside Googletest constructs such as ON_CALL(),
// EXPECT_TRUE(), etc. to produce deterministic results conforming to the
// distribution's API contract.
//
// Example:
//
//  // Mock a call to an `abel::Bernoulli` distribution using Googletest
//   abel::MockingBitGen bitgen;
//
//   ON_CALL(abel::MockBernoulli(), Call(bitgen, 0.5))
//       .WillByDefault(testing::Return(true));
//   EXPECT_TRUE(abel::Bernoulli(bitgen, 0.5));
//
//  // Mock a call to an `abel::Uniform` distribution within Googletest
//  abel::MockingBitGen bitgen;
//
//   ON_CALL(abel::MockUniform<int>(), Call(bitgen, testing::_, testing::_))
//       .WillByDefault([] (int low, int high) {
//           return (low + high) / 2;
//       });
//
//   EXPECT_EQ(abel::Uniform<int>(gen, 0, 10), 5);
//   EXPECT_EQ(abel::Uniform<int>(gen, 30, 40), 35);
//
// At this time, only mock distributions supplied within the abel random
// library are officially supported.
//
class MockingBitGen : public abel::random_internal::MockingBitGenBase {
 public:
  MockingBitGen() {}

  ~MockingBitGen() override;

 private:
  template <typename DistrT, typename... Args>
  using MockFnType =
      ::testing::MockFunction<typename DistrT::result_type(Args...)>;

  // MockingBitGen::Register
  //
  // Register<DistrT, FormatT, ArgTupleT> is the main extension point for
  // extending the MockingBitGen framework. It provides a mechanism to install a
  // mock expectation for the distribution `distr_t` onto the MockingBitGen
  // context.
  //
  // The returned MockFunction<...> type can be used to setup additional
  // distribution parameters of the expectation.
  template <typename DistrT, typename... Args, typename... Ms>
  decltype(std::declval<MockFnType<DistrT, Args...>>().gmock_Call(
      std::declval<Ms>()...))
  Register(Ms&&... matchers) {
    auto& mock =
        mocks_[std::type_index(GetTypeId<DistrT, std::tuple<Args...>>())];

    if (!mock.mock_fn) {
      auto* mock_fn = new MockFnType<DistrT, Args...>;
      mock.mock_fn = mock_fn;
      mock.match_impl = &MatchImpl<DistrT, Args...>;
      deleters_.emplace_back([mock_fn] { delete mock_fn; });
    }

    return static_cast<MockFnType<DistrT, Args...>*>(mock.mock_fn)
        ->gmock_Call(std::forward<Ms>(matchers)...);
  }

  mutable std::vector<std::function<void()>> deleters_;

  using match_impl_fn = void (*)(void* mock_fn, void* t_erased_dist_args,
                                 void* t_erased_result);
  struct MockData {
    void* mock_fn = nullptr;
    match_impl_fn match_impl = nullptr;
  };

  mutable abel::flat_hash_map<std::type_index, MockData> mocks_;

  template <typename DistrT, typename... Args>
  static void MatchImpl(void* mock_fn, void* dist_args, void* result) {
    using result_type = typename DistrT::result_type;
    *static_cast<result_type*>(result) = abel::apply(
        [mock_fn](Args... args) -> result_type {
          return (*static_cast<MockFnType<DistrT, Args...>*>(mock_fn))
              .Call(std::move(args)...);
        },
        *static_cast<std::tuple<Args...>*>(dist_args));
  }

  // Looks for an appropriate mock - Returns the mocked result if one is found.
  // Otherwise, returns a random value generated by the underlying URBG.
  bool CallImpl(const std::type_info& key_type, void* dist_args,
                void* result) override {
    // Trigger a mock, if there exists one that matches `param`.
    auto it = mocks_.find(std::type_index(key_type));
    if (it == mocks_.end()) return false;
    auto* mock_data = static_cast<MockData*>(&it->second);
    mock_data->match_impl(mock_data->mock_fn, dist_args, result);
    return true;
  }

  template <typename, typename>
  friend struct ::abel::random_internal::MockSingleOverload;
  friend struct ::abel::random_internal::DistributionCaller<
      abel::MockingBitGen>;
};

// -----------------------------------------------------------------------------
// Implementation Details Only Below
// -----------------------------------------------------------------------------

namespace random_internal {

template <>
struct DistributionCaller<abel::MockingBitGen> {
  template <typename DistrT, typename FormatT, typename... Args>
  static typename DistrT::result_type Call(abel::MockingBitGen* gen,
                                           Args&&... args) {
    return gen->template Call<DistrT, FormatT>(std::forward<Args>(args)...);
  }
};

}  // namespace random_internal
ABEL_NAMESPACE_END
}  // namespace abel

#endif  // ABEL_RANDOM_MOCKING_BIT_GEN_H_
