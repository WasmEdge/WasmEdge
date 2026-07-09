// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "common/statistics.h"
#include "common/enum_ast.hpp"

#include <cstdint>
#include <gtest/gtest.h>

namespace {
using WasmEdge::OpCode;
using WasmEdge::Statistics::Statistics;

// Statistics is the gas/cost-metering and instruction-counting helper used by
// the executor, VM, and C API. The cost-limit and counter logic is pure and
// deterministic, so it can be exercised directly without a running VM.

TEST(StatisticsTest, AddCostRespectsLimit) {
  Statistics S(100);
  EXPECT_EQ(S.getCostLimit(), UINT64_C(100));
  EXPECT_TRUE(S.addCost(10));
  EXPECT_EQ(S.getTotalCost(), UINT64_C(10));
  EXPECT_TRUE(S.addCost(90)); // reaches exactly the limit
  EXPECT_EQ(S.getTotalCost(), UINT64_C(100));
  EXPECT_FALSE(S.addCost(1)); // would exceed; rejected and sum unchanged
  EXPECT_EQ(S.getTotalCost(), UINT64_C(100));
}

TEST(StatisticsTest, SubCostGuardsUnderflow) {
  Statistics S;
  EXPECT_TRUE(S.addCost(10));
  EXPECT_TRUE(S.subCost(5));
  EXPECT_EQ(S.getTotalCost(), UINT64_C(5));
  // subCost rejects when the remaining sum is less than or equal to the cost,
  // so it can never drive the total to or below zero.
  EXPECT_FALSE(S.subCost(5));
  EXPECT_EQ(S.getTotalCost(), UINT64_C(5));
  EXPECT_FALSE(S.subCost(10));
  EXPECT_EQ(S.getTotalCost(), UINT64_C(5));
}

TEST(StatisticsTest, SetCostLimit) {
  Statistics S;
  S.setCostLimit(5);
  EXPECT_EQ(S.getCostLimit(), UINT64_C(5));
  EXPECT_TRUE(S.addCost(5));
  EXPECT_FALSE(S.addCost(1));
}

TEST(StatisticsTest, InstructionCounter) {
  Statistics S;
  EXPECT_EQ(S.getInstrCount(), UINT64_C(0));
  S.incInstrCount();
  S.incInstrCount();
  S.incInstrCount();
  EXPECT_EQ(S.getInstrCount(), UINT64_C(3));
}

TEST(StatisticsTest, DefaultCostTableChargesOnePerInstr) {
  Statistics S;
  EXPECT_TRUE(S.addInstrCost(OpCode::Block));
  EXPECT_TRUE(S.addInstrCost(OpCode::Br));
  EXPECT_EQ(S.getTotalCost(), UINT64_C(2)); // default table entry is 1
  // addInstrCost charges gas only; it does not bump the instruction counter.
  EXPECT_EQ(S.getInstrCount(), UINT64_C(0));
}

TEST(StatisticsTest, CustomCostTableAndSubInstrCost) {
  Statistics S;
  S.getCostTable()[static_cast<uint16_t>(OpCode::Block)] = 7;
  EXPECT_TRUE(S.addInstrCost(OpCode::Block));
  EXPECT_TRUE(S.addInstrCost(OpCode::Block));
  EXPECT_EQ(S.getTotalCost(), UINT64_C(14));
  EXPECT_TRUE(S.subInstrCost(OpCode::Block)); // 14 > 7
  EXPECT_EQ(S.getTotalCost(), UINT64_C(7));
  EXPECT_FALSE(S.subInstrCost(OpCode::Block)); // 7 <= 7 -> rejected
  EXPECT_EQ(S.getTotalCost(), UINT64_C(7));
}

TEST(StatisticsTest, ClearResetsCountersNotLimit) {
  Statistics S(1000);
  S.incInstrCount();
  EXPECT_TRUE(S.addCost(50));
  S.clear();
  EXPECT_EQ(S.getInstrCount(), UINT64_C(0));
  EXPECT_EQ(S.getTotalCost(), UINT64_C(0));
  EXPECT_EQ(S.getCostLimit(), UINT64_C(1000)); // clear does not reset the limit
}

} // namespace
