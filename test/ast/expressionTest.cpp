// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/test/ast/expressionTest.cpp - AST expression unit tests ------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contents unit tests of AST expression node.
///
//===----------------------------------------------------------------------===//

#include "ast/expression.h"
#include "loader/filemgr.h"
#include "gtest/gtest.h"

namespace {

SSVM::FileMgrVector Mgr;
SSVM::ProposalConfigure PConf;

TEST(ExpressionTest, LoadExpression) {
  /// 1. Test load limit.
  ///
  ///   1.  Load invalid empty expression.
  ///   2.  Load expression with only end operation.
  ///   3.  Load expression with invalid operations.
  ///   4.  Load expression with instructions.
  Mgr.clearBuffer();
  SSVM::AST::Expression Exp1;
  EXPECT_FALSE(Exp1.loadBinary(Mgr, PConf));

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec2 = {
      0x0BU /// OpCode End.
  };
  Mgr.setCode(Vec2);
  SSVM::AST::Expression Exp2;
  EXPECT_TRUE(Exp2.loadBinary(Mgr, PConf) && Mgr.getRemainSize() == 0);

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec3 = {
      0x45U, 0x46U, 0x47U, /// Valid OpCodes.
      0xEDU, 0xEEU, 0xEFU, /// Invalid OpCodes.
      0x0BU                /// OpCode End.
  };
  Mgr.setCode(Vec3);
  SSVM::AST::Expression Exp3;
  EXPECT_FALSE(Exp3.loadBinary(Mgr, PConf));

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec4 = {
      0x45U, 0x46U, 0x47U, /// Valid OpCodes.
      0x0BU                /// OpCode End.
  };
  Mgr.setCode(Vec4);
  SSVM::AST::Expression Exp4;
  EXPECT_TRUE(Exp4.loadBinary(Mgr, PConf) && Mgr.getRemainSize() == 0);
}

} // namespace
