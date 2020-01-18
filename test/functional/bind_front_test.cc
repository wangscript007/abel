//

#include <abel/functional/bind_front.h>

#include <stddef.h>

#include <functional>
#include <memory>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <abel/memory/memory.h>

namespace {

char CharAt(const char* s, size_t index) { return s[index]; }

TEST(BindTest, Basics) {
  EXPECT_EQ('C', abel::bind_front(CharAt)("ABC", 2));
  EXPECT_EQ('C', abel::bind_front(CharAt, "ABC")(2));
  EXPECT_EQ('C', abel::bind_front(CharAt, "ABC", 2)());
}

TEST(BindTest, Lambda) {
  auto lambda = [](int x, int y, int z) { return x + y + z; };
  EXPECT_EQ(6, abel::bind_front(lambda)(1, 2, 3));
  EXPECT_EQ(6, abel::bind_front(lambda, 1)(2, 3));
  EXPECT_EQ(6, abel::bind_front(lambda, 1, 2)(3));
  EXPECT_EQ(6, abel::bind_front(lambda, 1, 2, 3)());
}

struct Functor {
  std::string operator()() & { return "&"; }
  std::string operator()() const& { return "const&"; }
  std::string operator()() && { return "&&"; }
  std::string operator()() const&& { return "const&&"; }
};

TEST(BindTest, PerfectForwardingOfBoundArgs) {
  auto f = abel::bind_front(Functor());
  const auto& cf = f;
  EXPECT_EQ("&", f());
  EXPECT_EQ("const&", cf());
  EXPECT_EQ("&&", std::move(f)());
  //EXPECT_EQ("const&&", std::move(cf)());
}

struct ArgDescribe {
  std::string operator()(int&) const { return "&"; }             // NOLINT
  std::string operator()(const int&) const { return "const&"; }  // NOLINT
  std::string operator()(int&&) const { return "&&"; }
  std::string operator()(const int&&) const { return "const&&"; }
};

TEST(BindTest, PerfectForwardingOfFreeArgs) {
  ArgDescribe f;
  int i;
  EXPECT_EQ("&", abel::bind_front(f)(static_cast<int&>(i)));
  EXPECT_EQ("const&", abel::bind_front(f)(static_cast<const int&>(i)));
  EXPECT_EQ("&&", abel::bind_front(f)(static_cast<int&&>(i)));
  EXPECT_EQ("const&&", abel::bind_front(f)(static_cast<const int&&>(i)));
}

struct NonCopyableFunctor {
  NonCopyableFunctor() = default;
  NonCopyableFunctor(const NonCopyableFunctor&) = delete;
  NonCopyableFunctor& operator=(const NonCopyableFunctor&) = delete;
  const NonCopyableFunctor* operator()() const { return this; }
};

TEST(BindTest, RefToFunctor) {
  // It won't copy/move the functor and use the original object.
  NonCopyableFunctor ncf;
  auto bound_ncf = abel::bind_front(std::ref(ncf));
  auto bound_ncf_copy = bound_ncf;
  EXPECT_EQ(&ncf, bound_ncf_copy());
}

struct Struct {
  std::string value;
};

TEST(BindTest, StoreByCopy) {
  Struct s = {"hello"};
  auto f = abel::bind_front(&Struct::value, s);
  auto g = f;
  EXPECT_EQ("hello", f());
  EXPECT_EQ("hello", g());
  EXPECT_NE(&s.value, &f());
  EXPECT_NE(&s.value, &g());
  EXPECT_NE(&g(), &f());
}

struct NonCopyable {
  explicit NonCopyable(const std::string& s) : value(s) {}
  NonCopyable(const NonCopyable&) = delete;
  NonCopyable& operator=(const NonCopyable&) = delete;

  std::string value;
};

const std::string& GetNonCopyableValue(const NonCopyable& n) { return n.value; }

TEST(BindTest, StoreByRef) {
  NonCopyable s("hello");
  auto f = abel::bind_front(&GetNonCopyableValue, std::ref(s));
  EXPECT_EQ("hello", f());
  EXPECT_EQ(&s.value, &f());
  auto g = std::move(f);  // NOLINT
  EXPECT_EQ("hello", g());
  EXPECT_EQ(&s.value, &g());
  s.value = "goodbye";
  EXPECT_EQ("goodbye", g());
}

TEST(BindTest, StoreByCRef) {
  NonCopyable s("hello");
  auto f = abel::bind_front(&GetNonCopyableValue, std::cref(s));
  EXPECT_EQ("hello", f());
  EXPECT_EQ(&s.value, &f());
  auto g = std::move(f);  // NOLINT
  EXPECT_EQ("hello", g());
  EXPECT_EQ(&s.value, &g());
  s.value = "goodbye";
  EXPECT_EQ("goodbye", g());
}

const std::string& GetNonCopyableValueByWrapper(
    std::reference_wrapper<NonCopyable> n) {
  return n.get().value;
}

TEST(BindTest, StoreByRefInvokeByWrapper) {
  NonCopyable s("hello");
  auto f = abel::bind_front(GetNonCopyableValueByWrapper, std::ref(s));
  EXPECT_EQ("hello", f());
  EXPECT_EQ(&s.value, &f());
  auto g = std::move(f);
  EXPECT_EQ("hello", g());
  EXPECT_EQ(&s.value, &g());
  s.value = "goodbye";
  EXPECT_EQ("goodbye", g());
}

TEST(BindTest, StoreByPointer) {
  NonCopyable s("hello");
  auto f = abel::bind_front(&NonCopyable::value, &s);
  EXPECT_EQ("hello", f());
  EXPECT_EQ(&s.value, &f());
  auto g = std::move(f);
  EXPECT_EQ("hello", g());
  EXPECT_EQ(&s.value, &g());
}

int Sink(std::unique_ptr<int> p) {
  return *p;
}

std::unique_ptr<int> Factory(int n) { return abel::make_unique<int>(n); }

TEST(BindTest, NonCopyableArg) {
  EXPECT_EQ(42, abel::bind_front(Sink)(abel::make_unique<int>(42)));
  EXPECT_EQ(42, abel::bind_front(Sink, abel::make_unique<int>(42))());
}

TEST(BindTest, NonCopyableResult) {
  EXPECT_THAT(abel::bind_front(Factory)(42), ::testing::Pointee(42));
  EXPECT_THAT(abel::bind_front(Factory, 42)(), ::testing::Pointee(42));
}

// is_copy_constructible<FalseCopyable<unique_ptr<T>> is true but an attempt to
// instantiate the copy constructor leads to a compile error. This is similar
// to how standard containers behave.
template <class T>
struct FalseCopyable {
  FalseCopyable() {}
  FalseCopyable(const FalseCopyable& other) : m(other.m) {}
  FalseCopyable(FalseCopyable&& other) : m(std::move(other.m)) {}
  T m;
};

int GetMember(FalseCopyable<std::unique_ptr<int>> x) { return *x.m; }

TEST(BindTest, WrappedMoveOnly) {
  FalseCopyable<std::unique_ptr<int>> x;
  x.m = abel::make_unique<int>(42);
  auto f = abel::bind_front(&GetMember, std::move(x));
  EXPECT_EQ(42, std::move(f)());
}

int Plus(int a, int b) { return a + b; }

TEST(BindTest, ConstExpr) {
  constexpr auto f = abel::bind_front(CharAt);
  EXPECT_EQ(f("ABC", 1), 'B');
  static constexpr int five = 5;
  constexpr auto plus5 = abel::bind_front(Plus, five);
  EXPECT_EQ(plus5(1), 6);

  // There seems to be a bug in MSVC dealing constexpr construction of
  // char[]. Notice 'plus5' above; 'int' works just fine.
#if !(defined(_MSC_VER) && _MSC_VER < 1910)
  static constexpr char data[] = "DEF";
  constexpr auto g = abel::bind_front(CharAt, data);
  EXPECT_EQ(g(1), 'E');
#endif
}

struct ManglingCall {
  int operator()(int, double, std::string) const { return 0; }
};

TEST(BindTest, Mangling) {
  // We just want to generate a particular instantiation to see its mangling.
  abel::bind_front(ManglingCall{}, 1, 3.3)("A");
}

}  // namespace
