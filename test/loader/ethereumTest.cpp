//===-- ssvm/test/loader/ethereumTest.cpp - Ethereum related wasm tests ---===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contents unit tests of loading WASM about Ethereum.
///
//===----------------------------------------------------------------------===//

#include "loader/filemgr.h"
#include "loader/module.h"
#include "gtest/gtest.h"

namespace {

AST::FileMgrFStream Mgr;

TEST(WagonTest, Load__token) {
  Mgr.setPath("wagonTestData/token.wasm");
  AST::Module Mod;
  EXPECT_TRUE(Mod.loadBinary(Mgr));
}
TEST(WagonTest, Load__wrc20) {
  Mgr.setPath("wagonTestData/wrc20.wasm");
  AST::Module Mod;
  EXPECT_TRUE(Mod.loadBinary(Mgr));
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}