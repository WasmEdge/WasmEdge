// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "common/statistics.h"
#include "common/enum_ast.hpp"

#include <cstdint>
#include <gtest/gtest.h>

namespace {
using WasmEdge::OpCode;
using WasmEdge::Statistics::Statistics;

TEST(StatisticsTest, AddCostRespectsLimit) {
  Statistics S(100);
  EXPECT_EQ(S.getCostLimit(), UINT64_C(100));
  EXPECT_TRUE(S.addCost(10));
  EXPECT_EQ(S.getTotalCost(), UINT64_C(10));
  EXPECT_TRUE(S.addCost(90));
  EXPECT_EQ(S.getTotalCost(), UINT64_C(100));
  EXPECT_FALSE(S.addCost(1));
  EXPECT_EQ(S.getTotalCost(), UINT64_C(100));
}

TEST(StatisticsTest, SubCostRejectsCostAtOrAboveTotal) {
  Statistics S;
  EXPECT_TRUE(S.addCost(10));
  EXPECT_TRUE(S.subCost(5));
  EXPECT_EQ(S.getTotalCost(), UINT64_C(5));
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
  EXPECT_EQ(S.getTotalCost(), UINT64_C(2));
  EXPECT_EQ(S.getInstrCount(), UINT64_C(0));
}

TEST(StatisticsTest, CustomCostTableAndSubInstrCost) {
  Statistics S;
  S.getCostTable()[static_cast<uint16_t>(OpCode::Block)] = 7;
  EXPECT_TRUE(S.addInstrCost(OpCode::Block));
  EXPECT_TRUE(S.addInstrCost(OpCode::Block));
  EXPECT_EQ(S.getTotalCost(), UINT64_C(14));
  EXPECT_TRUE(S.subInstrCost(OpCode::Block));
  EXPECT_EQ(S.getTotalCost(), UINT64_C(7));
  EXPECT_FALSE(S.subInstrCost(OpCode::Block));
  EXPECT_EQ(S.getTotalCost(), UINT64_C(7));
}

TEST(StatisticsTest, ClearResetsCountersNotLimit) {
  Statistics S(1000);
  S.incInstrCount();
  EXPECT_TRUE(S.addCost(50));
  S.clear();
  EXPECT_EQ(S.getInstrCount(), UINT64_C(0));
  EXPECT_EQ(S.getTotalCost(), UINT64_C(0));
  EXPECT_EQ(S.getCostLimit(), UINT64_C(1000));
}

} // namespace
