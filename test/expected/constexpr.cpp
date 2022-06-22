// SPDX-License-Identifier: CC0-1.0
#include <array>
#include <experimental/expected.hpp>
#include <gtest/gtest.h>

TEST(ConstexprTest, Constexpr) {
  EXPECT_EQ(([]() { return *cxx20::expected<int, int>(5); }()), 5);
  EXPECT_EQ(([]() { return cxx20::unexpected<int>(3).value(); }()), 3);
  EXPECT_EQ(([]() {
              return cxx20::expected<int, int>(cxx20::unexpect, 5).value_or(4);
            }()),
            4);
}
