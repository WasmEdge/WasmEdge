/// Unit tests of serialize expressions

#include "loader/serialize.h"

#include <cstdint>
#include <gtest/gtest.h>
#include <vector>

namespace {

WasmEdge::Loader::Serializer Ser;

WasmEdge::AST::CodeSection createCodeSec(size_t SegSize, WasmEdge::AST::Expression Expr) {
  WasmEdge::AST::CodeSection CodeSec;
  WasmEdge::AST::CodeSegment CodeSeg;
  CodeSeg.getExpr() = Expr;
  CodeSeg.setSegSize(SegSize);
  CodeSec.getContent().push_back(CodeSeg);
  return CodeSec;
}

TEST(ExpressionTest, SerializeExpression) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  WasmEdge::AST::Expression Expr;

  // 1. Test serialize expression.
  //
  //   1.  Load expression with only end operation.
  //   2.  Load expression with instructions.

  WasmEdge::AST::Instruction End(WasmEdge::OpCode::End);
  WasmEdge::AST::Instruction I32Eqz(WasmEdge::OpCode::I32__eqz);
  WasmEdge::AST::Instruction I32Eq(WasmEdge::OpCode::I32__eq);
  WasmEdge::AST::Instruction I32Ne(WasmEdge::OpCode::I32__ne);

  Expr.getInstrs() = {
    End
  };
  Output = Ser.serializeSection(createCodeSec(2, Expr));
  Expected = {
      0x0AU, // Code section
      0x04U, // Content size = 4
      0x01U, // Vector length = 1
      0x02U, // Code segment size = 2
      0x00U, // Local vec(0)
      0x0BU  // Expression: OpCode End.
  };
  EXPECT_EQ(Output, Expected);

  Expr.getInstrs() = {
      I32Eqz, I32Eq, I32Ne,
      End
  };
  Output = Ser.serializeSection(createCodeSec(5, Expr));
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
}
}