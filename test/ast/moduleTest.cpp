// SPDX-License-Identifier: Apache-2.0
//===-- wasmedge/test/ast/moduleTest.cpp - AST module unit tests ----------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contents unit tests of AST module node and the main function.
///
//===----------------------------------------------------------------------===//

#include "ast/module.h"
#include "loader/filemgr.h"
#include "gtest/gtest.h"

namespace {

WasmEdge::FileMgr Mgr;
WasmEdge::Configure Conf;

TEST(ModuleTest, LoadModule) {
  /// 1. Test load empty file
  Mgr.setCode(std::vector<uint8_t>());
  WasmEdge::AST::Module Mod1;
  EXPECT_FALSE(Mod1.loadBinary(Mgr, Conf));

  /// 2. Test load empty module
  WasmEdge::AST::Module Mod2;
  std::vector<unsigned char> Vec2 = {0x00U, 0x61U, 0x73U, 0x6DU,
                                     0x01U, 0x00U, 0x00U, 0x00U};
  Mgr.setCode(Vec2);
  EXPECT_TRUE(Mod2.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);

  /// 3. Test load module with valid empty sections
  WasmEdge::AST::Module Mod3;
  std::vector<unsigned char> Vec3 = {
      0x00U, 0x61U, 0x73U, 0x6DU,                      /// Magic
      0x01U, 0x00U, 0x00U, 0x00U,                      /// Version
      0x00U, 0x81U, 0x80U, 0x80U, 0x80U, 0x00U, 0x00U, /// Custom section
      0x01U, 0x81U, 0x80U, 0x80U, 0x80U, 0x00U, 0x00U, /// Type section
      0x02U, 0x81U, 0x80U, 0x80U, 0x80U, 0x00U, 0x00U, /// Import section
      0x03U, 0x81U, 0x80U, 0x80U, 0x80U, 0x00U, 0x00U, /// Function section
      0x04U, 0x81U, 0x80U, 0x80U, 0x80U, 0x00U, 0x00U, /// Table section
      0x05U, 0x81U, 0x80U, 0x80U, 0x80U, 0x00U, 0x00U, /// Memory section
      0x06U, 0x81U, 0x80U, 0x80U, 0x80U, 0x00U, 0x00U, /// Global section
      0x07U, 0x81U, 0x80U, 0x80U, 0x80U, 0x00U, 0x00U, /// Export section
      0x08U, 0x81U, 0x80U, 0x80U, 0x80U, 0x00U, 0x00U, /// Start section
      0x09U, 0x81U, 0x80U, 0x80U, 0x80U, 0x00U, 0x00U, /// Element section
      0x0AU, 0x81U, 0x80U, 0x80U, 0x80U, 0x00U, 0x00U, /// Code section
      0x0BU, 0x81U, 0x80U, 0x80U, 0x80U, 0x00U, 0x00U  /// Data section
  };
  Mgr.setCode(Vec3);
  EXPECT_TRUE(Mod3.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);

  /// 4. Test load module with invalid sections
  WasmEdge::AST::Module Mod4;
  std::vector<unsigned char> Vec4 = {
      0x00U, 0x61U, 0x73U, 0x6DU,                      /// Magic
      0x01U, 0x00U, 0x00U, 0x00U,                      /// Version
      0x00U, 0x81U, 0x80U, 0x80U, 0x80U, 0x00U, 0x00U, /// Custom section
      0x01U, 0x81U, 0x80U, 0x80U, 0x80U, 0x00U, 0x00U, /// Type section
      0x02U, 0x81U, 0x80U, 0x80U, 0x80U, 0x00U, 0x00U, /// Import section
      0x03U, 0x81U, 0x80U, 0x80U, 0x80U, 0x00U, 0x00U, /// Function section
      0x04U, 0x81U, 0x80U, 0x80U, 0x80U, 0x00U, 0x00U, /// Table section
      0x05U, 0x81U, 0x80U, 0x80U, 0x80U, 0x00U, 0x00U, /// Memory section
      0x06U, 0x81U, 0x80U, 0x80U, 0x80U, 0x00U, 0x00U, /// Global section
      0x07U, 0x81U, 0x80U, 0x80U, 0x80U, 0x00U, 0x00U, /// Export section
      0x08U, 0x81U, 0x80U, 0x80U, 0x80U, 0x00U, 0x00U, /// Start section
      0x09U, 0x81U, 0x80U, 0x80U, 0x80U, 0x00U, 0x00U, /// Element section
      0x0AU, 0x81U, 0x80U, 0x80U, 0x80U, 0x00U, 0x00U, /// Code section
      0x0BU, 0x81U, 0x80U, 0x80U, 0x80U, 0x00U, 0x00U, /// Data section
      0x0DU, 0x81U, 0x80U, 0x80U, 0x80U, 0x00U, 0x00U  /// Invalid section
  };
  Mgr.setCode(Vec4);
  EXPECT_FALSE(Mod4.loadBinary(Mgr, Conf));
}

TEST(ModuleTest, LoadDataCountSecModule) {
  /// 5. Test load module with invalid datacount section without proposals.
  WasmEdge::AST::Module Mod5;
  std::vector<unsigned char> Vec5 = {
      0x00U, 0x61U, 0x73U, 0x6DU,       /// Magic
      0x01U, 0x00U, 0x00U, 0x00U,       /// Version
      0x0CU, 0x01U, 0x01U,              /// DataCount section
      0x0BU, 0x03U, 0x01U, 0x01U, 0x00U /// Data section
  };
  Mgr.setCode(Vec5);
  Conf.removeProposal(WasmEdge::Proposal::BulkMemoryOperations);
  Conf.removeProposal(WasmEdge::Proposal::ReferenceTypes);
  EXPECT_FALSE(Mod5.loadBinary(Mgr, Conf));
  Conf.addProposal(WasmEdge::Proposal::BulkMemoryOperations);
  Conf.addProposal(WasmEdge::Proposal::ReferenceTypes);

  /// 6. Test load module with invalid datacount section.
  WasmEdge::AST::Module Mod6;
  std::vector<unsigned char> Vec6 = {
      0x00U, 0x61U, 0x73U, 0x6DU, /// Magic
      0x01U, 0x00U, 0x00U, 0x00U, /// Version
      0x0CU, 0x00U                /// DataCount section, miss data count
  };
  Mgr.setCode(Vec6);
  EXPECT_FALSE(Mod6.loadBinary(Mgr, Conf));
}

TEST(ModuleTest, LoadStartSecModule) {
  /// 7. Test load module with invalid start section.
  WasmEdge::AST::Module Mod;
  std::vector<unsigned char> Vec = {
      0x00U, 0x61U, 0x73U, 0x6DU, /// Magic
      0x01U, 0x00U, 0x00U, 0x00U, /// Version
      0x08U, 0x00U                /// Start section, miss index
  };
  Mgr.setCode(Vec);
  EXPECT_FALSE(Mod.loadBinary(Mgr, Conf));
}

TEST(ModuleTest, LoadDupSecModule) {
  /// 8. Test load module with duplicated type section.
  WasmEdge::AST::Module Mod1;
  std::vector<unsigned char> Vec1 = {
      0x00U, 0x61U, 0x73U, 0x6DU, /// Magic
      0x01U, 0x00U, 0x00U, 0x00U, /// Version
      0x01U, 0x01U, 0x00U,        /// Type section
      0x01U, 0x01U, 0x00U         /// Type section duplicated
  };
  Mgr.setCode(Vec1);
  EXPECT_FALSE(Mod1.loadBinary(Mgr, Conf));

  /// 9. Test load module with duplicated import section.
  WasmEdge::AST::Module Mod2;
  std::vector<unsigned char> Vec2 = {
      0x00U, 0x61U, 0x73U, 0x6DU, /// Magic
      0x01U, 0x00U, 0x00U, 0x00U, /// Version
      0x02U, 0x01U, 0x00U,        /// Import section
      0x02U, 0x01U, 0x00U         /// Import section duplicated
  };
  Mgr.setCode(Vec2);
  EXPECT_FALSE(Mod2.loadBinary(Mgr, Conf));

  /// 10. Test load module with duplicated function section.
  WasmEdge::AST::Module Mod3;
  std::vector<unsigned char> Vec3 = {
      0x00U, 0x61U, 0x73U, 0x6DU, /// Magic
      0x01U, 0x00U, 0x00U, 0x00U, /// Version
      0x03U, 0x01U, 0x00U,        /// Function section
      0x03U, 0x01U, 0x00U         /// Function section duplicated
  };
  Mgr.setCode(Vec3);
  EXPECT_FALSE(Mod3.loadBinary(Mgr, Conf));

  /// 11. Test load module with duplicated table section.
  WasmEdge::AST::Module Mod4;
  std::vector<unsigned char> Vec4 = {
      0x00U, 0x61U, 0x73U, 0x6DU, /// Magic
      0x01U, 0x00U, 0x00U, 0x00U, /// Version
      0x04U, 0x01U, 0x00U,        /// Table section
      0x04U, 0x01U, 0x00U         /// Table section duplicated
  };
  Mgr.setCode(Vec4);
  EXPECT_FALSE(Mod4.loadBinary(Mgr, Conf));

  /// 12. Test load module with duplicated memory section.
  WasmEdge::AST::Module Mod5;
  std::vector<unsigned char> Vec5 = {
      0x00U, 0x61U, 0x73U, 0x6DU, /// Magic
      0x01U, 0x00U, 0x00U, 0x00U, /// Version
      0x05U, 0x01U, 0x00U,        /// Memory section
      0x05U, 0x01U, 0x00U         /// Memory section duplicated
  };
  Mgr.setCode(Vec5);
  EXPECT_FALSE(Mod5.loadBinary(Mgr, Conf));

  /// 13. Test load module with duplicated global section.
  WasmEdge::AST::Module Mod6;
  std::vector<unsigned char> Vec6 = {
      0x00U, 0x61U, 0x73U, 0x6DU, /// Magic
      0x01U, 0x00U, 0x00U, 0x00U, /// Version
      0x06U, 0x01U, 0x00U,        /// Global section
      0x06U, 0x01U, 0x00U         /// Global section duplicated
  };
  Mgr.setCode(Vec6);
  EXPECT_FALSE(Mod6.loadBinary(Mgr, Conf));

  /// 14. Test load module with duplicated export section.
  WasmEdge::AST::Module Mod7;
  std::vector<unsigned char> Vec7 = {
      0x00U, 0x61U, 0x73U, 0x6DU, /// Magic
      0x01U, 0x00U, 0x00U, 0x00U, /// Version
      0x07U, 0x01U, 0x00U,        /// Export section
      0x07U, 0x01U, 0x00U         /// Export section duplicated
  };
  Mgr.setCode(Vec7);
  EXPECT_FALSE(Mod7.loadBinary(Mgr, Conf));

  /// 15. Test load module with duplicated start section.
  WasmEdge::AST::Module Mod8;
  std::vector<unsigned char> Vec8 = {
      0x00U, 0x61U, 0x73U, 0x6DU, /// Magic
      0x01U, 0x00U, 0x00U, 0x00U, /// Version
      0x08U, 0x01U, 0x00U,        /// Start section
      0x08U, 0x01U, 0x00U         /// Start section duplicated
  };
  Mgr.setCode(Vec8);
  EXPECT_FALSE(Mod8.loadBinary(Mgr, Conf));

  /// 16. Test load module with duplicated element section.
  WasmEdge::AST::Module Mod9;
  std::vector<unsigned char> Vec9 = {
      0x00U, 0x61U, 0x73U, 0x6DU, /// Magic
      0x01U, 0x00U, 0x00U, 0x00U, /// Version
      0x09U, 0x01U, 0x00U,        /// Element section
      0x09U, 0x01U, 0x00U         /// Element section duplicated
  };
  Mgr.setCode(Vec9);
  EXPECT_FALSE(Mod9.loadBinary(Mgr, Conf));

  /// 17. Test load module with duplicated code section.
  WasmEdge::AST::Module ModA;
  std::vector<unsigned char> VecA = {
      0x00U, 0x61U, 0x73U, 0x6DU, /// Magic
      0x01U, 0x00U, 0x00U, 0x00U, /// Version
      0x0AU, 0x01U, 0x00U,        /// Code section
      0x0AU, 0x01U, 0x00U         /// Code section duplicated
  };
  Mgr.setCode(VecA);
  EXPECT_FALSE(ModA.loadBinary(Mgr, Conf));

  /// 18. Test load module with duplicated data section.
  WasmEdge::AST::Module ModB;
  std::vector<unsigned char> VecB = {
      0x00U, 0x61U, 0x73U, 0x6DU, /// Magic
      0x01U, 0x00U, 0x00U, 0x00U, /// Version
      0x0BU, 0x01U, 0x00U,        /// Data section
      0x0BU, 0x01U, 0x00U         /// Data section duplicated
  };
  Mgr.setCode(VecB);
  EXPECT_FALSE(ModB.loadBinary(Mgr, Conf));

  /// 19. Test load module with duplicated datacount section.
  WasmEdge::AST::Module ModC;
  std::vector<unsigned char> VecC = {
      0x00U, 0x61U, 0x73U, 0x6DU, /// Magic
      0x01U, 0x00U, 0x00U, 0x00U, /// Version
      0x0CU, 0x01U, 0x00U,        /// Datacount section
      0x0CU, 0x01U, 0x00U         /// Datacount section duplicated
  };
  Mgr.setCode(VecC);
  EXPECT_FALSE(ModC.loadBinary(Mgr, Conf));
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
