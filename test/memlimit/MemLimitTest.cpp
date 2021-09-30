// SPDX-License-Identifier: Apache-2.0

#include "common/configure.h"
#include "runtime/instance/memory.h"

#include "gtest/gtest.h"

namespace {

TEST(MemLimitTest, Limit__Pages) {
  using MemInst = WasmEdge::Runtime::Instance::MemoryInstance;
  WasmEdge::Configure Conf;
  Conf.getRuntimeConfigure().setMaxMemoryPage(256);

  WasmEdge::AST::Limit Lim1(257);
  MemInst Inst1(Lim1, Conf.getRuntimeConfigure().getMaxMemoryPage());
  ASSERT_TRUE(Inst1.getDataPtr() == nullptr);

  MemInst Inst2(Lim1);
  ASSERT_FALSE(Inst2.getDataPtr() == nullptr);

  WasmEdge::AST::Limit Lim2(1);
  MemInst Inst3(Lim2, Conf.getRuntimeConfigure().getMaxMemoryPage());
  ASSERT_FALSE(Inst3.getDataPtr() == nullptr);
  ASSERT_FALSE(Inst3.growPage(256));
  ASSERT_TRUE(Inst3.growPage(255));

  MemInst Inst4(Lim2);
  ASSERT_FALSE(Inst4.getDataPtr() == nullptr);
  ASSERT_TRUE(Inst4.growPage(256));

  WasmEdge::AST::Limit Lim3(1, 128);
  MemInst Inst5(Lim3, Conf.getRuntimeConfigure().getMaxMemoryPage());
  ASSERT_FALSE(Inst5.getDataPtr() == nullptr);
  ASSERT_FALSE(Inst5.growPage(128));
  ASSERT_TRUE(Inst5.growPage(127));
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  WasmEdge::Log::setErrorLoggingLevel();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
