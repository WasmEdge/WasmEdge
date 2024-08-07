// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "loader/serialize.h"

#include <cstdint>
#include <gtest/gtest.h>
#include <vector>

namespace {

WasmEdge::Configure Conf;
WasmEdge::Loader::Serializer Ser(Conf);

WasmEdge::AST::CodeSection createCodeSec(WasmEdge::AST::Expression Expr) {
  WasmEdge::AST::CodeSection CodeSec;
  WasmEdge::AST::CodeSegment CodeSeg;
  CodeSeg.getExpr() = Expr;
  CodeSec.getContent().push_back(CodeSeg);
  return CodeSec;
}

TEST(ExpressionTest, SerializeExpression) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  WasmEdge::AST::Expression Expr;

  WasmEdge::Configure ConfNoRefType;
  ConfNoRefType.removeProposal(WasmEdge::Proposal::BulkMemoryOperations);
  ConfNoRefType.removeProposal(WasmEdge::Proposal::ReferenceTypes);
  WasmEdge::Loader::Serializer SerNoRefType(ConfNoRefType);

  // 1. Test serialize expression.
  //
  //   1.  Serialize expression with only end operation.
  //   2.  Serialize expression with instructions.
  //   3.  Serialize expression with instructions not in proposals.

  WasmEdge::AST::Instruction End(WasmEdge::OpCode::End);
  WasmEdge::AST::Instruction I32Eqz(WasmEdge::OpCode::I32__eqz);
  WasmEdge::AST::Instruction I32Eq(WasmEdge::OpCode::I32__eq);
  WasmEdge::AST::Instruction I32Ne(WasmEdge::OpCode::I32__ne);
  WasmEdge::AST::Instruction TableGet(WasmEdge::OpCode::Table__get);

  Expr.getInstrs() = {End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Expr), Output));
  Expected = {
      0x0AU, // Code section
      0x04U, // Content size = 4
      0x01U, // Vector length = 1
      0x02U, // Code segment size = 2
      0x00U, // Local vec(0)
      0x0BU  // Expression: OpCode End.
  };
  EXPECT_EQ(Output, Expected);

  Expr.getInstrs() = {I32Eqz, I32Eq, I32Ne, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Expr), Output));
  Expected = {
      0x0AU,               // Code section
      0x07U,               // Content size = 7
      0x01U,               // Vector length = 1
      0x05U,               // Code segment size = 5
      0x00U,               // Local vec(0)
      0x45U, 0x46U, 0x47U, // Valid OpCodes.
      0x0BU                // OpCode End.
  };
  EXPECT_EQ(Output, Expected);

  Expr.getInstrs() = {TableGet, End};
  EXPECT_FALSE(SerNoRefType.serializeSection(createCodeSec(Expr), Output));
}
} // namespace
