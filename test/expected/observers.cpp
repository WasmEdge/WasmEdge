// SPDX-License-Identifier: CC0-1.0
#include <experimental/expected.hpp>
#include <gtest/gtest.h>
#include <type_traits>
#include <utility>

using cxx20::bad_expected_access;
using cxx20::expected;
using cxx20::unexpect;

struct move_detector {
  move_detector() = default;
  move_detector(move_detector &&rhs) { rhs.been_moved = true; }
  bool been_moved = false;
};

TEST(ObserversTest, Observers) {
  expected<int, int> o1 = 42;
  expected<int, int> o2{unexpect, 0};
  const expected<int, int> o3 = 42;

  EXPECT_EQ(*o1, 42);
  EXPECT_EQ(*o1, o1.value());
  EXPECT_EQ(o2.value_or(42), 42);
  EXPECT_EQ(o2.error(), 0);
  EXPECT_EQ(o3.value(), 42);
  EXPECT_THROW(o2.value(), bad_expected_access<int>);
  EXPECT_TRUE((std::is_same_v<decltype(o1.value()), int &>));
  EXPECT_TRUE((std::is_same_v<decltype(o3.value()), const int &>));
  EXPECT_TRUE((std::is_same_v<decltype(std::move(o1).value()), int &&>));
  EXPECT_TRUE((std::is_same_v<decltype(std::move(o3).value()), const int &&>));

  expected<move_detector, int> o4{std::in_place};
  move_detector o5 = std::move(o4).value();
  EXPECT_TRUE(o4->been_moved);
  EXPECT_FALSE(o5.been_moved);
}
