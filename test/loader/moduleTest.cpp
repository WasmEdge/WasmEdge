//===-- ssvm/test/loader/moduleTest.cpp - AST module unit tests -----------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contents unit tests of AST module node and the main function.
///
//===----------------------------------------------------------------------===//

#include "loader/module.h"
#include "filemgrTest.h"
#include "gtest/gtest.h"

namespace {

FileMgrTest Mgr;

TEST(ModuleTest, LoadInvalidModule) {
  /// 1. Test load empty file
  AST::Module Mod;
  EXPECT_FALSE(Mod.loadBinary(Mgr));
}

TEST(ModuleTest, LoadEmptyModule) {
  /// 2. Test load empty module
  AST::Module Mod;
  std::vector<unsigned char> Vec = {0x00U, 0x61U, 0x73U, 0x6DU,
                                    0x01U, 0x00U, 0x00U, 0x00U};
  Mgr.setVector(Vec);
  EXPECT_TRUE(Mod.loadBinary(Mgr));
}

TEST(ModuleTest, LoadValidSecModule) {
  /// 3. Test load module with valid empty sections
  AST::Module Mod;
  std::vector<unsigned char> Vec = {
      0x00U, 0x61U, 0x73U, 0x6DU, /// Magic
      0x01U, 0x00U, 0x00U, 0x00U, /// Version
      0x00U, 0x00U,               /// Custom section
      0x01U, 0x01U, 0x00U,        /// Type section
      0x02U, 0x01U, 0x00U,        /// Import section
      0x03U, 0x01U, 0x00U,        /// Function section
      0x04U, 0x01U, 0x00U,        /// Table section
      0x05U, 0x01U, 0x00U,        /// Memory section
      0x06U, 0x01U, 0x00U,        /// Global section
      0x07U, 0x01U, 0x00U,        /// Export section
      0x08U, 0x01U, 0x00U,        /// Start section
      0x09U, 0x01U, 0x00U,        /// Element section
      0x0AU, 0x01U, 0x00U,        /// Code section
      0x0BU, 0x01U, 0x00U         /// Data section
  };
  Mgr.setVector(Vec);
  EXPECT_TRUE(Mod.loadBinary(Mgr));
}

TEST(ModuleTest, LoadInvalidSecModule) {
  /// 4. Test load module with invalid sections
  AST::Module Mod;
  std::vector<unsigned char> Vec = {
      0x00U, 0x61U, 0x73U, 0x6DU, /// Magic
      0x01U, 0x00U, 0x00U, 0x00U, /// Version
      0x00U, 0x00U,               /// Custom section
      0x01U, 0x01U, 0x00U,        /// Type section
      0x02U, 0x01U, 0x00U,        /// Import section
      0x03U, 0x01U, 0x00U,        /// Function section
      0x04U, 0x01U, 0x00U,        /// Table section
      0x05U, 0x01U, 0x00U,        /// Memory section
      0x06U, 0x01U, 0x00U,        /// Global section
      0x07U, 0x01U, 0x00U,        /// Export section
      0x08U, 0x01U, 0x00U,        /// Start section
      0x09U, 0x01U, 0x00U,        /// Element section
      0x0AU, 0x01U, 0x00U,        /// Code section
      0x0BU, 0x01U, 0x00U,        /// Data section
      0x0CU, 0x01U, 0x00U         /// Invalid section
  };
  Mgr.setVector(Vec);
  EXPECT_FALSE(Mod.loadBinary(Mgr));
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}