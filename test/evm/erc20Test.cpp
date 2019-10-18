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

#include "vm/configure.h"
#include "vm/environment.h"
#include "vm/result.h"
#include "vm/vm.h"
#include "gtest/gtest.h"

#include <iostream>

namespace {

const std::string Erc20Path("ethereumTestData/erc20.wasm");

TEST(ERC20Test, Run__mint) {}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}