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

// SpareEnumMap backs the sparse enum->string tables in the codebase (e.g. the
// opcode mnemonics in enum_ast.hpp and the error-code strings in
// enum_errcode.hpp). Unlike DenseEnumMap it sorts its entries at construction
// (an insertion sort) and looks them up with std::lower_bound, so the entries
// are deliberately given here out of key order to exercise the sort.
constexpr std::pair<Op, std::string_view> OpArr[] = {{Op::End, "end"sv},
                                                     {Op::Add, "add"sv},
                                                     {Op::Nop, "nop"sv},
                                                     {Op::Sub, "sub"sv},
                                                     {Op::Eq, "eq"sv}};
constexpr WasmEdge::SpareEnumMap OpMap(OpArr);

// Structural invariant verified at compile time: the map exposes Size entries.
// The backing array has Size + 1 slots, so (unlike DenseEnumMap) comparing
// against end() never reads out of bounds. find() lookups are checked at
// runtime below because find() uses std::lower_bound, which is not constexpr in
// C++17.
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

// The constructor must produce ascending key order regardless of input order,
// because find() relies on a binary search over the sorted storage.
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
