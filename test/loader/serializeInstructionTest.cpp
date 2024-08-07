// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "loader/serialize.h"

#include <cstdint>
#include <gtest/gtest.h>
#include <vector>

namespace {

WasmEdge::Configure Conf;
WasmEdge::Loader::Serializer Ser(Conf);

WasmEdge::AST::CodeSection
createCodeSec(std::vector<WasmEdge::AST::Instruction> Instructions) {
  WasmEdge::AST::CodeSection CodeSec;
  WasmEdge::AST::CodeSegment CodeSeg;
  WasmEdge::AST::Expression Expr;
  Expr.getInstrs() = Instructions;
  CodeSeg.getExpr() = Expr;
  CodeSec.getContent().push_back(CodeSeg);
  return CodeSec;
}

TEST(SerializeInstructionTest, SerializeBlockControlInstruction) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  std::vector<WasmEdge::AST::Instruction> Instructions;

  // 1. Test block control instructions.
  //
  //   1.  Serialize block with only end operation.
  //   2.  Serialize loop with only end operation.
  //   3.  Serialize block with instructions.
  //   4.  Serialize loop with instructions.

  WasmEdge::AST::Instruction Block(WasmEdge::OpCode::Block);
  WasmEdge::AST::Instruction Loop(WasmEdge::OpCode::Loop);
  WasmEdge::AST::Instruction End(WasmEdge::OpCode::End);
  WasmEdge::AST::Instruction I32Eqz(WasmEdge::OpCode::I32__eqz);
  WasmEdge::AST::Instruction I32Eq(WasmEdge::OpCode::I32__eq);
  WasmEdge::AST::Instruction I32Ne(WasmEdge::OpCode::I32__ne);

  Block.getBlockType().setEmpty();
  Instructions = {Block, End, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected = {
      0x0AU, // Code section
      0x07U, // Content size = 7
      0x01U, // Vector length = 1
      0x05U, // Code segment size = 5
      0x00U, // Local vec(0)
      0x02U, // OpCode Block.
      0x40U, // Block type.
      0x0BU, // OpCode End.
      0x0BU  // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  Loop.getBlockType().setEmpty();
  Instructions = {Loop, End, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected = {
      0x0AU, // Code section
      0x07U, // Content size = 7
      0x01U, // Vector length = 1
      0x05U, // Code segment size = 5
      0x00U, // Local vec(0)
      0x03U, // OpCode Loop.
      0x40U, // Block type.
      0x0BU, // OpCode End.
      0x0BU  // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  Loop.getBlockType().setEmpty();
  Instructions = {Block, I32Eqz, I32Eq, I32Ne, End, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected = {
      0x0AU,               // Code section
      0x0AU,               // Content size = 10
      0x01U,               // Vector length = 1
      0x08U,               // Code segment size = 8
      0x00U,               // Local vec(0)
      0x02U,               // OpCode Block.
      0x40U,               // Block type.
      0x45U, 0x46U, 0x47U, // Valid OpCodes.
      0x0BU,               // OpCode End.
      0x0BU                // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  Loop.getBlockType().setEmpty();
  Instructions = {Loop, I32Eqz, I32Eq, I32Ne, End, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected = {
      0x0AU,               // Code section
      0x0AU,               // Content size = 10
      0x01U,               // Vector length = 1
      0x08U,               // Code segment size = 8
      0x00U,               // Local vec(0)
      0x03U,               // OpCode Loop.
      0x40U,               // Block type.
      0x45U, 0x46U, 0x47U, // Valid OpCodes.
      0x0BU,               // OpCode End.
      0x0BU                // Expression End.
  };
  EXPECT_EQ(Output, Expected);
}

TEST(SerializeInstructionTest, SerializeIfElseControlInstruction) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  std::vector<WasmEdge::AST::Instruction> Instructions;

  // 2. Test serialize if-else control instruction.
  //
  //   1.  Serialize if statement with only end operation.
  //   2.  Serialize if and else statements with only end operation.
  //   3.  Serialize if statement with instructions.
  //   4.  Serialize if and else statements with instructions.

  WasmEdge::AST::Instruction If(WasmEdge::OpCode::If);
  WasmEdge::AST::Instruction Else(WasmEdge::OpCode::Else);
  WasmEdge::AST::Instruction End(WasmEdge::OpCode::End);
  WasmEdge::AST::Instruction I32Eqz(WasmEdge::OpCode::I32__eqz);
  WasmEdge::AST::Instruction I32Eq(WasmEdge::OpCode::I32__eq);
  WasmEdge::AST::Instruction I32Ne(WasmEdge::OpCode::I32__ne);

  If.getBlockType().setEmpty();
  Instructions = {If, End, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected = {
      0x0AU, // Code section
      0x07U, // Content size = 7
      0x01U, // Vector length = 1
      0x05U, // Code segment size = 5
      0x00U, // Local vec(0)
      0x04U, // OpCode If.
      0x40U, // Block type.
      0x0BU, // OpCode End.
      0x0BU  // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  If.getBlockType().setEmpty();
  Instructions = {If, Else, End, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected = {
      0x0AU, // Code section
      0x08U, // Content size = 8
      0x01U, // Vector length = 1
      0x06U, // Code segment size = 6
      0x00U, // Local vec(0)
      0x04U, // OpCode If.
      0x40U, // Block type.
      0x05U, // OpCode Else
      0x0BU, // OpCode End.
      0x0BU  // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  If.getBlockType().setEmpty();
  Instructions = {If, I32Eqz, I32Eq, I32Ne, End, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected = {
      0x0AU,               // Code section
      0x0AU,               // Content size = 10
      0x01U,               // Vector length = 1
      0x08U,               // Code segment size = 8
      0x00U,               // Local vec(0)
      0x04U,               // OpCode If.
      0x40U,               // Block type.
      0x45U, 0x46U, 0x47U, // Valid OpCodes in if statement.
      0x0BU,               // OpCode End.
      0x0BU                // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  If.getBlockType().setEmpty();
  Instructions = {If,     I32Eqz, I32Eq, I32Ne, Else,
                  I32Eqz, I32Eq,  I32Ne, End,   End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected = {
      0x0AU,               // Code section
      0x0EU,               // Content size = 14
      0x01U,               // Vector length = 1
      0x0CU,               // Code segment size = 12
      0x00U,               // Local vec(0)
      0x04U,               // OpCode If.
      0x40U,               // Block type.
      0x45U, 0x46U, 0x47U, // Valid OpCodes in if statement.
      0x05U,               // OpCode Else
      0x45U, 0x46U, 0x47U, // Valid OpCodes in else statement.
      0x0BU,               // OpCode End.
      0x0BU                // Expression End.
  };
  EXPECT_EQ(Output, Expected);
}

TEST(SerializeInstructionTest, SerializeBrControlInstruction) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  std::vector<WasmEdge::AST::Instruction> Instructions;

  // 3. Test branch control instructions.
  //
  //   1.  Serialize valid label index.

  WasmEdge::AST::Instruction Br(WasmEdge::OpCode::Br);
  WasmEdge::AST::Instruction BrIf(WasmEdge::OpCode::Br_if);
  WasmEdge::AST::Instruction End(WasmEdge::OpCode::End);

  Br.getJump().TargetIndex = 0xFFFFFFFFU;
  Instructions = {Br, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected = {
      0x0AU,                             // Code section
      0x0AU,                             // Content size = 10
      0x01U,                             // Vector length = 1
      0x08U,                             // Code segment size = 8
      0x00U,                             // Local vec(0)
      0x0CU,                             // OpCode Br.
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU, // Label index.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  BrIf.getJump().TargetIndex = 0xFFFFFFFFU;
  Instructions = {BrIf, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected[5] = 0x0DU; // OpCode Br_if.
  EXPECT_EQ(Output, Expected);
}

TEST(SerializeInstructionTest, SerializeBrTableControlInstruction) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  std::vector<WasmEdge::AST::Instruction> Instructions;

  // 4. Test branch table control instruction.
  //
  //   1.  Serialize instruction with empty label vector.
  //   2.  Serialize instruction with label vector.

  WasmEdge::AST::Instruction BrTable(WasmEdge::OpCode::Br_table);
  WasmEdge::AST::Instruction End(WasmEdge::OpCode::End);

  BrTable.setLabelListSize(1);
  BrTable.getLabelList()[0].TargetIndex = 0xFFFFFFFFU;
  Instructions = {BrTable, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected = {
      0x0AU,                             // Code section
      0x0BU,                             // Content size = 11
      0x01U,                             // Vector length = 1
      0x09U,                             // Code segment size = 9
      0x00U,                             // Local vec(0)
      0x0EU,                             // OpCode Br_table.
      0x00U,                             // Vector length = 0
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU, // Label index.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  BrTable.setLabelListSize(4);
  BrTable.getLabelList()[0].TargetIndex = 0xFFFFFFF1U;
  BrTable.getLabelList()[1].TargetIndex = 0xFFFFFFF2U;
  BrTable.getLabelList()[2].TargetIndex = 0xFFFFFFF3U;
  BrTable.getLabelList()[3].TargetIndex = 0xFFFFFFFFU;
  Instructions = {BrTable, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected = {
      0x0AU,                             // Code section
      0x1AU,                             // Content size = 26
      0x01U,                             // Vector length = 1
      0x18U,                             // Code segment size = 24
      0x00U,                             // Local vec(0)
      0x0EU,                             // OpCode Br_table.
      0x03U,                             // Vector length = 3
      0xF1U, 0xFFU, 0xFFU, 0xFFU, 0x0FU, // vec[0]
      0xF2U, 0xFFU, 0xFFU, 0xFFU, 0x0FU, // vec[1]
      0xF3U, 0xFFU, 0xFFU, 0xFFU, 0x0FU, // vec[2]
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU, // Label index.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);
}

TEST(SerializeInstructionTest, SerializeCallControlInstruction) {
  WasmEdge::Configure ConfNoRefType;
  ConfNoRefType.removeProposal(WasmEdge::Proposal::ReferenceTypes);
  WasmEdge::Loader::Serializer SerNoRefType(ConfNoRefType);

  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  std::vector<WasmEdge::AST::Instruction> Instructions;

  // 5. Test call control instructions.
  //
  //   1.  Serialize call instruction with valid type index.
  //   2.  Serialize call_indirect instruction with valid type and table index.
  //   3.  Serialize call_indirect instruction with invalid table index without
  //       Ref-Types proposal.

  WasmEdge::AST::Instruction Call(WasmEdge::OpCode::Call);
  WasmEdge::AST::Instruction CallIndirect(WasmEdge::OpCode::Call_indirect);
  WasmEdge::AST::Instruction End(WasmEdge::OpCode::End);

  Call.getTargetIndex() = 0xFFFFFFFFU;
  Instructions = {Call, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected = {
      0x0AU,                             // Code section
      0x0AU,                             // Content size = 10
      0x01U,                             // Vector length = 1
      0x08U,                             // Code segment size = 8
      0x00U,                             // Local vec(0)
      0x10U,                             // OpCode Call.
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU, // Function type index.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  CallIndirect.getTargetIndex() = 0xFFFFFFFFU;
  CallIndirect.getSourceIndex() = 0x05U;
  Instructions = {CallIndirect, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected = {
      0x0AU,                             // Code section
      0x0BU,                             // Content size = 11
      0x01U,                             // Vector length = 1
      0x09U,                             // Code segment size = 9
      0x00U,                             // Local vec(0)
      0x11U,                             // OpCode Call_indirect.
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU, // Type index.
      0x05U,                             // Table index.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  EXPECT_FALSE(
      SerNoRefType.serializeSection(createCodeSec(Instructions), Output));
}

TEST(SerializeInstructionTest, SerializeReferenceInstruction) {
  WasmEdge::Configure ConfNoRefType;
  ConfNoRefType.removeProposal(WasmEdge::Proposal::ReferenceTypes);
  WasmEdge::Loader::Serializer SerNoRefType(ConfNoRefType);

  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  std::vector<WasmEdge::AST::Instruction> Instructions;

  // 6. Test reference instructions.
  //
  //   1.  Serialize function reference type.
  //   2.  Serialize invalid reference type without Ref-Types proposal.

  WasmEdge::AST::Instruction RefNull(WasmEdge::OpCode::Ref__null);
  WasmEdge::AST::Instruction End(WasmEdge::OpCode::End);

  RefNull.setValType(WasmEdge::TypeCode::FuncRef);
  Instructions = {RefNull, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected = {
      0x0AU, // Code section
      0x06U, // Content size = 6
      0x01U, // Vector length = 1
      0x04U, // Code segment size = 4
      0x00U, // Local vec(0)
      0xD0U, // OpCode Ref__null.
      0x70U, // FuncRef
      0x0BU  // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  RefNull.setValType(WasmEdge::TypeCode::ExternRef);
  Instructions = {RefNull, End};
  EXPECT_FALSE(
      SerNoRefType.serializeSection(createCodeSec(Instructions), Output));
}

TEST(SerializeInstructionTest, SerializeParametricInstruction) {
  WasmEdge::Configure ConfNoSIMD;
  ConfNoSIMD.removeProposal(WasmEdge::Proposal::SIMD);
  WasmEdge::Loader::Serializer SerNoSIMD(ConfNoSIMD);

  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  std::vector<WasmEdge::AST::Instruction> Instructions;

  // 7. Test parametric instructions.
  //
  //   1.  Serialize valid select_t instruction with value type list.
  //   2.  Serialize invalid value type list without SIMD proposal.

  WasmEdge::AST::Instruction SelectT(WasmEdge::OpCode::Select_t);
  WasmEdge::AST::Instruction End(WasmEdge::OpCode::End);

  SelectT.setValTypeListSize(2);
  SelectT.getValTypeList()[0] = WasmEdge::TypeCode::I32;
  SelectT.getValTypeList()[1] = WasmEdge::TypeCode::I64;
  Instructions = {SelectT, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected = {
      0x0AU,        // Code section
      0x08U,        // Content size = 8
      0x01U,        // Vector length = 1
      0x06U,        // Code segment size = 6
      0x00U,        // Local vec(0)
      0x1CU,        // OpCode Select_t.
      0x02U,        // Vector length = 2
      0x7FU, 0x7EU, // Value types
      0x0BU         // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  SelectT.getValTypeList()[0] = WasmEdge::TypeCode::V128;
  SelectT.getValTypeList()[1] = WasmEdge::TypeCode::V128;
  Instructions = {SelectT, End};
  EXPECT_FALSE(SerNoSIMD.serializeSection(createCodeSec(Instructions), Output));
}

TEST(SerializeInstructionTest, SerializeVariableInstruction) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  std::vector<WasmEdge::AST::Instruction> Instructions;

  // 8. Test variable instructions.
  //
  //   1.  Serialize valid local or global index.

  WasmEdge::AST::Instruction LocalGet(WasmEdge::OpCode::Local__get);
  WasmEdge::AST::Instruction End(WasmEdge::OpCode::End);

  LocalGet.getTargetIndex() = 0xFFFFFFFFU;
  Instructions = {LocalGet, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected = {
      0x0AU,                             // Code section
      0x0AU,                             // Content size = 10
      0x01U,                             // Vector length = 1
      0x08U,                             // Code segment size = 8
      0x00U,                             // Local vec(0)
      0x20U,                             // OpCode Local__get.
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU, // Local index.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);
}

TEST(SerializeInstructionTest, SerializeTableInstruction) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  std::vector<WasmEdge::AST::Instruction> Instructions;

  // 9. Test table instructions.
  //
  //   1.  Serialize table_get instruction.
  //   2.  Serialize table_init instruction.

  WasmEdge::AST::Instruction TableGet(WasmEdge::OpCode::Table__get);
  WasmEdge::AST::Instruction TableInit(WasmEdge::OpCode::Table__init);
  WasmEdge::AST::Instruction End(WasmEdge::OpCode::End);

  TableGet.getTargetIndex() = 0xFFFFFFFFU;
  Instructions = {TableGet, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected = {
      0x0AU,                             // Code section
      0x0AU,                             // Content size = 10
      0x01U,                             // Vector length = 1
      0x08U,                             // Code segment size = 8
      0x00U,                             // Local vec(0)
      0x25U,                             // OpCode Table__get.
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU, // Table index.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  TableInit.getSourceIndex() = 0x05U;
  TableInit.getTargetIndex() = 0xFFFFFFFFU;
  Instructions = {TableInit, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected = {
      0x0AU,                             // Code section
      0x0CU,                             // Content size = 12
      0x01U,                             // Vector length = 1
      0x0AU,                             // Code segment size = 10
      0x00U,                             // Local vec(0)
      0xFCU, 0x0CU,                      // OpCode Table__init.
      0x05U,                             // Element idx.
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU, // Table index.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);
}

TEST(SerializeInstructionTest, SerializeMemoryInstruction) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  std::vector<WasmEdge::AST::Instruction> Instructions;

  // 10. Test memory instructions.
  //
  //   1.  Serialize memory_grow instruction.
  //   2.  Serialize i32_load instruction.

  WasmEdge::AST::Instruction MemoryGrow(WasmEdge::OpCode::Memory__grow);
  WasmEdge::AST::Instruction I32Load(WasmEdge::OpCode::I32__load);
  WasmEdge::AST::Instruction End(WasmEdge::OpCode::End);

  Instructions = {MemoryGrow, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected = {
      0x0AU, // Code section
      0x06U, // Content size = 6
      0x01U, // Vector length = 1
      0x04U, // Code segment size = 4
      0x00U, // Local vec(0)
      0x40U, // OpCode Memory__grow.
      0x00U, // Checking byte
      0x0BU  // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  I32Load.getMemoryAlign() = 0xFFFFFFFFU;
  I32Load.getMemoryOffset() = 0xFFFFFFFEU;
  Instructions = {I32Load, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected = {
      0x0AU,                             // Code section
      0x0FU,                             // Content size = 15
      0x01U,                             // Vector length = 1
      0x0DU,                             // Code segment size = 13
      0x00U,                             // Local vec(0)
      0x28U,                             // OpCode I32__load.
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU, // Align.
      0xFEU, 0xFFU, 0xFFU, 0xFFU, 0x0FU, // Offset.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  I32Load.getMemoryAlign() = 0xFFFFFFFFU;
  I32Load.getMemoryOffset() = 0xFFFFFFFEU;
  Instructions = {I32Load, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected = {
      0x0AU,                             // Code section
      0x0FU,                             // Content size = 15
      0x01U,                             // Vector length = 1
      0x0DU,                             // Code segment size = 13
      0x00U,                             // Local vec(0)
      0x28U,                             // OpCode I32__load.
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU, // Align.
      0xFEU, 0xFFU, 0xFFU, 0xFFU, 0x0FU, // Offset.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);
}

TEST(SerializeInstructionTest, SerializeConstInstruction) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  std::vector<WasmEdge::AST::Instruction> Instructions;

  // 11. Test const numeric instructions.
  //
  //   1.  Serialize I32 const numeric instruction.
  //   2.  Serialize I64 const numeric instruction.
  //   3.  Serialize F32 const numeric instruction.
  //   4.  Serialize F64 const numeric instruction.

  WasmEdge::AST::Instruction I32Const(WasmEdge::OpCode::I32__const);
  WasmEdge::AST::Instruction I64Const(WasmEdge::OpCode::I64__const);
  WasmEdge::AST::Instruction F32Const(WasmEdge::OpCode::F32__const);
  WasmEdge::AST::Instruction F64Const(WasmEdge::OpCode::F64__const);
  WasmEdge::AST::Instruction End(WasmEdge::OpCode::End);

  I32Const.setNum(-123456);
  Instructions = {I32Const, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected = {
      0x0AU,               // Code section
      0x08U,               // Content size = 8
      0x01U,               // Vector length = 1
      0x06U,               // Code segment size = 6
      0x00U,               // Local vec(0)
      0x41U,               // OpCode I32__const.
      0xC0U, 0xBBU, 0x78U, // I32 -123456.
      0x0BU                // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  I64Const.setNum(static_cast<uint64_t>(-112233445566L));
  Instructions = {I64Const, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected = {
      0x0AU,                                    // Code section
      0x0BU,                                    // Content size = 11
      0x01U,                                    // Vector length = 1
      0x09U,                                    // Code segment size = 9
      0x00U,                                    // Local vec(0)
      0x42U,                                    // OpCode I64__const.
      0xC2U, 0x8EU, 0xF6U, 0xF2U, 0xDDU, 0x7CU, // I64 -112233445566
      0x0BU                                     // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  F32Const.setNum(static_cast<float>(-0x1.921fb4p+1)); // -3.1415926F
  Instructions = {F32Const, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected = {
      0x0AU,                      // Code section
      0x09U,                      // Content size = 9
      0x01U,                      // Vector length = 1
      0x07U,                      // Code segment size = 7
      0x00U,                      // Local vec(0)
      0x43U,                      // OpCode F32__const.
      0xDAU, 0x0FU, 0x49U, 0xC0U, // F32 -3.1415926
      0x0BU                       // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  F64Const.setNum(-3.1415926535897932);
  Instructions = {F64Const, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected = {
      0x0AU, // Code section
      0x0DU, // Content size = 13
      0x01U, // Vector length = 1
      0x0BU, // Code segment size = 11
      0x00U, // Local vec(0)
      0x44U, // OpCode F64__const.
      0x18U, 0x2DU, 0x44U, 0x54U,
      0xFBU, 0x21U, 0x09U, 0xC0U, // F64 -3.1415926535897932
      0x0BU                       // Expression End.
  };
  EXPECT_EQ(Output, Expected);
}
} // namespace
