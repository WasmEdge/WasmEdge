// SPDX-License-Identifier: Apache-2.0
//===-- wasmedge/test/ast/expressionTest.cpp - AST expression unit tests---===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contents unit tests of AST expression node.
///
//===----------------------------------------------------------------------===//

#include "ast/expression.h"

#include "gtest/gtest.h"

namespace {

WasmEdge::FileMgr Mgr;
WasmEdge::Configure Conf;

TEST(ExpressionTest, LoadExpression) {
  /// 1. Test load limit.
  ///
  ///   1.  Load invalid empty expression.
  ///   2.  Load expression with only end operation.
  ///   3.  Load expression with invalid operations.
  ///   4.  Load expression with instructions.
  ///   5.  Load expression with instructions not in proposals.
  Mgr.setCode(std::vector<uint8_t>());
  WasmEdge::AST::Expression Exp1;
  EXPECT_FALSE(Exp1.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec2 = {
      0x0BU /// OpCode End.
  };
  Mgr.setCode(Vec2);
  WasmEdge::AST::Expression Exp2;
  EXPECT_TRUE(Exp2.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);

  std::vector<unsigned char> Vec3 = {
      0x45U, 0x46U, 0x47U, /// Valid OpCodes.
      0xEDU, 0xEEU, 0xEFU, /// Invalid OpCodes.
      0x0BU                /// OpCode End.
  };
  Mgr.setCode(Vec3);
  WasmEdge::AST::Expression Exp3;
  EXPECT_FALSE(Exp3.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec4 = {
      0x45U, 0x46U, 0x47U, /// Valid OpCodes.
      0x0BU                /// OpCode End.
  };
  Mgr.setCode(Vec4);
  WasmEdge::AST::Expression Exp4;
  EXPECT_TRUE(Exp4.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);

  Conf.removeProposal(WasmEdge::Proposal::BulkMemoryOperations);
  Conf.removeProposal(WasmEdge::Proposal::ReferenceTypes);

  std::vector<unsigned char> Vec5 = {
      0x25U, 0x00U, /// Table_get.
      0x0BU         /// OpCode End.
  };
  Mgr.setCode(Vec5);
  WasmEdge::AST::Expression Exp5;
  EXPECT_FALSE(Exp5.loadBinary(Mgr, Conf));
}

} // namespace
