// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "common/spare_enum_map.h"

#include <cstdint>
#include <gtest/gtest.h>
#include <iterator>
#include <string_view>
#include <utility>

namespace {
using namespace std::literals;

enum class Op : uint16_t {
  Nop = 0x01,
  Eq = 0x46,
  Add = 0x6a,
  Sub = 0x6b,
  End = 0xff,
};
constexpr std::pair<Op, std::string_view> OpArr[] = {{Op::End, "end"sv},
                                                     {Op::Add, "add"sv},
                                                     {Op::Nop, "nop"sv},
                                                     {Op::Sub, "sub"sv},
                                                     {Op::Eq, "eq"sv}};
constexpr WasmEdge::SpareEnumMap OpMap(OpArr);

static_assert(OpMap.end() - OpMap.begin() == 5,
              "SpareEnumMap end() - begin() must equal Size");

TEST(SpareEnumMapTest, FindPresentKey) {
  auto It = OpMap.find(Op::Add);
  ASSERT_NE(It, OpMap.end());
  EXPECT_EQ(It->second, "add"sv);
  EXPECT_EQ(OpMap.find(Op::Nop)->second, "nop"sv);
  EXPECT_EQ(OpMap.find(Op::End)->second, "end"sv);
}

TEST(SpareEnumMapTest, FindAbsentKeyReturnsEnd) {
  EXPECT_EQ(OpMap.find(static_cast<Op>(0x00)), OpMap.end());
  EXPECT_EQ(OpMap.find(static_cast<Op>(0x47)), OpMap.end());
  EXPECT_EQ(OpMap.find(static_cast<Op>(0xfe)), OpMap.end());
}

TEST(SpareEnumMapTest, SubscriptByKey) {
  EXPECT_EQ(OpMap[Op::Nop], "nop"sv);
  EXPECT_EQ(OpMap[Op::Eq], "eq"sv);
  EXPECT_EQ(OpMap[Op::Sub], "sub"sv);
}

TEST(SpareEnumMapTest, SortedRegardlessOfInputOrder) {
  const std::pair<Op, std::string_view> Expected[] = {{Op::Nop, "nop"sv},
                                                      {Op::Eq, "eq"sv},
                                                      {Op::Add, "add"sv},
                                                      {Op::Sub, "sub"sv},
                                                      {Op::End, "end"sv}};
  std::size_t I = 0;
  for (auto It = OpMap.begin(); It != OpMap.end(); ++It, ++I) {
    ASSERT_LT(I, 5u);
    EXPECT_EQ(It->first, Expected[I].first);
    EXPECT_EQ(It->second, Expected[I].second);
  }
  EXPECT_EQ(I, 5u);
  EXPECT_EQ(std::distance(OpMap.begin(), OpMap.end()), 5);
}

} // namespace
