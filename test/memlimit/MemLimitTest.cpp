// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/configure.h"
#include "runtime/instance/memory.h"

#include <gtest/gtest.h>

namespace {

TEST(MemLimitTest, Limit__Pages) {
  using MemInst = WasmEdge::Runtime::Instance::MemoryInstance;
  WasmEdge::Configure Conf;
  Conf.getRuntimeConfigure().setMaxMemoryPage(256);

  MemInst Inst1(WasmEdge::AST::MemoryType(257),
                Conf.getRuntimeConfigure().getMaxMemoryPage());
  ASSERT_FALSE(Inst1.getDataPtr() == nullptr);
  EXPECT_EQ(Inst1.getPageSize(), 256U);

  MemInst Inst2(WasmEdge::AST::MemoryType(257));
  ASSERT_FALSE(Inst2.getDataPtr() == nullptr);

  MemInst Inst3(WasmEdge::AST::MemoryType(1),
                Conf.getRuntimeConfigure().getMaxMemoryPage());
  ASSERT_FALSE(Inst3.getDataPtr() == nullptr);
  ASSERT_FALSE(Inst3.growPage(256));
  ASSERT_TRUE(Inst3.growPage(255));

  MemInst Inst4(WasmEdge::AST::MemoryType(1));
  ASSERT_FALSE(Inst4.getDataPtr() == nullptr);
  ASSERT_TRUE(Inst4.growPage(256));

  MemInst Inst5(WasmEdge::AST::MemoryType(1, 128),
                Conf.getRuntimeConfigure().getMaxMemoryPage());
  ASSERT_FALSE(Inst5.getDataPtr() == nullptr);
  ASSERT_FALSE(Inst5.growPage(128));
  ASSERT_TRUE(Inst5.growPage(127));

  MemInst Inst6(WasmEdge::AST::MemoryType(1),
                Conf.getRuntimeConfigure().getMaxMemoryPage());
  ASSERT_FALSE(Inst6.growPage(0xFFFFFFFF));
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  WasmEdge::Log::setErrorLoggingLevel();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
