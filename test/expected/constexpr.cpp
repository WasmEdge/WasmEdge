// SPDX-License-Identifier: CC0-1.0
#include "gtest/gtest.h"
#include <array>
#include <experimental/expected.hpp>
using std::experimental::expected;
using std::experimental::unexpect;
using std::experimental::unexpected;

TEST(ConstexprTest, Constexpr) {
  EXPECT_EQ(([]() { return *expected<int, int>(5); }()), 5);
  EXPECT_EQ(([]() { return unexpected<int>(3).value(); }()), 3);
  EXPECT_EQ(([]() { return expected<int, int>(unexpect, 5).value_or(4); }()),
            4);
}
