//===-- ssvm/test/loader/expressionTest.cpp - AST expression unit tests ---===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contents unit tests of AST expression node.
///
//===----------------------------------------------------------------------===//

#include "loader/expression.h"
#include "filemgrTest.h"
#include "gtest/gtest.h"

namespace {

FileMgrTest Mgr;

TEST(ExpressionTest, LoadExpression) {
  /// 1. Test load limit.
  ///
  ///   1.  Load invalid empty expression.
  ///   2.  Load expression with only end operation.
  ///   3.  Load expression with invalid operations.
  ///   4.  Load expression with instructions.
  Mgr.clearBuffer();
  AST::Expression Exp1;
  EXPECT_FALSE(Exp1.loadBinary(Mgr));

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec2 = {
      0x0BU /// OpCode End.
  };
  Mgr.setVector(Vec2);
  AST::Expression Exp2;
  EXPECT_TRUE(Exp2.loadBinary(Mgr) && Mgr.getQueueSize() == 0);

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec3 = {
      0x45U, 0x46U, 0x47U, /// Valid OpCodes.
      0xEDU, 0xEEU, 0xEFU, /// Invalid OpCodes.
      0x0BU                /// OpCode End.
  };
  Mgr.setVector(Vec3);
  AST::Expression Exp3;
  EXPECT_FALSE(Exp3.loadBinary(Mgr));

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec4 = {
      0x45U, 0x46U, 0x47U, /// Valid OpCodes.
      0x0BU                /// OpCode End.
  };
  Mgr.setVector(Vec4);
  AST::Expression Exp4;
  EXPECT_TRUE(Exp4.loadBinary(Mgr) && Mgr.getQueueSize() == 0);
}

} // namespace