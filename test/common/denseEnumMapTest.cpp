// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/dense_enum_map.h"

#include <cstdint>
#include <gtest/gtest.h>
#include <iterator>
#include <string_view>
#include <utility>

namespace {
using namespace std::literals;

enum class Color : uint8_t { Red = 0, Green = 1, Blue = 2 };

// A small, contiguous enum map mirroring how the codebase builds its
// compile-time enum->string tables (e.g. ProposalStr in enum_configure.hpp).
constexpr std::pair<Color, std::string_view> ColorArr[] = {
    {Color::Red, "red"sv}, {Color::Green, "green"sv}, {Color::Blue, "blue"sv}};
constexpr WasmEdge::DenseEnumMap ColorMap(ColorArr);

// Compile-time guard: if operator- dereferences past the array again,
// these constexpr distance checks fail at build time.
static_assert(ColorMap.end() - ColorMap.begin() == 3,
              "DenseEnumMap end() - begin() must equal Size");
static_assert(ColorMap.find(Color::Green) - ColorMap.begin() == 1,
              "DenseEnumMap find() must be reachable from begin()");
static_assert(ColorMap.begin() != ColorMap.end(),
              "non-empty DenseEnumMap: begin() != end()");
static_assert(ColorMap.find(static_cast<Color>(3)) == ColorMap.end(),
              "out-of-range key must compare equal to end()");

// Iterator distance against end() must not access out-of-bounds memory. Under a
// hardened libstdc++ (_GLIBCXX_ASSERTIONS, the default on Fedora) the buggy
// implementation aborts here; with the fix it simply returns Size.
TEST(DenseEnumMapTest, DistanceToEnd) {
  EXPECT_EQ(ColorMap.end() - ColorMap.begin(), 3);
  EXPECT_EQ(std::distance(ColorMap.begin(), ColorMap.end()), 3);
}

// The real-world usage pattern from lib/common/errinfo.cpp: look up a key and
// guard the result against end() before dereferencing.
TEST(DenseEnumMapTest, FindPresentKey) {
  auto It = ColorMap.find(Color::Green);
  ASSERT_NE(It, ColorMap.end());
  EXPECT_EQ(It->second, "green"sv);
  EXPECT_EQ(ColorMap.find(Color::Red)->second, "red"sv);
  EXPECT_EQ(ColorMap.find(Color::Blue)->second, "blue"sv);
}

// A key outside the dense index range clamps to end().
TEST(DenseEnumMapTest, FindAbsentKey) {
  EXPECT_EQ(ColorMap.find(static_cast<Color>(3)), ColorMap.end());
  EXPECT_EQ(ColorMap.find(static_cast<Color>(99)), ColorMap.end());
}

// operator[] returns the stored value directly by key.
TEST(DenseEnumMapTest, SubscriptByKey) {
  EXPECT_EQ(ColorMap[Color::Red], "red"sv);
  EXPECT_EQ(ColorMap[Color::Green], "green"sv);
  EXPECT_EQ(ColorMap[Color::Blue], "blue"sv);
}

// Full forward traversal via begin()/end() and operator!=, exactly as a
// range-for would drive it.
TEST(DenseEnumMapTest, ForwardIteration) {
  std::string_view Expected[] = {"red"sv, "green"sv, "blue"sv};
  std::size_t I = 0;
  for (auto It = ColorMap.begin(); It != ColorMap.end(); ++It, ++I) {
    ASSERT_LT(I, 3u);
    EXPECT_EQ(It->second, Expected[I]);
  }
  EXPECT_EQ(I, 3u);
}

// Ordering operators are all defined in terms of operator-, so they must also
// stay valid when compared against end().
TEST(DenseEnumMapTest, OrderingAgainstEnd) {
  EXPECT_TRUE(ColorMap.begin() < ColorMap.end());
  EXPECT_TRUE(ColorMap.end() > ColorMap.begin());
  EXPECT_TRUE(ColorMap.begin() <= ColorMap.end());
  EXPECT_TRUE(ColorMap.end() >= ColorMap.begin());
  EXPECT_FALSE(ColorMap.end() < ColorMap.begin());
}

// Random-access arithmetic should land on end() without dereferencing past it.
TEST(DenseEnumMapTest, RandomAccessArithmetic) {
  auto It = ColorMap.begin() + 3;
  EXPECT_EQ(It, ColorMap.end());
  EXPECT_EQ(It - ColorMap.begin(), 3);
  // Note: ConstIterator provides operator+(iter, diff) but not
  // operator-(iter, diff), so step back via operator+ with a negative offset.
  auto Back = It + (-1);
  EXPECT_EQ(Back->second, "blue"sv);
}

} // namespace
