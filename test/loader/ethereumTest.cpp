// SPDX-License-Identifier: Apache-2.0
//===-- wasmedge/test/loader/ethereumTest.cpp - Ethereum related wasm tests ==//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contents unit tests of loading WASM about Ethereum.
///
//===----------------------------------------------------------------------===//

#include "ast/module.h"
#include "loader/filemgr.h"
#include "gtest/gtest.h"

namespace {

WasmEdge::FileMgrFStream Mgr;
WasmEdge::Configure Conf;

TEST(EthereumTest, Load__token) {
  WasmEdge::AST::Module Mod;
  ASSERT_TRUE(Mgr.setPath("ethereumTestData/token.wasm"));
  ASSERT_TRUE(Mod.loadBinary(Mgr, Conf));
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
