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

#include "ast/module.h"
#include "loader/filemgr.h"
#include "gtest/gtest.h"

namespace {

FileMgrFStream Mgr;
Loader::ErrCode SuccessCode = Loader::ErrCode::Success;

TEST(WagonTest, Load__token) {
  Mgr.setPath("ethereumTestData/token.wasm");
  AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}
TEST(WagonTest, Load__wrc20) {
  Mgr.setPath("ethereumTestData/wrc20.wasm");
  AST::Module Mod;
  EXPECT_EQ(Mod.loadBinary(Mgr), SuccessCode);
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}