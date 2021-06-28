// SPDX-License-Identifier: CC0-1.0
#include "gtest/gtest.h"
#include <experimental/expected.hpp>
#include <memory>
#include <tuple>
#include <vector>

using cxx20::expected;
using cxx20::unexpected;

namespace {
struct takes_init_and_variadic {
  std::vector<int> v;
  std::tuple<int, int> t;
  template <class... Args>
  takes_init_and_variadic(std::initializer_list<int> l, Args &&...args)
      : v(l), t(std::forward<Args>(args)...) {}
};
} // namespace

TEST(EmplaceTest, Emplace) {
  {
    expected<std::unique_ptr<int>, int> e;
    e.emplace(new int{42});
    EXPECT_TRUE(e);
    EXPECT_EQ(**e, 42);
  }

  {
    expected<std::vector<int>, int> e;
    e.emplace({0, 1});
    EXPECT_TRUE(e);
    EXPECT_EQ((*e)[0], 0);
    EXPECT_EQ((*e)[1], 1);
  }

  {
    expected<std::tuple<int, int>, int> e;
    e.emplace(2, 3);
    EXPECT_TRUE(e);
    EXPECT_EQ(std::get<0>(*e), 2);
    EXPECT_EQ(std::get<1>(*e), 3);
  }

  {
    expected<takes_init_and_variadic, int> e = unexpected(0);
    e.emplace({0, 1}, 2, 3);
    EXPECT_TRUE(e);
    EXPECT_EQ(e->v[0], 0);
    EXPECT_EQ(e->v[1], 1);
    EXPECT_EQ(std::get<0>(e->t), 2);
    EXPECT_EQ(std::get<1>(e->t), 3);
  }
}
