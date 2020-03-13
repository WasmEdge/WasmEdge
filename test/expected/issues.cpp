// SPDX-License-Identifier: CC0-1.0
#include "gtest/gtest.h"
#include <experimental/expected.hpp>
#include <memory>
#include <string>

using std::experimental::expected;
using std::experimental::unexpect;
using std::experimental::unexpected;

expected<int, std::string> getInt3(int val) { return val; }

expected<int, std::string> getInt2(int val) { return val; }

expected<int, std::string> getInt1() { return getInt2(5).and_then(getInt3); }

TEST(RegressionTest, Issue1) { getInt1(); }

expected<int, int> operation1() { return 42; }

expected<std::string, int> operation2(int const val) { return "Bananas"; }

TEST(RegressionTest, Issue17) {
  auto const intermediate_result = operation1();

  intermediate_result.and_then(operation2);
}

struct a {};
struct b : a {};

auto doit() -> expected<std::unique_ptr<b>, int> { return unexpected(0); }

TEST(RegressionTest, Issue23) {
  expected<std::unique_ptr<a>, int> msg = doit();
  EXPECT_FALSE(msg.has_value());
}

TEST(RegressionTest, Issue26) {
  expected<a, int> exp = expected<b, int>(unexpect, 0);
  EXPECT_FALSE(exp.has_value());
}

struct foo {
  foo() = default;
  foo(foo &) = delete;
  foo(foo &&){};
};

TEST(RegressionTest, Issue29) {
  std::vector<foo> v;
  v.emplace_back();
  expected<std::vector<foo>, int> ov = std::move(v);
  EXPECT_EQ(ov->size(), 1);
}

expected<int, std::string> error() {
  return unexpected(std::string("error1 "));
}
std::string maperror(std::string s) { return s + "maperror "; }

TEST(RegressionTest, Issue30) { error().map_error(maperror); }

struct i31 {
  int i;
};
TEST(RegressionTest, Issue31) {
  const expected<i31, int> a = i31{42};
  EXPECT_EQ(a->i, 42);

  expected<void, std::string> result;
  expected<void, std::string> result2 = result;
  result2 = result;
}

TEST(RegressionTest, Issue33) {
  expected<void, int> res{unexpect, 0};
  EXPECT_FALSE(res);
  res = res.map_error([](int i) { return 42; });
  EXPECT_EQ(res.error(), 42);
}

expected<void, std::string> voidWork() { return {}; }
expected<int, std::string> work2() { return 42; }
void errorhandling(std::string) {}

TEST(RegressionTest, Issue34) {
  expected<int, std::string> result = voidWork().and_then(work2);
  result.map_error([&](std::string result) { errorhandling(result); });
}

struct non_copyable {
  non_copyable(non_copyable &&) = default;
  non_copyable(non_copyable const &) = delete;
  non_copyable() = default;
};

TEST(RegressionTest, Issue42) {
  expected<non_copyable, int>{}.map([](non_copyable) {});
}

TEST(RegressionTest, Issue43) {
  auto result = expected<void, std::string>{};
  result = unexpected(std::string{"foo"});
}

using MaybeDataPtr = expected<int, std::unique_ptr<int>>;

MaybeDataPtr test(int i) noexcept { return std::move(i); }

MaybeDataPtr test2(int i) noexcept { return std::move(i); }

TEST(RegressionTest, Issue49) { auto m = test(10).and_then(test2); }

expected<int, std::unique_ptr<std::string>> func() { return 1; }

TEST(RegressionTest, Issue61) { EXPECT_EQ(func().value(), 1); }
