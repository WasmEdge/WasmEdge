// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "loader/serialize.h"

#include <cstdint>
#include <gtest/gtest.h>
#include <vector>

namespace {

WasmEdge::Loader::Serializer Ser;

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
  Ser.serializeSection(createCodeSec(Instructions), Output);
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
  Ser.serializeSection(createCodeSec(Instructions), Output);
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
  Ser.serializeSection(createCodeSec(Instructions), Output);
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
  Ser.serializeSection(createCodeSec(Instructions), Output);
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
  Block.getBlockType().setData(WasmEdge::ValType(WasmEdge::TypeCode::I32));
  Instructions = {Block, End, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU, // Code section
      0x07U, // Content size = 7
      0x01U, // Vector length = 1
      0x05U, // Code segment size = 5
      0x00U, // Local vec(0)
      0x02U, // OpCode Block.
      0x7FU, // Block type I32.
      0x0BU, // OpCode End.
      0x0BU  // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  // A type index block type is an s33, so 64 needs a continuation byte.
  Block.getBlockType().setData(64U);
  Instructions = {Block, End, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU,        // Code section
      0x08U,        // Content size = 8
      0x01U,        // Vector length = 1
      0x06U,        // Code segment size = 6
      0x00U,        // Local vec(0)
      0x02U,        // OpCode Block.
      0xC0U, 0x00U, // Block type index 64.
      0x0BU,        // OpCode End.
      0x0BU         // Expression End.
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
  Ser.serializeSection(createCodeSec(Instructions), Output);
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
  Ser.serializeSection(createCodeSec(Instructions), Output);
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
  Ser.serializeSection(createCodeSec(Instructions), Output);
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
  Ser.serializeSection(createCodeSec(Instructions), Output);
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
  //   2.  Serialize Br_on_null instruction.
  //   3.  Serialize Br_on_non_null instruction.
  //   4.  Serialize Br_on_cast instruction.
  //   5.  Serialize Br_on_cast_fail instruction.

  WasmEdge::AST::Instruction Br(WasmEdge::OpCode::Br);
  WasmEdge::AST::Instruction BrIf(WasmEdge::OpCode::Br_if);
  WasmEdge::AST::Instruction BrOnNull(WasmEdge::OpCode::Br_on_null);
  WasmEdge::AST::Instruction BrOnNonNull(WasmEdge::OpCode::Br_on_non_null);
  WasmEdge::AST::Instruction BrOnCast(WasmEdge::OpCode::Br_on_cast);
  WasmEdge::AST::Instruction BrOnCastFail(WasmEdge::OpCode::Br_on_cast_fail);
  WasmEdge::AST::Instruction End(WasmEdge::OpCode::End);

  Br.getJump().TargetIndex = 0xFFFFFFFFU;
  Instructions = {Br, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
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
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected[5] = 0x0DU; // OpCode Br_if.
  EXPECT_EQ(Output, Expected);

  BrOnNull.getJump().TargetIndex = 0xFFFFFFFFU;
  Instructions = {BrOnNull, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected[5] = 0xD5U; // OpCode Br_on_null
  EXPECT_EQ(Output, Expected);

  BrOnNonNull.getJump().TargetIndex = 0xFFFFFFFFU;
  Instructions = {BrOnNonNull, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected[5] = 0xD6U; // OpCode Br_on_non_null
  EXPECT_EQ(Output, Expected);

  Instructions = {BrOnNonNull, End};
  Output = {};

  BrOnCast.setBrCast(0xFFFFFFFFU);
  BrOnCast.getBrCast().RType1 = WasmEdge::TypeCode::AnyRef;
  BrOnCast.getBrCast().RType2 = WasmEdge::TypeCode::EqRef;
  Instructions = {BrOnCast, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU,                             // Code section
      0x0EU,                             // Content size = 14
      0x01U,                             // Vector length = 1
      0x0CU,                             // Code segment size = 12
      0x00U,                             // Local vec(0)
      0xFBU, 0x18U,                      // OpCode Br_on_cast.
      0x03U,                             // Flags.
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU, // Label index.
      0x6EU,                             // OpCode AnyRef.
      0x6DU,                             // OpCode EqRef.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  BrOnCastFail.setBrCast(0xFFFFFFFFU);
  BrOnCastFail.getBrCast().RType1 = WasmEdge::TypeCode::AnyRef;
  BrOnCastFail.getBrCast().RType2 = WasmEdge::TypeCode::EqRef;
  Instructions = {BrOnCastFail, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected[6] = 0x19; // OpCode Br_on_cast_fail.
  EXPECT_EQ(Output, Expected);
  // Only the first type is nullable here, which separates the two flag bits.
  BrOnCast.setBrCast(0xFFFFFFFFU);
  BrOnCast.getBrCast().RType1 = WasmEdge::TypeCode::AnyRef;
  BrOnCast.getBrCast().RType2 =
      WasmEdge::ValType(WasmEdge::TypeCode::Ref, WasmEdge::TypeCode::EqRef, 0);
  Instructions = {BrOnCast, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU,                             // Code section
      0x0EU,                             // Content size = 14
      0x01U,                             // Vector length = 1
      0x0CU,                             // Code segment size = 12
      0x00U,                             // Local vec(0)
      0xFBU, 0x18U,                      // OpCode Br_on_cast.
      0x01U,                             // Flags.
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU, // Label index.
      0x6EU,                             // OpCode AnyRef.
      0x6DU,                             // OpCode EqRef.
      0x0BU                              // Expression End.
  };
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
  Ser.serializeSection(createCodeSec(Instructions), Output);
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
  Ser.serializeSection(createCodeSec(Instructions), Output);
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

  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  std::vector<WasmEdge::AST::Instruction> Instructions;

  // 5. Test call control instructions.
  //
  //   1.  Serialize call instruction with valid type index.
  //   2.  Serialize call_indirect instruction with valid type and table index.
  //   3.  Serialize Call_ref instruction with valid type index.
  //   4.  Serialize Return_call_ref instruction with valid type and table.

  WasmEdge::AST::Instruction Call(WasmEdge::OpCode::Call);
  WasmEdge::AST::Instruction CallIndirect(WasmEdge::OpCode::Call_indirect);
  WasmEdge::AST::Instruction CallRef(WasmEdge::OpCode::Call_ref);
  WasmEdge::AST::Instruction ReturnCallRef(WasmEdge::OpCode::Return_call_ref);
  WasmEdge::AST::Instruction End(WasmEdge::OpCode::End);

  Call.getTargetIndex() = 0xFFFFFFFFU;
  Instructions = {Call, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
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
  Ser.serializeSection(createCodeSec(Instructions), Output);
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

  CallRef.getTargetIndex() = 0xFFFFFFFFU;
  Instructions = {CallRef, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU,                             // Code section
      0x0AU,                             // Content size = 10
      0x01U,                             // Vector length = 1
      0x08U,                             // Code segment size = 8
      0x00U,                             // Local vec(0)
      0x14U,                             // OpCode Call_ref.
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU, // Function type index.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  ReturnCallRef.getTargetIndex() = 0xFFFFFFFFU;
  Instructions = {ReturnCallRef, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected[5] = 0x15U; // OpCode Return_call_ref.
  EXPECT_EQ(Output, Expected);

  ReturnCallRef.getTargetIndex() = 0xFFFFFFFFU;
  Instructions = {ReturnCallRef, End};

  // The remaining opcodes of this group, with their immediates zeroed.
  // The cases above cover the non-zero values.

  Instructions = {
      WasmEdge::AST::Instruction(WasmEdge::OpCode::Return),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::Return_call),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::Return_call_indirect), End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU, 0x0AU, 0x01U, 0x08U, 0x00U, // Framing.
      0x0FU,                             // Return.
      0x12U, 0x00U,                      // Return_call.
      0x13U, 0x00U, 0x00U,               // Return_call_indirect.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);
}

TEST(SerializeInstructionTest, SerializeEHControlInstruction) {

  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  std::vector<WasmEdge::AST::Instruction> Instructions;

  // 6. Test exception handling instruction.
  //
  //   1.  Serialize Throw_ref instruction.
  //   2.  Serialize Throw instruction.
  //   3.  Serialize Try_table instruction.

  WasmEdge::AST::Instruction ThrowRef(WasmEdge::OpCode::Throw_ref);
  WasmEdge::AST::Instruction Throw(WasmEdge::OpCode::Throw);
  WasmEdge::AST::Instruction TryTable(WasmEdge::OpCode::Try_table);
  WasmEdge::AST::Instruction End(WasmEdge::OpCode::End);

  Instructions = {ThrowRef, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU, // Code section
      0x05U, // Content size = 5
      0x01U, // Vector length = 1
      0x03U, // Code segment size = 3
      0x00U, // Local vec(0)
      0x0AU, // Throw_ref instruction.
      0x0BU  // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  Output = {};

  Throw.getTargetIndex() = 0xFFFFFFFFU;
  Instructions = {Throw, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU,                             // Code section
      0x0AU,                             // Content size = 10
      0x01U,                             // Vector length = 1
      0x08U,                             // Code segment size = 8
      0x00U,                             // Local vec(0)
      0x08U,                             // OpCode Throw.
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU, // Throw type index.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  Output = {};

  TryTable.setTryCatch();
  TryTable.getTryCatch().ResType.setEmpty();
  TryTable.getTryCatch().Catch.resize(4);
  TryTable.getTryCatch().Catch[0].TagIndex = 0xFF3F1F0FU;
  TryTable.getTryCatch().Catch[0].LabelIndex = 0xFFFFFFFFU;
  TryTable.getTryCatch().Catch[1].IsRef = true;
  TryTable.getTryCatch().Catch[1].TagIndex = 0xFF3F1F0FU;
  TryTable.getTryCatch().Catch[1].LabelIndex = 0xFFFFFFFFU;
  TryTable.getTryCatch().Catch[2].IsAll = true;
  TryTable.getTryCatch().Catch[2].LabelIndex = 0xFFFFFFFFU;
  TryTable.getTryCatch().Catch[3].IsRef = true;
  TryTable.getTryCatch().Catch[3].IsAll = true;
  TryTable.getTryCatch().Catch[3].LabelIndex = 0xFFFFFFFFU;
  Instructions = {TryTable, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU,                             // Code section
      0x29U,                             // Content size = 41
      0x01U,                             // Vector length = 1
      0x27U,                             // Code segment size = 39
      0x00U,                             // Local vec(0)
      0x1FU,                             // OpCode Try_table.
      0x40U,                             // OpCode Epsilon.
      0x04U,                             // Vector length = 4
      0x00U,                             // Catch flag.
      0x8FU, 0xBEU, 0xFCU, 0xF9U, 0x0FU, // Tag index.
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU, // Label index.
      0x01U,                             // Catch flag.
      0x8FU, 0xBEU, 0xFCU, 0xF9U, 0x0FU, // Tag index.
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU, // Label index.
      0x02U,                             // Catch flag.
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU, // Label index.
      0x03U,                             // Catch flag.
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU, // Label index.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);
}

TEST(SerializeInstructionTest, SerializeReferenceInstruction) {

  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  std::vector<WasmEdge::AST::Instruction> Instructions;

  // 7. Test reference instructions.
  //
  //   1.  Serialize function reference type.
  //   2.  Serialize Ref_as_non_null instruction with valid type index.
  //   3.  Serialize Ref__eq instruction.
  //   4.  Serialize Ref__i31 instruction.
  //   5.  Serialize Ref__test instruction.
  //   6.  Serialize Ref__test_null instruction.
  //   7.  Serialize Ref__cast instruction.
  //   8.  Serialize Ref__cast_null instruction.
  //   9.  Serialize Any__convert_extern instruction.
  //   10.  Serialize Extern__convert_any instruction.
  //   11.  Serialize I31__get_s instruction.
  //   12.  Serialize I31__get_u instruction.
  //   13.  Serialize Struct__new instruction.
  //   14.  Serialize Struct__new_default instruction.
  //   15.  Serialize Struct__get instruction.
  //   16.  Serialize Struct__get_s instruction.
  //   17.  Serialize Struct__get_u instruction.
  //   18.  Serialize Struct__set instruction.
  //   19.  Serialize Array__new instruction.
  //   20.  Serialize Array__new_default instruction.
  //   21.  Serialize Array__get instruction.
  //   22.  Serialize Array__get_s instruction.
  //   23.  Serialize Array__get_u instruction.
  //   24.  Serialize Array__set instruction.
  //   25.  Serialize Array__fill instruction.
  //   26.  Serialize Array__len instruction.
  //   27.  Serialize Array__new_fixed instruction.
  //   28.  Serialize Array__new_data instruction.
  //   29.  Serialize Array__new_elem instruction.
  //   30.  Serialize Array__copy instruction.
  //   31.  Serialize Array__init_data instruction.
  //   32.  Serialize Array__init_elem instruction.

  WasmEdge::AST::Instruction RefNull(WasmEdge::OpCode::Ref__null);
  WasmEdge::AST::Instruction RefAsNonNull(WasmEdge::OpCode::Ref__as_non_null);
  WasmEdge::AST::Instruction RefEq(WasmEdge::OpCode::Ref__eq);
  WasmEdge::AST::Instruction RefI31(WasmEdge::OpCode::Ref__i31);
  WasmEdge::AST::Instruction RefTest(WasmEdge::OpCode::Ref__test);
  WasmEdge::AST::Instruction RefTestNull(WasmEdge::OpCode::Ref__test_null);
  WasmEdge::AST::Instruction RefCast(WasmEdge::OpCode::Ref__cast);
  WasmEdge::AST::Instruction RefCastNull(WasmEdge::OpCode::Ref__cast_null);
  WasmEdge::AST::Instruction AnyConvertExtern(
      WasmEdge::OpCode::Any__convert_extern);
  WasmEdge::AST::Instruction ExternConvertAny(
      WasmEdge::OpCode::Extern__convert_any);
  WasmEdge::AST::Instruction I31GetS(WasmEdge::OpCode::I31__get_s);
  WasmEdge::AST::Instruction I31GetU(WasmEdge::OpCode::I31__get_u);
  WasmEdge::AST::Instruction StructNew(WasmEdge::OpCode::Struct__new);
  WasmEdge::AST::Instruction StructNewDefault(
      WasmEdge::OpCode::Struct__new_default);
  WasmEdge::AST::Instruction StructGet(WasmEdge::OpCode::Struct__get);
  WasmEdge::AST::Instruction StructGetS(WasmEdge::OpCode::Struct__get_s);
  WasmEdge::AST::Instruction StructGetU(WasmEdge::OpCode::Struct__get_u);
  WasmEdge::AST::Instruction StructSet(WasmEdge::OpCode::Struct__set);
  WasmEdge::AST::Instruction ArrayNew(WasmEdge::OpCode::Array__new);
  WasmEdge::AST::Instruction ArrayNewDefault(
      WasmEdge::OpCode::Array__new_default);
  WasmEdge::AST::Instruction ArrayGet(WasmEdge::OpCode::Array__get);
  WasmEdge::AST::Instruction ArrayGetS(WasmEdge::OpCode::Array__get_s);
  WasmEdge::AST::Instruction ArrayGetU(WasmEdge::OpCode::Array__get_u);
  WasmEdge::AST::Instruction ArraySet(WasmEdge::OpCode::Array__set);
  WasmEdge::AST::Instruction ArrayFill(WasmEdge::OpCode::Array__fill);
  WasmEdge::AST::Instruction ArrayLen(WasmEdge::OpCode::Array__len);
  WasmEdge::AST::Instruction ArrayNewFixed(WasmEdge::OpCode::Array__new_fixed);
  WasmEdge::AST::Instruction ArrayNewData(WasmEdge::OpCode::Array__new_data);
  WasmEdge::AST::Instruction ArrayNewElem(WasmEdge::OpCode::Array__new_elem);
  WasmEdge::AST::Instruction ArrayCopy(WasmEdge::OpCode::Array__copy);
  WasmEdge::AST::Instruction ArrayInitData(WasmEdge::OpCode::Array__init_data);
  WasmEdge::AST::Instruction ArrayInitElem(WasmEdge::OpCode::Array__init_elem);
  WasmEdge::AST::Instruction End(WasmEdge::OpCode::End);

  RefNull.setValType(WasmEdge::TypeCode::FuncRef);
  Instructions = {RefNull, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
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

  Instructions = {RefAsNonNull, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU, // Code section
      0x05U, // Content size = 5
      0x01U, // Vector length = 1
      0x03U, // Code segment size = 3
      0x00U, // Local vec(0)
      0xD4U, // OpCode Ref__as_non_null.
      0x0BU  // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  Instructions = {RefAsNonNull, End};
  Output = {};

  Instructions = {RefEq, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected[5] = 0xD3U; // OpCode Ref__eq.
  EXPECT_EQ(Expected, Output);

  Instructions = {RefI31, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU,       // Code section
      0x06U,       // Content size = 5
      0x01U,       // Vector length = 1
      0x04U,       // Code segment size = 3
      0x00U,       // Local vec(0)
      0xFBU, 0x1C, // OpCode Ref__i31.
      0x0BU        // Expression End.
  };
  EXPECT_EQ(Expected, Output);

  RefTest.setValType(WasmEdge::TypeCode::FuncRef);
  Instructions = {RefTest, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU,        // Code section
      0x07U,        // Content size = 6
      0x01U,        // Vector length = 1
      0x05U,        // Code segment size = 4
      0x00U,        // Local vec(0)
      0xFBU, 0x14U, // OpCode Ref__test.
      0x70U,        // FuncRef
      0x0BU         // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  RefTestNull.setValType(WasmEdge::TypeCode::FuncRef);
  Instructions = {RefTestNull, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected[6] = 0x15U; // OpCode Ref__test_null.
  EXPECT_EQ(Output, Expected);

  RefCast.setValType(WasmEdge::TypeCode::FuncRef);
  Instructions = {RefCast, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected[6] = 0x16U; // OpCode Ref__cast.
  EXPECT_EQ(Output, Expected);

  RefCastNull.setValType(WasmEdge::TypeCode::FuncRef);
  Instructions = {RefCastNull, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected[6] = 0x17U; // OpCode Ref__cast_null.
  EXPECT_EQ(Output, Expected);

  Output = {};
  Instructions = {RefCastNull, End};

  Instructions = {AnyConvertExtern, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU,        // Code section
      0x06U,        // Content size = 5
      0x01U,        // Vector length = 1
      0x04U,        // Code segment size = 3
      0x00U,        // Local vec(0)
      0xFBU, 0x1AU, // OpCode Any__convert_extern.
      0x0BU         // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  Instructions = {ExternConvertAny, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected[6] = 0x1BU; // OpCode Extern__convert_any.
  EXPECT_EQ(Output, Expected);

  Output = {};

  Instructions = {I31GetS, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU,        // Code section
      0x06U,        // Content size = 5
      0x01U,        // Vector length = 1
      0x04U,        // Code segment size = 3
      0x00U,        // Local vec(0)
      0xFBU, 0x1DU, // OpCode I31__get_s.
      0x0BU         // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  Instructions = {I31GetU, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected[6] = 0x1EU; // OpCode I31__get_u.
  EXPECT_EQ(Output, Expected);

  Output = {};

  StructNew.getTargetIndex() = 0xFFFFFFFFU;
  Instructions = {StructNew, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU,                             // Code section
      0x0BU,                             // Content size = 11
      0x01U,                             // Vector length = 1
      0x09U,                             // Code segment size = 9
      0x00U,                             // Local vec(0)
      0xFBU, 0x00U,                      // OpCode Struct__new.
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU, // Type index.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  StructNewDefault.getTargetIndex() = 0xFFFFFFFFU;
  Instructions = {StructNewDefault, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected[6] = 0x01U; // OpCode Struct__new_default.
  EXPECT_EQ(Output, Expected);

  StructGet.getSourceIndex() = 0x05U;
  StructGet.getTargetIndex() = 0xFFFFFFFFU;
  Instructions = {StructGet, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU,                             // Code section
      0x0CU,                             // Content size = 12
      0x01U,                             // Vector length = 1
      0x0AU,                             // Code segment size = 10
      0x00U,                             // Local vec(0)
      0xFBU, 0x02U,                      // OpCode Struct__get.
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU, // Type index.
      0x05U,                             // Source index.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  StructGetS.getSourceIndex() = 0x05U;
  StructGetS.getTargetIndex() = 0xFFFFFFFFU;
  Instructions = {StructGetS, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected[6] = 0x03U; // OpCode Struct__get_s.
  EXPECT_EQ(Output, Expected);

  StructGetU.getSourceIndex() = 0x05U;
  StructGetU.getTargetIndex() = 0xFFFFFFFFU;
  Instructions = {StructGetU, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected[6] = 0x04U; // OpCode Struct__get_u.
  EXPECT_EQ(Output, Expected);

  StructSet.getSourceIndex() = 0x05U;
  StructSet.getTargetIndex() = 0xFFFFFFFFU;
  Instructions = {StructSet, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected[6] = 0x05U; // OpCode Struct__set.
  EXPECT_EQ(Output, Expected);

  Output = {};

  ArrayNew.getTargetIndex() = 0xFFFFFFFFU;
  Instructions = {ArrayNew, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU,                             // Code section
      0x0BU,                             // Content size = 11
      0x01U,                             // Vector length = 1
      0x09U,                             // Code segment size = 9
      0x00U,                             // Local vec(0)
      0xFBU, 0x06U,                      // OpCode Array__new.
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU, // Type index.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  ArrayNewDefault.getTargetIndex() = 0xFFFFFFFFU;
  Instructions = {ArrayNewDefault, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected[6] = 0x07U; // OpCode Array__new_default.
  EXPECT_EQ(Output, Expected);

  ArrayGet.getTargetIndex() = 0xFFFFFFFFU;
  Instructions = {ArrayGet, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected[6] = 0x0BU; // OpCode Array__get.
  EXPECT_EQ(Output, Expected);

  ArrayGetS.getTargetIndex() = 0xFFFFFFFFU;
  Instructions = {ArrayGetS, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected[6] = 0x0CU; // OpCode Array__get_s.
  EXPECT_EQ(Output, Expected);

  ArrayGetU.getTargetIndex() = 0xFFFFFFFFU;
  Instructions = {ArrayGetU, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected[6] = 0x0DU; // OpCode Array__get_u.
  EXPECT_EQ(Output, Expected);

  ArraySet.getTargetIndex() = 0xFFFFFFFFU;
  Instructions = {ArraySet, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected[6] = 0x0EU; // OpCode Array__set.
  EXPECT_EQ(Output, Expected);

  ArrayFill.getTargetIndex() = 0xFFFFFFFFU;
  Instructions = {ArrayFill, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected[6] = 0x10U; // OpCode Array__fill.
  EXPECT_EQ(Output, Expected);

  Instructions = {ArrayLen, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected[6] = 0x0FU; // OpCode Array__len.
  Expected = {
      0x0AU,        // Code section
      0x06U,        // Content size = 6
      0x01U,        // Vector length = 1
      0x04U,        // Code segment size = 4
      0x00U,        // Local vec(0)
      0xFBU, 0x0FU, // OpCode Array__len.
      0x0BU         // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  ArrayNewFixed.getSourceIndex() = 0x05U;
  ArrayNewFixed.getTargetIndex() = 0xFFFFFFFFU;
  Instructions = {ArrayNewFixed, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU,                             // Code section
      0x0CU,                             // Content size = 12
      0x01U,                             // Vector length = 1
      0x0AU,                             // Code segment size = 10
      0x00U,                             // Local vec(0)
      0xFBU, 0x08U,                      // OpCode Array__new_fixed.
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU, // Type index.
      0x05U,                             // Source index.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  ArrayNewData.getSourceIndex() = 0x05U;
  ArrayNewData.getTargetIndex() = 0xFFFFFFFFU;
  Instructions = {ArrayNewData, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected[6] = 0x09U; // OpCode Array__new_data.
  EXPECT_EQ(Output, Expected);

  ArrayNewElem.getSourceIndex() = 0x05U;
  ArrayNewElem.getTargetIndex() = 0xFFFFFFFFU;
  Instructions = {ArrayNewElem, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected[6] = 0x0AU; // OpCode Array__new_elem.
  EXPECT_EQ(Output, Expected);

  ArrayCopy.getSourceIndex() = 0x05U;
  ArrayCopy.getTargetIndex() = 0xFFFFFFFFU;
  Instructions = {ArrayCopy, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected[6] = 0x11U; // OpCode Array__copy.
  EXPECT_EQ(Output, Expected);

  ArrayInitData.getSourceIndex() = 0x05U;
  ArrayInitData.getTargetIndex() = 0xFFFFFFFFU;
  Instructions = {ArrayInitData, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected[6] = 0x12U; // OpCode Array__init_data.
  EXPECT_EQ(Output, Expected);

  ArrayInitElem.getSourceIndex() = 0x05U;
  ArrayInitElem.getTargetIndex() = 0xFFFFFFFFU;
  Instructions = {ArrayInitElem, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected[6] = 0x13U; // OpCode Array__init_elem.
  EXPECT_EQ(Output, Expected);
  // A concrete heap type is an s33 type index, so 64 needs a continuation
  // byte.
  RefNull.setValType(WasmEdge::ValType(WasmEdge::TypeCode::RefNull,
                                       WasmEdge::TypeCode::TypeIndex, 64));
  Instructions = {RefNull, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU,        // Code section
      0x07U,        // Content size = 7
      0x01U,        // Vector length = 1
      0x05U,        // Code segment size = 5
      0x00U,        // Local vec(0)
      0xD0U,        // OpCode Ref__null.
      0xC0U, 0x00U, // Heap type index 64.
      0x0BU         // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  // The remaining opcodes of this group, with their immediates zeroed.
  // The cases above cover the non-zero values.

  Instructions = {WasmEdge::AST::Instruction(WasmEdge::OpCode::Ref__func),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::Ref__is_null),
                  End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU, 0x07U, 0x01U, 0x05U, 0x00U, // Framing.
      0xD2U, 0x00U,                      // Ref__func.
      0xD1U,                             // Ref__is_null.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);
}

TEST(SerializeInstructionTest, SerializeParametricInstruction) {

  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  std::vector<WasmEdge::AST::Instruction> Instructions;

  // 8. Test parametric instructions.
  //
  //   1.  Serialize valid select_t instruction with value type list.

  WasmEdge::AST::Instruction SelectT(WasmEdge::OpCode::Select_t);
  WasmEdge::AST::Instruction End(WasmEdge::OpCode::End);

  SelectT.setValTypeListSize(2);
  SelectT.getValTypeList()[0] = WasmEdge::TypeCode::I32;
  SelectT.getValTypeList()[1] = WasmEdge::TypeCode::I64;
  Instructions = {SelectT, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
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

  // The remaining opcodes of this group, with their immediates zeroed.
  // The cases above cover the non-zero values.

  Instructions = {WasmEdge::AST::Instruction(WasmEdge::OpCode::Drop),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::Select), End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU, 0x06U, 0x01U, 0x04U, 0x00U, // Framing.
      0x1AU,                             // Drop.
      0x1BU,                             // Select.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);
}

TEST(SerializeInstructionTest, SerializeVariableInstruction) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  std::vector<WasmEdge::AST::Instruction> Instructions;

  // 9. Test variable instructions.
  //
  //   1.  Serialize valid local or global index.

  WasmEdge::AST::Instruction LocalGet(WasmEdge::OpCode::Local__get);
  WasmEdge::AST::Instruction End(WasmEdge::OpCode::End);

  LocalGet.getTargetIndex() = 0xFFFFFFFFU;
  Instructions = {LocalGet, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
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

  // The remaining opcodes of this group, with their immediates zeroed.
  // The cases above cover the non-zero values.

  Instructions = {WasmEdge::AST::Instruction(WasmEdge::OpCode::Global__get),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::Global__set),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::Local__set),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::Local__tee),
                  End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU, 0x0CU, 0x01U, 0x0AU, 0x00U, // Framing.
      0x23U, 0x00U,                      // Global__get.
      0x24U, 0x00U,                      // Global__set.
      0x21U, 0x00U,                      // Local__set.
      0x22U, 0x00U,                      // Local__tee.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);
}

TEST(SerializeInstructionTest, SerializeTableInstruction) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  std::vector<WasmEdge::AST::Instruction> Instructions;

  // 10. Test table instructions.
  //
  //   1.  Serialize table_get instruction.
  //   2.  Serialize table_init instruction.
  //   3.  Serialize table_copy instruction with distinct source and
  //   destination.

  WasmEdge::AST::Instruction TableGet(WasmEdge::OpCode::Table__get);
  WasmEdge::AST::Instruction TableInit(WasmEdge::OpCode::Table__init);
  WasmEdge::AST::Instruction TableCopy(WasmEdge::OpCode::Table__copy);
  WasmEdge::AST::Instruction End(WasmEdge::OpCode::End);

  TableGet.getTargetIndex() = 0xFFFFFFFFU;
  Instructions = {TableGet, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
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
  Ser.serializeSection(createCodeSec(Instructions), Output);
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

  // table.copy x y encodes both x (destination) and y (source) as u32.
  TableCopy.getTargetIndex() = 0x01U;
  TableCopy.getSourceIndex() = 0x02U;
  Instructions = {TableCopy, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU,        // Code section
      0x08U,        // Content size = 8
      0x01U,        // Vector length = 1
      0x06U,        // Code segment size = 6
      0x00U,        // Local vec(0)
      0xFCU, 0x0EU, // OpCode Table__copy.
      0x01U,        // Destination table index.
      0x02U,        // Source table index.
      0x0BU         // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  // The remaining opcodes of this group, with their immediates zeroed.
  // The cases above cover the non-zero values.

  Instructions = {WasmEdge::AST::Instruction(WasmEdge::OpCode::Elem__drop),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::Table__fill),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::Table__grow),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::Table__set),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::Table__size),
                  End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU, 0x12U, 0x01U, 0x10U, 0x00U, // Framing.
      0xFCU, 0x0DU, 0x00U,               // Elem__drop.
      0xFCU, 0x11U, 0x00U,               // Table__fill.
      0xFCU, 0x0FU, 0x00U,               // Table__grow.
      0x26U, 0x00U,                      // Table__set.
      0xFCU, 0x10U, 0x00U,               // Table__size.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);
}

TEST(SerializeInstructionTest, SerializeMemoryInstruction) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  std::vector<WasmEdge::AST::Instruction> Instructions;

  // 11. Test memory instructions.
  //
  //   1.  Serialize memory_grow instruction.
  //   2.  Serialize i32_load instruction.
  //   3.  Serialize memory_init with a non-zero data segment index.
  //   4.  Serialize memory_copy with both memory indices zero.

  WasmEdge::AST::Instruction MemoryGrow(WasmEdge::OpCode::Memory__grow);
  WasmEdge::AST::Instruction I32Load(WasmEdge::OpCode::I32__load);
  WasmEdge::AST::Instruction MemoryInit(WasmEdge::OpCode::Memory__init);
  WasmEdge::AST::Instruction MemoryCopy(WasmEdge::OpCode::Memory__copy);
  WasmEdge::AST::Instruction End(WasmEdge::OpCode::End);

  Instructions = {MemoryGrow, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
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
  Ser.serializeSection(createCodeSec(Instructions), Output);
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
  Ser.serializeSection(createCodeSec(Instructions), Output);
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

  // memory.init x y encodes the data segment index before the memory index.
  MemoryInit.getSourceIndex() = 0x05U;
  MemoryInit.getTargetIndex() = 0x00U;
  Instructions = {MemoryInit, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU,        // Code section
      0x08U,        // Content size = 8
      0x01U,        // Vector length = 1
      0x06U,        // Code segment size = 6
      0x00U,        // Local vec(0)
      0xFCU, 0x08U, // OpCode Memory__init.
      0x05U,        // Data segment index (SourceIndex).
      0x00U,        // Memory index (TargetIndex, must be 0x00).
      0x0BU         // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  // memory.copy (non-multi-memory) encodes both memory indices as 0x00.
  MemoryCopy.getTargetIndex() = 0x00U;
  MemoryCopy.getSourceIndex() = 0x00U;
  Instructions = {MemoryCopy, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU,        // Code section
      0x08U,        // Content size = 8
      0x01U,        // Vector length = 1
      0x06U,        // Code segment size = 6
      0x00U,        // Local vec(0)
      0xFCU, 0x0AU, // OpCode Memory__copy.
      0x00U,        // Destination memory index (TargetIndex, must be 0x00).
      0x00U,        // Source memory index (SourceIndex, must be 0x00).
      0x0BU         // Expression End.
  };
  EXPECT_EQ(Output, Expected);
  // The offset is a u64, so only a value past the 32-bit range tells the two
  // encodings apart.
  I32Load.getMemoryAlign() = 0x02U;
  I32Load.getMemoryOffset() = 0x1FF3F1F0FULL;
  I32Load.getTargetIndex() = 0x00U;
  Instructions = {I32Load, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU,                             // Code section
      0x0BU,                             // Content size = 11
      0x01U,                             // Vector length = 1
      0x09U,                             // Code segment size = 9
      0x00U,                             // Local vec(0)
      0x28U,                             // OpCode I32__load.
      0x02U,                             // Align.
      0x8FU, 0xBEU, 0xFCU, 0xF9U, 0x1FU, // Offset = 0x1FF3F1F0F.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  // The remaining opcodes of this group, with their immediates zeroed.
  // The cases above cover the non-zero values.

  Instructions = {WasmEdge::AST::Instruction(WasmEdge::OpCode::Data__drop),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::F32__load),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::F32__store),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::F64__load),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::F64__store),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__load16_s),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__load16_u),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__load8_s),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__load8_u),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__store16),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__store8),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__load),
                  End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU, 0x28U, 0x01U, 0x26U, 0x00U, // Framing.
      0xFCU, 0x09U, 0x00U,               // Data__drop.
      0x2AU, 0x00U, 0x00U,               // F32__load.
      0x38U, 0x00U, 0x00U,               // F32__store.
      0x2BU, 0x00U, 0x00U,               // F64__load.
      0x39U, 0x00U, 0x00U,               // F64__store.
      0x2EU, 0x00U, 0x00U,               // I32__load16_s.
      0x2FU, 0x00U, 0x00U,               // I32__load16_u.
      0x2CU, 0x00U, 0x00U,               // I32__load8_s.
      0x2DU, 0x00U, 0x00U,               // I32__load8_u.
      0x3BU, 0x00U, 0x00U,               // I32__store16.
      0x3AU, 0x00U, 0x00U,               // I32__store8.
      0x29U, 0x00U, 0x00U,               // I64__load.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  Instructions = {WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__load16_s),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__load16_u),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__load32_s),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__load32_u),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__load8_s),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__load8_u),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__store),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__store16),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__store32),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__store8),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::Memory__fill),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::Memory__size),
                  End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU, 0x27U, 0x01U, 0x25U, 0x00U, // Framing.
      0x32U, 0x00U, 0x00U,               // I64__load16_s.
      0x33U, 0x00U, 0x00U,               // I64__load16_u.
      0x34U, 0x00U, 0x00U,               // I64__load32_s.
      0x35U, 0x00U, 0x00U,               // I64__load32_u.
      0x30U, 0x00U, 0x00U,               // I64__load8_s.
      0x31U, 0x00U, 0x00U,               // I64__load8_u.
      0x37U, 0x00U, 0x00U,               // I64__store.
      0x3DU, 0x00U, 0x00U,               // I64__store16.
      0x3EU, 0x00U, 0x00U,               // I64__store32.
      0x3CU, 0x00U, 0x00U,               // I64__store8.
      0xFCU, 0x0BU, 0x00U,               // Memory__fill.
      0x3FU, 0x00U,                      // Memory__size.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);
}

TEST(SerializeInstructionTest, SerializeMultiMemoryMemArgInstruction) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  std::vector<WasmEdge::AST::Instruction> Instructions;

  // 12. Test multi-memory memarg encoding.
  //
  //   1.  Serialize i32_load with a non-zero memory index.
  //   2.  Serialize i32_store with a non-zero memory index.
  //   3.  Serialize i32_load with memory index 0 (single-memory encoding).
  //   4.  Serialize i32_load with a non-zero memory index again.

  WasmEdge::AST::Instruction End(WasmEdge::OpCode::End);

  WasmEdge::AST::Instruction I32Load(WasmEdge::OpCode::I32__load);
  I32Load.getMemoryAlign() = 0x02U;
  I32Load.getTargetIndex() = 0x03U;
  I32Load.getMemoryOffset() = 0x10U;
  Instructions = {I32Load, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU, // Code section
      0x08U, // Content size = 8
      0x01U, // Vector length = 1
      0x06U, // Code segment size = 6
      0x00U, // Local vec(0)
      0x28U, // OpCode I32__load.
      0x42U, // Align with multi-memory flag (0x02 + 0x40).
      0x03U, // Memory index.
      0x10U, // Offset.
      0x0BU  // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  WasmEdge::AST::Instruction I32Store(WasmEdge::OpCode::I32__store);
  I32Store.getMemoryAlign() = 0x02U;
  I32Store.getTargetIndex() = 0x03U;
  I32Store.getMemoryOffset() = 0x10U;
  Instructions = {I32Store, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU, // Code section
      0x08U, // Content size = 8
      0x01U, // Vector length = 1
      0x06U, // Code segment size = 6
      0x00U, // Local vec(0)
      0x36U, // OpCode I32__store.
      0x42U, // Align with multi-memory flag (0x02 + 0x40).
      0x03U, // Memory index.
      0x10U, // Offset.
      0x0BU  // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  WasmEdge::AST::Instruction I32LoadMem0(WasmEdge::OpCode::I32__load);
  I32LoadMem0.getMemoryAlign() = 0x02U;
  I32LoadMem0.getTargetIndex() = 0x00U;
  I32LoadMem0.getMemoryOffset() = 0x10U;
  Instructions = {I32LoadMem0, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU, // Code section
      0x07U, // Content size = 7
      0x01U, // Vector length = 1
      0x05U, // Code segment size = 5
      0x00U, // Local vec(0)
      0x28U, // OpCode I32__load.
      0x02U, // Align (no multi-memory flag, memory index 0).
      0x10U, // Offset.
      0x0BU  // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  Instructions = {I32Load, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU, // Code section
      0x08U, // Content size = 8
      0x01U, // Vector length = 1
      0x06U, // Code segment size = 6
      0x00U, // Local vec(0)
      0x28U, // OpCode I32__load.
      0x42U, // Align with the memory index flag (0x02 | 0x40).
      0x03U, // Memory index.
      0x10U, // Offset.
      0x0BU  // Expression End.
  };
  EXPECT_EQ(Output, Expected);
}

TEST(SerializeInstructionTest, SerializeConstInstruction) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  std::vector<WasmEdge::AST::Instruction> Instructions;

  // 13. Test const numeric instructions.
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
  Ser.serializeSection(createCodeSec(Instructions), Output);
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
  Ser.serializeSection(createCodeSec(Instructions), Output);
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
  Ser.serializeSection(createCodeSec(Instructions), Output);
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
  Ser.serializeSection(createCodeSec(Instructions), Output);
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

TEST(SerializeInstructionTest, SerializeNumericInstruction) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  std::vector<WasmEdge::AST::Instruction> Instructions;

  // Test numeric instructions.

  WasmEdge::AST::Instruction End(WasmEdge::OpCode::End);

  // Every opcode of this group, with its immediates zeroed.

  Instructions = {
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F32__abs),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F32__add),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F32__ceil),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F32__convert_i32_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F32__convert_i32_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F32__convert_i64_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F32__convert_i64_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F32__copysign),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F32__demote_f64),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F32__div),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F32__eq),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F32__floor),
      End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU, 0x10U, 0x01U, 0x0EU, 0x00U, // Framing.
      0x8BU,                             // F32__abs.
      0x92U,                             // F32__add.
      0x8DU,                             // F32__ceil.
      0xB2U,                             // F32__convert_i32_s.
      0xB3U,                             // F32__convert_i32_u.
      0xB4U,                             // F32__convert_i64_s.
      0xB5U,                             // F32__convert_i64_u.
      0x98U,                             // F32__copysign.
      0xB6U,                             // F32__demote_f64.
      0x95U,                             // F32__div.
      0x5BU,                             // F32__eq.
      0x8EU,                             // F32__floor.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  Instructions = {
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F32__ge),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F32__gt),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F32__le),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F32__lt),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F32__max),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F32__min),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F32__mul),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F32__ne),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F32__nearest),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F32__neg),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F32__reinterpret_i32),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F32__sqrt),
      End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU, 0x10U, 0x01U, 0x0EU, 0x00U, // Framing.
      0x60U,                             // F32__ge.
      0x5EU,                             // F32__gt.
      0x5FU,                             // F32__le.
      0x5DU,                             // F32__lt.
      0x97U,                             // F32__max.
      0x96U,                             // F32__min.
      0x94U,                             // F32__mul.
      0x5CU,                             // F32__ne.
      0x90U,                             // F32__nearest.
      0x8CU,                             // F32__neg.
      0xBEU,                             // F32__reinterpret_i32.
      0x91U,                             // F32__sqrt.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  Instructions = {
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F32__sub),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F32__trunc),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F64__abs),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F64__add),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F64__ceil),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F64__convert_i32_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F64__convert_i32_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F64__convert_i64_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F64__convert_i64_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F64__copysign),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F64__div),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F64__eq),
      End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU, 0x10U, 0x01U, 0x0EU, 0x00U, // Framing.
      0x93U,                             // F32__sub.
      0x8FU,                             // F32__trunc.
      0x99U,                             // F64__abs.
      0xA0U,                             // F64__add.
      0x9BU,                             // F64__ceil.
      0xB7U,                             // F64__convert_i32_s.
      0xB8U,                             // F64__convert_i32_u.
      0xB9U,                             // F64__convert_i64_s.
      0xBAU,                             // F64__convert_i64_u.
      0xA6U,                             // F64__copysign.
      0xA3U,                             // F64__div.
      0x61U,                             // F64__eq.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  Instructions = {
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F64__floor),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F64__ge),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F64__gt),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F64__le),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F64__lt),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F64__max),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F64__min),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F64__mul),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F64__ne),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F64__nearest),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F64__neg),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F64__promote_f32),
      End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU, 0x10U, 0x01U, 0x0EU, 0x00U, // Framing.
      0x9CU,                             // F64__floor.
      0x66U,                             // F64__ge.
      0x64U,                             // F64__gt.
      0x65U,                             // F64__le.
      0x63U,                             // F64__lt.
      0xA5U,                             // F64__max.
      0xA4U,                             // F64__min.
      0xA2U,                             // F64__mul.
      0x62U,                             // F64__ne.
      0x9EU,                             // F64__nearest.
      0x9AU,                             // F64__neg.
      0xBBU,                             // F64__promote_f32.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  Instructions = {
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F64__reinterpret_i64),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F64__sqrt),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F64__sub),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F64__trunc),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__add),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__and),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__clz),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__ctz),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__div_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__div_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__extend16_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__extend8_s),
      End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU, 0x10U, 0x01U, 0x0EU, 0x00U, // Framing.
      0xBFU,                             // F64__reinterpret_i64.
      0x9FU,                             // F64__sqrt.
      0xA1U,                             // F64__sub.
      0x9DU,                             // F64__trunc.
      0x6AU,                             // I32__add.
      0x71U,                             // I32__and.
      0x67U,                             // I32__clz.
      0x68U,                             // I32__ctz.
      0x6DU,                             // I32__div_s.
      0x6EU,                             // I32__div_u.
      0xC1U,                             // I32__extend16_s.
      0xC0U,                             // I32__extend8_s.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  Instructions = {
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__ge_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__ge_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__gt_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__gt_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__le_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__le_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__lt_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__lt_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__mul),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__or),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__popcnt),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__reinterpret_f32),
      End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU, 0x10U, 0x01U, 0x0EU, 0x00U, // Framing.
      0x4EU,                             // I32__ge_s.
      0x4FU,                             // I32__ge_u.
      0x4AU,                             // I32__gt_s.
      0x4BU,                             // I32__gt_u.
      0x4CU,                             // I32__le_s.
      0x4DU,                             // I32__le_u.
      0x48U,                             // I32__lt_s.
      0x49U,                             // I32__lt_u.
      0x6CU,                             // I32__mul.
      0x72U,                             // I32__or.
      0x69U,                             // I32__popcnt.
      0xBCU,                             // I32__reinterpret_f32.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  Instructions = {
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__rem_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__rem_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__rotl),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__rotr),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__shl),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__shr_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__shr_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__sub),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__trunc_f32_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__trunc_f32_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__trunc_f64_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__trunc_f64_u),
      End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU, 0x10U, 0x01U, 0x0EU, 0x00U, // Framing.
      0x6FU,                             // I32__rem_s.
      0x70U,                             // I32__rem_u.
      0x77U,                             // I32__rotl.
      0x78U,                             // I32__rotr.
      0x74U,                             // I32__shl.
      0x75U,                             // I32__shr_s.
      0x76U,                             // I32__shr_u.
      0x6BU,                             // I32__sub.
      0xA8U,                             // I32__trunc_f32_s.
      0xA9U,                             // I32__trunc_f32_u.
      0xAAU,                             // I32__trunc_f64_s.
      0xABU,                             // I32__trunc_f64_u.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  Instructions = {WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__wrap_i64),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__xor),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__add),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__and),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__clz),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__ctz),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__div_s),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__div_u),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__eq),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__eqz),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__extend16_s),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__extend32_s),
                  End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU, 0x10U, 0x01U, 0x0EU, 0x00U, // Framing.
      0xA7U,                             // I32__wrap_i64.
      0x73U,                             // I32__xor.
      0x7CU,                             // I64__add.
      0x83U,                             // I64__and.
      0x79U,                             // I64__clz.
      0x7AU,                             // I64__ctz.
      0x7FU,                             // I64__div_s.
      0x80U,                             // I64__div_u.
      0x51U,                             // I64__eq.
      0x50U,                             // I64__eqz.
      0xC3U,                             // I64__extend16_s.
      0xC4U,                             // I64__extend32_s.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  Instructions = {
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__extend8_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__extend_i32_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__extend_i32_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__ge_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__ge_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__gt_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__gt_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__le_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__le_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__lt_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__lt_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__mul),
      End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU, 0x10U, 0x01U, 0x0EU, 0x00U, // Framing.
      0xC2U,                             // I64__extend8_s.
      0xACU,                             // I64__extend_i32_s.
      0xADU,                             // I64__extend_i32_u.
      0x59U,                             // I64__ge_s.
      0x5AU,                             // I64__ge_u.
      0x55U,                             // I64__gt_s.
      0x56U,                             // I64__gt_u.
      0x57U,                             // I64__le_s.
      0x58U,                             // I64__le_u.
      0x53U,                             // I64__lt_s.
      0x54U,                             // I64__lt_u.
      0x7EU,                             // I64__mul.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  Instructions = {
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__ne),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__or),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__popcnt),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__reinterpret_f64),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__rem_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__rem_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__rotl),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__rotr),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__shl),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__shr_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__shr_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__sub),
      End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU, 0x10U, 0x01U, 0x0EU, 0x00U, // Framing.
      0x52U,                             // I64__ne.
      0x84U,                             // I64__or.
      0x7BU,                             // I64__popcnt.
      0xBDU,                             // I64__reinterpret_f64.
      0x81U,                             // I64__rem_s.
      0x82U,                             // I64__rem_u.
      0x89U,                             // I64__rotl.
      0x8AU,                             // I64__rotr.
      0x86U,                             // I64__shl.
      0x87U,                             // I64__shr_s.
      0x88U,                             // I64__shr_u.
      0x7DU,                             // I64__sub.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  Instructions = {
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__trunc_f32_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__trunc_f32_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__trunc_f64_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__trunc_f64_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__xor),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::Nop),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::Unreachable),
      End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU, 0x0BU, 0x01U, 0x09U, 0x00U, // Framing.
      0xAEU,                             // I64__trunc_f32_s.
      0xAFU,                             // I64__trunc_f32_u.
      0xB0U,                             // I64__trunc_f64_s.
      0xB1U,                             // I64__trunc_f64_u.
      0x85U,                             // I64__xor.
      0x01U,                             // Nop.
      0x00U,                             // Unreachable.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);
}

TEST(SerializeInstructionTest, SerializeTruncSatInstruction) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  std::vector<WasmEdge::AST::Instruction> Instructions;

  // 14. Test saturating truncation instructions (0xFC prefix, no immediates).
  //
  //   1.  Serialize i32.trunc_sat_f32_s instruction.
  //   2.  Serialize i32.trunc_sat_f32_u instruction.
  //   3.  Serialize i32.trunc_sat_f64_s instruction.
  //   4.  Serialize i32.trunc_sat_f64_u instruction.
  //   5.  Serialize i64.trunc_sat_f32_s instruction.
  //   6.  Serialize i64.trunc_sat_f32_u instruction.
  //   7.  Serialize i64.trunc_sat_f64_s instruction.
  //   8.  Serialize i64.trunc_sat_f64_u instruction.

  WasmEdge::AST::Instruction End(WasmEdge::OpCode::End);

  WasmEdge::AST::Instruction I32TruncSatF32S(
      WasmEdge::OpCode::I32__trunc_sat_f32_s);
  WasmEdge::AST::Instruction I32TruncSatF32U(
      WasmEdge::OpCode::I32__trunc_sat_f32_u);
  WasmEdge::AST::Instruction I32TruncSatF64S(
      WasmEdge::OpCode::I32__trunc_sat_f64_s);
  WasmEdge::AST::Instruction I32TruncSatF64U(
      WasmEdge::OpCode::I32__trunc_sat_f64_u);
  WasmEdge::AST::Instruction I64TruncSatF32S(
      WasmEdge::OpCode::I64__trunc_sat_f32_s);
  WasmEdge::AST::Instruction I64TruncSatF32U(
      WasmEdge::OpCode::I64__trunc_sat_f32_u);
  WasmEdge::AST::Instruction I64TruncSatF64S(
      WasmEdge::OpCode::I64__trunc_sat_f64_s);
  WasmEdge::AST::Instruction I64TruncSatF64U(
      WasmEdge::OpCode::I64__trunc_sat_f64_u);

  // 1. i32.trunc_sat_f32_s
  Instructions = {I32TruncSatF32S, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU,        // Code section
      0x06U,        // Content size = 6
      0x01U,        // Vector length = 1
      0x04U,        // Code segment size = 4
      0x00U,        // Local vec(0)
      0xFCU, 0x00U, // OpCode I32__trunc_sat_f32_s.
      0x0BU         // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  // 2. i32.trunc_sat_f32_u
  Instructions = {I32TruncSatF32U, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU,        // Code section
      0x06U,        // Content size = 6
      0x01U,        // Vector length = 1
      0x04U,        // Code segment size = 4
      0x00U,        // Local vec(0)
      0xFCU, 0x01U, // OpCode I32__trunc_sat_f32_u.
      0x0BU         // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  // 3. i32.trunc_sat_f64_s
  Instructions = {I32TruncSatF64S, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU,        // Code section
      0x06U,        // Content size = 6
      0x01U,        // Vector length = 1
      0x04U,        // Code segment size = 4
      0x00U,        // Local vec(0)
      0xFCU, 0x02U, // OpCode I32__trunc_sat_f64_s.
      0x0BU         // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  // 4. i32.trunc_sat_f64_u
  Instructions = {I32TruncSatF64U, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU,        // Code section
      0x06U,        // Content size = 6
      0x01U,        // Vector length = 1
      0x04U,        // Code segment size = 4
      0x00U,        // Local vec(0)
      0xFCU, 0x03U, // OpCode I32__trunc_sat_f64_u.
      0x0BU         // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  // 5. i64.trunc_sat_f32_s
  Instructions = {I64TruncSatF32S, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU,        // Code section
      0x06U,        // Content size = 6
      0x01U,        // Vector length = 1
      0x04U,        // Code segment size = 4
      0x00U,        // Local vec(0)
      0xFCU, 0x04U, // OpCode I64__trunc_sat_f32_s.
      0x0BU         // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  // 6. i64.trunc_sat_f32_u
  Instructions = {I64TruncSatF32U, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU,        // Code section
      0x06U,        // Content size = 6
      0x01U,        // Vector length = 1
      0x04U,        // Code segment size = 4
      0x00U,        // Local vec(0)
      0xFCU, 0x05U, // OpCode I64__trunc_sat_f32_u.
      0x0BU         // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  // 7. i64.trunc_sat_f64_s
  Instructions = {I64TruncSatF64S, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU,        // Code section
      0x06U,        // Content size = 6
      0x01U,        // Vector length = 1
      0x04U,        // Code segment size = 4
      0x00U,        // Local vec(0)
      0xFCU, 0x06U, // OpCode I64__trunc_sat_f64_s.
      0x0BU         // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  // 8. i64.trunc_sat_f64_u
  Instructions = {I64TruncSatF64U, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU,        // Code section
      0x06U,        // Content size = 6
      0x01U,        // Vector length = 1
      0x04U,        // Code segment size = 4
      0x00U,        // Local vec(0)
      0xFCU, 0x07U, // OpCode I64__trunc_sat_f64_u.
      0x0BU         // Expression End.
  };
  EXPECT_EQ(Output, Expected);
}

TEST(SerializeInstructionTest, SerializeSIMDConstInstruction) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  std::vector<WasmEdge::AST::Instruction> Instructions;

  // 15. Test SIMD const and shuffle instructions.
  //
  //   1.  Serialize V128__const instruction.
  //   2.  Serialize I8x16__shuffle instruction.

  WasmEdge::AST::Instruction V128Const(WasmEdge::OpCode::V128__const);
  WasmEdge::AST::Instruction I8x16Shuffle(WasmEdge::OpCode::I8x16__shuffle);
  WasmEdge::AST::Instruction End(WasmEdge::OpCode::End);

  // Use an asymmetric 16-byte pattern so byte-order bugs are detectable.
  WasmEdge::uint128_t Value = 0U;
  for (uint32_t I = 0; I < 16; ++I) {
    Value |= static_cast<WasmEdge::uint128_t>(I + 1) << (I * 8);
  }

  V128Const.setNum(Value);
  Instructions = {V128Const, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU,        // Code section
      0x16U,        // Content size = 22
      0x01U,        // Vector length = 1
      0x14U,        // Code segment size = 20
      0x00U,        // Local vec(0)
      0xFDU, 0x0CU, // OpCode V128__const.
      0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U, 0x08U,
      0x09U, 0x0AU, 0x0BU, 0x0CU, 0x0DU, 0x0EU, 0x0FU, 0x10U,
      0x0BU // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  I8x16Shuffle.setNum(Value);
  Instructions = {I8x16Shuffle, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU,        // Code section
      0x16U,        // Content size = 22
      0x01U,        // Vector length = 1
      0x14U,        // Code segment size = 20
      0x00U,        // Local vec(0)
      0xFDU, 0x0DU, // OpCode I8x16__shuffle.
      0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U, 0x08U,
      0x09U, 0x0AU, 0x0BU, 0x0CU, 0x0DU, 0x0EU, 0x0FU, 0x10U,
      0x0BU // Expression End.
  };
  EXPECT_EQ(Output, Expected);
}

TEST(SerializeInstructionTest, SerializeSIMDMemoryInstruction) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  std::vector<WasmEdge::AST::Instruction> Instructions;

  // 16. Test base SIMD memory instructions.
  //
  //   1.  Serialize v128_load instruction with memarg.
  //   2.  Serialize v128_store instruction with memarg.
  //   3.  Serialize v128_load8_lane instruction with memarg and lane.

  WasmEdge::AST::Instruction V128Load(WasmEdge::OpCode::V128__load);
  WasmEdge::AST::Instruction V128Store(WasmEdge::OpCode::V128__store);
  WasmEdge::AST::Instruction V128Load8Lane(WasmEdge::OpCode::V128__load8_lane);
  WasmEdge::AST::Instruction End(WasmEdge::OpCode::End);

  V128Load.getMemoryAlign() = 0x04U;
  V128Load.getMemoryOffset() = 0x10U;
  Instructions = {V128Load, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU,        // Code section
      0x08U,        // Content size = 8
      0x01U,        // Vector length = 1
      0x06U,        // Code segment size = 6
      0x00U,        // Local vec(0)
      0xFDU, 0x00U, // OpCode V128__load.
      0x04U,        // Align.
      0x10U,        // Offset.
      0x0BU         // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  V128Store.getMemoryAlign() = 0x04U;
  V128Store.getMemoryOffset() = 0x10U;
  Instructions = {V128Store, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU,        // Code section
      0x08U,        // Content size = 8
      0x01U,        // Vector length = 1
      0x06U,        // Code segment size = 6
      0x00U,        // Local vec(0)
      0xFDU, 0x0BU, // OpCode V128__store.
      0x04U,        // Align.
      0x10U,        // Offset.
      0x0BU         // Expression End.
  };
  EXPECT_EQ(Output, Expected);
  V128Load8Lane.getMemoryAlign() = 0x00U;
  V128Load8Lane.getMemoryOffset() = 0x10U;
  V128Load8Lane.getMemoryLane() = 0x0FU;
  Instructions = {V128Load8Lane, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU,        // Code section
      0x09U,        // Content size = 9
      0x01U,        // Vector length = 1
      0x07U,        // Code segment size = 7
      0x00U,        // Local vec(0)
      0xFDU, 0x54U, // OpCode V128__load8_lane.
      0x00U,        // Align.
      0x10U,        // Offset.
      0x0FU,        // Lane index.
      0x0BU         // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  // The remaining opcodes of this group, with their immediates zeroed.
  // The cases above cover the non-zero values.

  Instructions = {
      WasmEdge::AST::Instruction(WasmEdge::OpCode::V128__load16_lane),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::V128__load16_splat),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::V128__load16x4_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::V128__load16x4_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::V128__load32_lane),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::V128__load32_splat),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::V128__load32_zero),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::V128__load32x2_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::V128__load32x2_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::V128__load64_lane),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::V128__load64_splat),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::V128__load64_zero),
      End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU, 0x37U, 0x01U, 0x35U, 0x00U, // Framing.
      0xFDU, 0x55U, 0x00U, 0x00U, 0x00U, // V128__load16_lane.
      0xFDU, 0x08U, 0x00U, 0x00U,        // V128__load16_splat.
      0xFDU, 0x03U, 0x00U, 0x00U,        // V128__load16x4_s.
      0xFDU, 0x04U, 0x00U, 0x00U,        // V128__load16x4_u.
      0xFDU, 0x56U, 0x00U, 0x00U, 0x00U, // V128__load32_lane.
      0xFDU, 0x09U, 0x00U, 0x00U,        // V128__load32_splat.
      0xFDU, 0x5CU, 0x00U, 0x00U,        // V128__load32_zero.
      0xFDU, 0x05U, 0x00U, 0x00U,        // V128__load32x2_s.
      0xFDU, 0x06U, 0x00U, 0x00U,        // V128__load32x2_u.
      0xFDU, 0x57U, 0x00U, 0x00U, 0x00U, // V128__load64_lane.
      0xFDU, 0x0AU, 0x00U, 0x00U,        // V128__load64_splat.
      0xFDU, 0x5DU, 0x00U, 0x00U,        // V128__load64_zero.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  Instructions = {
      WasmEdge::AST::Instruction(WasmEdge::OpCode::V128__load8_splat),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::V128__load8x8_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::V128__load8x8_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::V128__store16_lane),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::V128__store32_lane),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::V128__store64_lane),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::V128__store8_lane),
      End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU, 0x24U, 0x01U, 0x22U, 0x00U, // Framing.
      0xFDU, 0x07U, 0x00U, 0x00U,        // V128__load8_splat.
      0xFDU, 0x01U, 0x00U, 0x00U,        // V128__load8x8_s.
      0xFDU, 0x02U, 0x00U, 0x00U,        // V128__load8x8_u.
      0xFDU, 0x59U, 0x00U, 0x00U, 0x00U, // V128__store16_lane.
      0xFDU, 0x5AU, 0x00U, 0x00U, 0x00U, // V128__store32_lane.
      0xFDU, 0x5BU, 0x00U, 0x00U, 0x00U, // V128__store64_lane.
      0xFDU, 0x58U, 0x00U, 0x00U, 0x00U, // V128__store8_lane.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);
}

TEST(SerializeInstructionTest, SerializeSIMDLaneAndNumericInstruction) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  std::vector<WasmEdge::AST::Instruction> Instructions;

  // 17. Test base SIMD lane and numeric instructions.
  //
  //   1.  Serialize i8x16_splat instruction.
  //   2.  Serialize i8x16_add instruction.
  //   3.  Serialize i32x4_mul instruction (two-byte LEB128 opcode suffix).
  //   4.  Serialize f32x4_add instruction (two-byte LEB128 opcode suffix).
  //   5.  Serialize i8x16_extract_lane_s instruction with lane immediate.
  //   6.  Serialize i8x16_replace_lane instruction with lane immediate.

  WasmEdge::AST::Instruction I8x16Splat(WasmEdge::OpCode::I8x16__splat);
  WasmEdge::AST::Instruction I8x16Add(WasmEdge::OpCode::I8x16__add);
  WasmEdge::AST::Instruction I32x4Mul(WasmEdge::OpCode::I32x4__mul);
  WasmEdge::AST::Instruction F32x4Add(WasmEdge::OpCode::F32x4__add);
  WasmEdge::AST::Instruction I8x16ExtractLaneS(
      WasmEdge::OpCode::I8x16__extract_lane_s);
  WasmEdge::AST::Instruction I8x16ReplaceLane(
      WasmEdge::OpCode::I8x16__replace_lane);
  WasmEdge::AST::Instruction End(WasmEdge::OpCode::End);

  Instructions = {I8x16Splat, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU,        // Code section
      0x06U,        // Content size = 6
      0x01U,        // Vector length = 1
      0x04U,        // Code segment size = 4
      0x00U,        // Local vec(0)
      0xFDU, 0x0FU, // OpCode I8x16__splat.
      0x0BU         // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  Instructions = {I8x16Add, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU,        // Code section
      0x06U,        // Content size = 6
      0x01U,        // Vector length = 1
      0x04U,        // Code segment size = 4
      0x00U,        // Local vec(0)
      0xFDU, 0x6EU, // OpCode I8x16__add.
      0x0BU         // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  Instructions = {I32x4Mul, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU,               // Code section
      0x07U,               // Content size = 7
      0x01U,               // Vector length = 1
      0x05U,               // Code segment size = 5
      0x00U,               // Local vec(0)
      0xFDU, 0xB5U, 0x01U, // OpCode I32x4__mul.
      0x0BU                // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  Instructions = {F32x4Add, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU,               // Code section
      0x07U,               // Content size = 7
      0x01U,               // Vector length = 1
      0x05U,               // Code segment size = 5
      0x00U,               // Local vec(0)
      0xFDU, 0xE4U, 0x01U, // OpCode F32x4__add.
      0x0BU                // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  I8x16ExtractLaneS.getMemoryLane() = 0x05U;
  Instructions = {I8x16ExtractLaneS, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU,        // Code section
      0x07U,        // Content size = 7
      0x01U,        // Vector length = 1
      0x05U,        // Code segment size = 5
      0x00U,        // Local vec(0)
      0xFDU, 0x15U, // OpCode I8x16__extract_lane_s.
      0x05U,        // Lane index.
      0x0BU         // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  I8x16ReplaceLane.getMemoryLane() = 0x05U;
  Instructions = {I8x16ReplaceLane, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU,        // Code section
      0x07U,        // Content size = 7
      0x01U,        // Vector length = 1
      0x05U,        // Code segment size = 5
      0x00U,        // Local vec(0)
      0xFDU, 0x17U, // OpCode I8x16__replace_lane.
      0x05U,        // Lane index.
      0x0BU         // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  // The remaining opcodes of this group, with their immediates zeroed.
  // The cases above cover the non-zero values.

  Instructions = {
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F32x4__extract_lane),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F32x4__replace_lane),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F64x2__extract_lane),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F64x2__replace_lane),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I16x8__extract_lane_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I16x8__extract_lane_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I16x8__replace_lane),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32x4__extract_lane),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32x4__replace_lane),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64x2__extract_lane),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64x2__replace_lane),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I8x16__extract_lane_u),
      End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU, 0x28U, 0x01U, 0x26U, 0x00U, // Framing.
      0xFDU, 0x1FU, 0x00U,               // F32x4__extract_lane.
      0xFDU, 0x20U, 0x00U,               // F32x4__replace_lane.
      0xFDU, 0x21U, 0x00U,               // F64x2__extract_lane.
      0xFDU, 0x22U, 0x00U,               // F64x2__replace_lane.
      0xFDU, 0x18U, 0x00U,               // I16x8__extract_lane_s.
      0xFDU, 0x19U, 0x00U,               // I16x8__extract_lane_u.
      0xFDU, 0x1AU, 0x00U,               // I16x8__replace_lane.
      0xFDU, 0x1BU, 0x00U,               // I32x4__extract_lane.
      0xFDU, 0x1CU, 0x00U,               // I32x4__replace_lane.
      0xFDU, 0x1DU, 0x00U,               // I64x2__extract_lane.
      0xFDU, 0x1EU, 0x00U,               // I64x2__replace_lane.
      0xFDU, 0x16U, 0x00U,               // I8x16__extract_lane_u.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);
}

TEST(SerializeInstructionTest, SerializeSIMDNumericInstruction) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  std::vector<WasmEdge::AST::Instruction> Instructions;

  // Test SIMD numeric instructions.

  WasmEdge::AST::Instruction End(WasmEdge::OpCode::End);

  // Every opcode of this group, with its immediates zeroed.

  Instructions = {
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F32x4__abs),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F32x4__ceil),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F32x4__convert_i32x4_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F32x4__convert_i32x4_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F32x4__demote_f64x2_zero),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F32x4__div),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F32x4__eq),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F32x4__floor),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F32x4__ge),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F32x4__gt),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F32x4__le),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F32x4__lt),
      End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU, 0x20U, 0x01U, 0x1EU, 0x00U, // Framing.
      0xFDU, 0xE0U, 0x01U,               // F32x4__abs.
      0xFDU, 0x67U,                      // F32x4__ceil.
      0xFDU, 0xFAU, 0x01U,               // F32x4__convert_i32x4_s.
      0xFDU, 0xFBU, 0x01U,               // F32x4__convert_i32x4_u.
      0xFDU, 0x5EU,                      // F32x4__demote_f64x2_zero.
      0xFDU, 0xE7U, 0x01U,               // F32x4__div.
      0xFDU, 0x41U,                      // F32x4__eq.
      0xFDU, 0x68U,                      // F32x4__floor.
      0xFDU, 0x46U,                      // F32x4__ge.
      0xFDU, 0x44U,                      // F32x4__gt.
      0xFDU, 0x45U,                      // F32x4__le.
      0xFDU, 0x43U,                      // F32x4__lt.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  Instructions = {WasmEdge::AST::Instruction(WasmEdge::OpCode::F32x4__max),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::F32x4__min),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::F32x4__mul),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::F32x4__ne),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::F32x4__nearest),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::F32x4__neg),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::F32x4__pmax),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::F32x4__pmin),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::F32x4__splat),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::F32x4__sqrt),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::F32x4__sub),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::F32x4__trunc),
                  End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU, 0x24U, 0x01U, 0x22U, 0x00U, // Framing.
      0xFDU, 0xE9U, 0x01U,               // F32x4__max.
      0xFDU, 0xE8U, 0x01U,               // F32x4__min.
      0xFDU, 0xE6U, 0x01U,               // F32x4__mul.
      0xFDU, 0x42U,                      // F32x4__ne.
      0xFDU, 0x6AU,                      // F32x4__nearest.
      0xFDU, 0xE1U, 0x01U,               // F32x4__neg.
      0xFDU, 0xEBU, 0x01U,               // F32x4__pmax.
      0xFDU, 0xEAU, 0x01U,               // F32x4__pmin.
      0xFDU, 0x13U,                      // F32x4__splat.
      0xFDU, 0xE3U, 0x01U,               // F32x4__sqrt.
      0xFDU, 0xE5U, 0x01U,               // F32x4__sub.
      0xFDU, 0x69U,                      // F32x4__trunc.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  Instructions = {
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F64x2__abs),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F64x2__add),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F64x2__ceil),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F64x2__convert_low_i32x4_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F64x2__convert_low_i32x4_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F64x2__div),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F64x2__eq),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F64x2__floor),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F64x2__ge),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F64x2__gt),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F64x2__le),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F64x2__lt),
      End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU, 0x21U, 0x01U, 0x1FU, 0x00U, // Framing.
      0xFDU, 0xECU, 0x01U,               // F64x2__abs.
      0xFDU, 0xF0U, 0x01U,               // F64x2__add.
      0xFDU, 0x74U,                      // F64x2__ceil.
      0xFDU, 0xFEU, 0x01U,               // F64x2__convert_low_i32x4_s.
      0xFDU, 0xFFU, 0x01U,               // F64x2__convert_low_i32x4_u.
      0xFDU, 0xF3U, 0x01U,               // F64x2__div.
      0xFDU, 0x47U,                      // F64x2__eq.
      0xFDU, 0x75U,                      // F64x2__floor.
      0xFDU, 0x4CU,                      // F64x2__ge.
      0xFDU, 0x4AU,                      // F64x2__gt.
      0xFDU, 0x4BU,                      // F64x2__le.
      0xFDU, 0x49U,                      // F64x2__lt.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  Instructions = {
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F64x2__max),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F64x2__min),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F64x2__mul),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F64x2__ne),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F64x2__nearest),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F64x2__neg),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F64x2__pmax),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F64x2__pmin),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F64x2__promote_low_f32x4),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F64x2__splat),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F64x2__sqrt),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F64x2__sub),
      End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU, 0x25U, 0x01U, 0x23U, 0x00U, // Framing.
      0xFDU, 0xF5U, 0x01U,               // F64x2__max.
      0xFDU, 0xF4U, 0x01U,               // F64x2__min.
      0xFDU, 0xF2U, 0x01U,               // F64x2__mul.
      0xFDU, 0x48U,                      // F64x2__ne.
      0xFDU, 0x94U, 0x01U,               // F64x2__nearest.
      0xFDU, 0xEDU, 0x01U,               // F64x2__neg.
      0xFDU, 0xF7U, 0x01U,               // F64x2__pmax.
      0xFDU, 0xF6U, 0x01U,               // F64x2__pmin.
      0xFDU, 0x5FU,                      // F64x2__promote_low_f32x4.
      0xFDU, 0x14U,                      // F64x2__splat.
      0xFDU, 0xEFU, 0x01U,               // F64x2__sqrt.
      0xFDU, 0xF1U, 0x01U,               // F64x2__sub.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  Instructions = {
      WasmEdge::AST::Instruction(WasmEdge::OpCode::F64x2__trunc),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I16x8__abs),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I16x8__add),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I16x8__add_sat_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I16x8__add_sat_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I16x8__all_true),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I16x8__avgr_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I16x8__bitmask),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I16x8__eq),
      WasmEdge::AST::Instruction(
          WasmEdge::OpCode::I16x8__extadd_pairwise_i8x16_s),
      WasmEdge::AST::Instruction(
          WasmEdge::OpCode::I16x8__extadd_pairwise_i8x16_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I16x8__extend_high_i8x16_s),
      End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU, 0x24U, 0x01U, 0x22U, 0x00U, // Framing.
      0xFDU, 0x7AU,                      // F64x2__trunc.
      0xFDU, 0x80U, 0x01U,               // I16x8__abs.
      0xFDU, 0x8EU, 0x01U,               // I16x8__add.
      0xFDU, 0x8FU, 0x01U,               // I16x8__add_sat_s.
      0xFDU, 0x90U, 0x01U,               // I16x8__add_sat_u.
      0xFDU, 0x83U, 0x01U,               // I16x8__all_true.
      0xFDU, 0x9BU, 0x01U,               // I16x8__avgr_u.
      0xFDU, 0x84U, 0x01U,               // I16x8__bitmask.
      0xFDU, 0x2DU,                      // I16x8__eq.
      0xFDU, 0x7CU,                      // I16x8__extadd_pairwise_i8x16_s.
      0xFDU, 0x7DU,                      // I16x8__extadd_pairwise_i8x16_u.
      0xFDU, 0x88U, 0x01U,               // I16x8__extend_high_i8x16_s.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  Instructions = {
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I16x8__extend_high_i8x16_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I16x8__extend_low_i8x16_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I16x8__extend_low_i8x16_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I16x8__extmul_high_i8x16_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I16x8__extmul_high_i8x16_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I16x8__extmul_low_i8x16_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I16x8__extmul_low_i8x16_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I16x8__ge_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I16x8__ge_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I16x8__gt_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I16x8__gt_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I16x8__le_s),
      End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU, 0x23U, 0x01U, 0x21U, 0x00U, // Framing.
      0xFDU, 0x8AU, 0x01U,               // I16x8__extend_high_i8x16_u.
      0xFDU, 0x87U, 0x01U,               // I16x8__extend_low_i8x16_s.
      0xFDU, 0x89U, 0x01U,               // I16x8__extend_low_i8x16_u.
      0xFDU, 0x9DU, 0x01U,               // I16x8__extmul_high_i8x16_s.
      0xFDU, 0x9FU, 0x01U,               // I16x8__extmul_high_i8x16_u.
      0xFDU, 0x9CU, 0x01U,               // I16x8__extmul_low_i8x16_s.
      0xFDU, 0x9EU, 0x01U,               // I16x8__extmul_low_i8x16_u.
      0xFDU, 0x35U,                      // I16x8__ge_s.
      0xFDU, 0x36U,                      // I16x8__ge_u.
      0xFDU, 0x31U,                      // I16x8__gt_s.
      0xFDU, 0x32U,                      // I16x8__gt_u.
      0xFDU, 0x33U,                      // I16x8__le_s.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  Instructions = {
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I16x8__le_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I16x8__lt_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I16x8__lt_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I16x8__max_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I16x8__max_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I16x8__min_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I16x8__min_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I16x8__mul),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I16x8__narrow_i32x4_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I16x8__narrow_i32x4_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I16x8__ne),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I16x8__neg),
      End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU, 0x24U, 0x01U, 0x22U, 0x00U, // Framing.
      0xFDU, 0x34U,                      // I16x8__le_u.
      0xFDU, 0x2FU,                      // I16x8__lt_s.
      0xFDU, 0x30U,                      // I16x8__lt_u.
      0xFDU, 0x98U, 0x01U,               // I16x8__max_s.
      0xFDU, 0x99U, 0x01U,               // I16x8__max_u.
      0xFDU, 0x96U, 0x01U,               // I16x8__min_s.
      0xFDU, 0x97U, 0x01U,               // I16x8__min_u.
      0xFDU, 0x95U, 0x01U,               // I16x8__mul.
      0xFDU, 0x85U, 0x01U,               // I16x8__narrow_i32x4_s.
      0xFDU, 0x86U, 0x01U,               // I16x8__narrow_i32x4_u.
      0xFDU, 0x2EU,                      // I16x8__ne.
      0xFDU, 0x81U, 0x01U,               // I16x8__neg.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  Instructions = {
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I16x8__q15mulr_sat_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I16x8__shl),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I16x8__shr_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I16x8__shr_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I16x8__splat),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I16x8__sub),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I16x8__sub_sat_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I16x8__sub_sat_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32x4__abs),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32x4__add),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32x4__all_true),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32x4__bitmask),
      End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU, 0x27U, 0x01U, 0x25U, 0x00U, // Framing.
      0xFDU, 0x82U, 0x01U,               // I16x8__q15mulr_sat_s.
      0xFDU, 0x8BU, 0x01U,               // I16x8__shl.
      0xFDU, 0x8CU, 0x01U,               // I16x8__shr_s.
      0xFDU, 0x8DU, 0x01U,               // I16x8__shr_u.
      0xFDU, 0x10U,                      // I16x8__splat.
      0xFDU, 0x91U, 0x01U,               // I16x8__sub.
      0xFDU, 0x92U, 0x01U,               // I16x8__sub_sat_s.
      0xFDU, 0x93U, 0x01U,               // I16x8__sub_sat_u.
      0xFDU, 0xA0U, 0x01U,               // I32x4__abs.
      0xFDU, 0xAEU, 0x01U,               // I32x4__add.
      0xFDU, 0xA3U, 0x01U,               // I32x4__all_true.
      0xFDU, 0xA4U, 0x01U,               // I32x4__bitmask.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  Instructions = {
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32x4__dot_i16x8_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32x4__eq),
      WasmEdge::AST::Instruction(
          WasmEdge::OpCode::I32x4__extadd_pairwise_i16x8_s),
      WasmEdge::AST::Instruction(
          WasmEdge::OpCode::I32x4__extadd_pairwise_i16x8_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32x4__extend_high_i16x8_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32x4__extend_high_i16x8_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32x4__extend_low_i16x8_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32x4__extend_low_i16x8_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32x4__extmul_high_i16x8_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32x4__extmul_high_i16x8_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32x4__extmul_low_i16x8_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32x4__extmul_low_i16x8_u),
      End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU, 0x25U, 0x01U, 0x23U, 0x00U, // Framing.
      0xFDU, 0xBAU, 0x01U,               // I32x4__dot_i16x8_s.
      0xFDU, 0x37U,                      // I32x4__eq.
      0xFDU, 0x7EU,                      // I32x4__extadd_pairwise_i16x8_s.
      0xFDU, 0x7FU,                      // I32x4__extadd_pairwise_i16x8_u.
      0xFDU, 0xA8U, 0x01U,               // I32x4__extend_high_i16x8_s.
      0xFDU, 0xAAU, 0x01U,               // I32x4__extend_high_i16x8_u.
      0xFDU, 0xA7U, 0x01U,               // I32x4__extend_low_i16x8_s.
      0xFDU, 0xA9U, 0x01U,               // I32x4__extend_low_i16x8_u.
      0xFDU, 0xBDU, 0x01U,               // I32x4__extmul_high_i16x8_s.
      0xFDU, 0xBFU, 0x01U,               // I32x4__extmul_high_i16x8_u.
      0xFDU, 0xBCU, 0x01U,               // I32x4__extmul_low_i16x8_s.
      0xFDU, 0xBEU, 0x01U,               // I32x4__extmul_low_i16x8_u.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  Instructions = {WasmEdge::AST::Instruction(WasmEdge::OpCode::I32x4__ge_s),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::I32x4__ge_u),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::I32x4__gt_s),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::I32x4__gt_u),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::I32x4__le_s),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::I32x4__le_u),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::I32x4__lt_s),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::I32x4__lt_u),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::I32x4__max_s),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::I32x4__max_u),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::I32x4__min_s),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::I32x4__min_u),
                  End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU, 0x20U, 0x01U, 0x1EU, 0x00U, // Framing.
      0xFDU, 0x3FU,                      // I32x4__ge_s.
      0xFDU, 0x40U,                      // I32x4__ge_u.
      0xFDU, 0x3BU,                      // I32x4__gt_s.
      0xFDU, 0x3CU,                      // I32x4__gt_u.
      0xFDU, 0x3DU,                      // I32x4__le_s.
      0xFDU, 0x3EU,                      // I32x4__le_u.
      0xFDU, 0x39U,                      // I32x4__lt_s.
      0xFDU, 0x3AU,                      // I32x4__lt_u.
      0xFDU, 0xB8U, 0x01U,               // I32x4__max_s.
      0xFDU, 0xB9U, 0x01U,               // I32x4__max_u.
      0xFDU, 0xB6U, 0x01U,               // I32x4__min_s.
      0xFDU, 0xB7U, 0x01U,               // I32x4__min_u.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  Instructions = {
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32x4__ne),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32x4__neg),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32x4__shl),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32x4__shr_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32x4__shr_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32x4__splat),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32x4__sub),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32x4__trunc_sat_f32x4_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32x4__trunc_sat_f32x4_u),
      WasmEdge::AST::Instruction(
          WasmEdge::OpCode::I32x4__trunc_sat_f64x2_s_zero),
      WasmEdge::AST::Instruction(
          WasmEdge::OpCode::I32x4__trunc_sat_f64x2_u_zero),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64x2__abs),
      End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU, 0x26U, 0x01U, 0x24U, 0x00U, // Framing.
      0xFDU, 0x38U,                      // I32x4__ne.
      0xFDU, 0xA1U, 0x01U,               // I32x4__neg.
      0xFDU, 0xABU, 0x01U,               // I32x4__shl.
      0xFDU, 0xACU, 0x01U,               // I32x4__shr_s.
      0xFDU, 0xADU, 0x01U,               // I32x4__shr_u.
      0xFDU, 0x11U,                      // I32x4__splat.
      0xFDU, 0xB1U, 0x01U,               // I32x4__sub.
      0xFDU, 0xF8U, 0x01U,               // I32x4__trunc_sat_f32x4_s.
      0xFDU, 0xF9U, 0x01U,               // I32x4__trunc_sat_f32x4_u.
      0xFDU, 0xFCU, 0x01U,               // I32x4__trunc_sat_f64x2_s_zero.
      0xFDU, 0xFDU, 0x01U,               // I32x4__trunc_sat_f64x2_u_zero.
      0xFDU, 0xC0U, 0x01U,               // I64x2__abs.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  Instructions = {
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64x2__add),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64x2__all_true),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64x2__bitmask),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64x2__eq),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64x2__extend_high_i32x4_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64x2__extend_high_i32x4_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64x2__extend_low_i32x4_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64x2__extend_low_i32x4_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64x2__extmul_high_i32x4_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64x2__extmul_high_i32x4_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64x2__extmul_low_i32x4_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64x2__extmul_low_i32x4_u),
      End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU, 0x28U, 0x01U, 0x26U, 0x00U, // Framing.
      0xFDU, 0xCEU, 0x01U,               // I64x2__add.
      0xFDU, 0xC3U, 0x01U,               // I64x2__all_true.
      0xFDU, 0xC4U, 0x01U,               // I64x2__bitmask.
      0xFDU, 0xD6U, 0x01U,               // I64x2__eq.
      0xFDU, 0xC8U, 0x01U,               // I64x2__extend_high_i32x4_s.
      0xFDU, 0xCAU, 0x01U,               // I64x2__extend_high_i32x4_u.
      0xFDU, 0xC7U, 0x01U,               // I64x2__extend_low_i32x4_s.
      0xFDU, 0xC9U, 0x01U,               // I64x2__extend_low_i32x4_u.
      0xFDU, 0xDDU, 0x01U,               // I64x2__extmul_high_i32x4_s.
      0xFDU, 0xDFU, 0x01U,               // I64x2__extmul_high_i32x4_u.
      0xFDU, 0xDCU, 0x01U,               // I64x2__extmul_low_i32x4_s.
      0xFDU, 0xDEU, 0x01U,               // I64x2__extmul_low_i32x4_u.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  Instructions = {WasmEdge::AST::Instruction(WasmEdge::OpCode::I64x2__ge_s),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::I64x2__gt_s),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::I64x2__le_s),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::I64x2__lt_s),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::I64x2__mul),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::I64x2__ne),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::I64x2__neg),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::I64x2__shl),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::I64x2__shr_s),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::I64x2__shr_u),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::I64x2__splat),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::I64x2__sub),
                  End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU, 0x27U, 0x01U, 0x25U, 0x00U, // Framing.
      0xFDU, 0xDBU, 0x01U,               // I64x2__ge_s.
      0xFDU, 0xD9U, 0x01U,               // I64x2__gt_s.
      0xFDU, 0xDAU, 0x01U,               // I64x2__le_s.
      0xFDU, 0xD8U, 0x01U,               // I64x2__lt_s.
      0xFDU, 0xD5U, 0x01U,               // I64x2__mul.
      0xFDU, 0xD7U, 0x01U,               // I64x2__ne.
      0xFDU, 0xC1U, 0x01U,               // I64x2__neg.
      0xFDU, 0xCBU, 0x01U,               // I64x2__shl.
      0xFDU, 0xCCU, 0x01U,               // I64x2__shr_s.
      0xFDU, 0xCDU, 0x01U,               // I64x2__shr_u.
      0xFDU, 0x12U,                      // I64x2__splat.
      0xFDU, 0xD1U, 0x01U,               // I64x2__sub.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  Instructions = {
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I8x16__abs),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I8x16__add_sat_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I8x16__add_sat_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I8x16__all_true),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I8x16__avgr_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I8x16__bitmask),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I8x16__eq),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I8x16__ge_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I8x16__ge_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I8x16__gt_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I8x16__gt_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I8x16__le_s),
      End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU, 0x1CU, 0x01U, 0x1AU, 0x00U, // Framing.
      0xFDU, 0x60U,                      // I8x16__abs.
      0xFDU, 0x6FU,                      // I8x16__add_sat_s.
      0xFDU, 0x70U,                      // I8x16__add_sat_u.
      0xFDU, 0x63U,                      // I8x16__all_true.
      0xFDU, 0x7BU,                      // I8x16__avgr_u.
      0xFDU, 0x64U,                      // I8x16__bitmask.
      0xFDU, 0x23U,                      // I8x16__eq.
      0xFDU, 0x2BU,                      // I8x16__ge_s.
      0xFDU, 0x2CU,                      // I8x16__ge_u.
      0xFDU, 0x27U,                      // I8x16__gt_s.
      0xFDU, 0x28U,                      // I8x16__gt_u.
      0xFDU, 0x29U,                      // I8x16__le_s.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  Instructions = {
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I8x16__le_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I8x16__lt_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I8x16__lt_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I8x16__max_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I8x16__max_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I8x16__min_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I8x16__min_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I8x16__narrow_i16x8_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I8x16__narrow_i16x8_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I8x16__ne),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I8x16__neg),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I8x16__popcnt),
      End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU, 0x1CU, 0x01U, 0x1AU, 0x00U, // Framing.
      0xFDU, 0x2AU,                      // I8x16__le_u.
      0xFDU, 0x25U,                      // I8x16__lt_s.
      0xFDU, 0x26U,                      // I8x16__lt_u.
      0xFDU, 0x78U,                      // I8x16__max_s.
      0xFDU, 0x79U,                      // I8x16__max_u.
      0xFDU, 0x76U,                      // I8x16__min_s.
      0xFDU, 0x77U,                      // I8x16__min_u.
      0xFDU, 0x65U,                      // I8x16__narrow_i16x8_s.
      0xFDU, 0x66U,                      // I8x16__narrow_i16x8_u.
      0xFDU, 0x24U,                      // I8x16__ne.
      0xFDU, 0x61U,                      // I8x16__neg.
      0xFDU, 0x62U,                      // I8x16__popcnt.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  Instructions = {
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I8x16__shl),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I8x16__shr_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I8x16__shr_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I8x16__sub),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I8x16__sub_sat_s),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I8x16__sub_sat_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I8x16__swizzle),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::V128__and),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::V128__andnot),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::V128__any_true),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::V128__bitselect),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::V128__not),
      End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU, 0x1CU, 0x01U, 0x1AU, 0x00U, // Framing.
      0xFDU, 0x6BU,                      // I8x16__shl.
      0xFDU, 0x6CU,                      // I8x16__shr_s.
      0xFDU, 0x6DU,                      // I8x16__shr_u.
      0xFDU, 0x71U,                      // I8x16__sub.
      0xFDU, 0x72U,                      // I8x16__sub_sat_s.
      0xFDU, 0x73U,                      // I8x16__sub_sat_u.
      0xFDU, 0x0EU,                      // I8x16__swizzle.
      0xFDU, 0x4EU,                      // V128__and.
      0xFDU, 0x4FU,                      // V128__andnot.
      0xFDU, 0x53U,                      // V128__any_true.
      0xFDU, 0x52U,                      // V128__bitselect.
      0xFDU, 0x4DU,                      // V128__not.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  Instructions = {WasmEdge::AST::Instruction(WasmEdge::OpCode::V128__or),
                  WasmEdge::AST::Instruction(WasmEdge::OpCode::V128__xor), End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU, 0x08U, 0x01U, 0x06U, 0x00U, // Framing.
      0xFDU, 0x50U,                      // V128__or.
      0xFDU, 0x51U,                      // V128__xor.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);
}

TEST(SerializeInstructionTest, SerializeSwizzleInstruction) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  std::vector<WasmEdge::AST::Instruction> Instructions;

  // 18. Test swizzle instruction.
  //
  //   1.  Serialize I8x16__relaxed_swizzle instruction.

  WasmEdge::AST::Instruction I8x16RelaxedSwizzle(
      WasmEdge::OpCode::I8x16__relaxed_swizzle);
  WasmEdge::AST::Instruction End(WasmEdge::OpCode::End);

  Instructions = {I8x16RelaxedSwizzle, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU,        // Code section
      0x07U,        // Content size = 7
      0x01U,        // Vector length = 1
      0x05U,        // Code segment size = 5
      0x00U,        // Local vec(0)
      0xFDU,        // SIMD/relaxed-SIMD prefix.
      0x80U, 0x02U, // OpCode I8x16__relaxed_swizzle.
      0x0BU         // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  Output = {};
  Instructions = {I8x16RelaxedSwizzle, End};
}

TEST(SerializeInstructionTest, SerializeTruncInstruction) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  std::vector<WasmEdge::AST::Instruction> Instructions;

  // 19. Test trunc instruction.
  //
  //   1.  Serialize I32x4__relaxed_trunc_f32x4_s instruction.
  //   2.  Serialize I32x4__relaxed_trunc_f32x4_u instruction.
  //   3.  Serialize I32x4__relaxed_trunc_f64x2_s_zero instruction.
  //   4.  Serialize I32x4__relaxed_trunc_f64x2_u_zero instruction.

  WasmEdge::AST::Instruction I8x16RelaxedTruncF32x4S(
      WasmEdge::OpCode::I32x4__relaxed_trunc_f32x4_s);
  WasmEdge::AST::Instruction I8x16RelaxedTruncF32x4U(
      WasmEdge::OpCode::I32x4__relaxed_trunc_f32x4_u);
  WasmEdge::AST::Instruction I32x4RelaxedTruncF64x2SZero(
      WasmEdge::OpCode::I32x4__relaxed_trunc_f64x2_s_zero);
  WasmEdge::AST::Instruction I32x4RelaxedTruncF64x2UZero(
      WasmEdge::OpCode::I32x4__relaxed_trunc_f64x2_u_zero);
  WasmEdge::AST::Instruction End(WasmEdge::OpCode::End);

  Instructions = {I8x16RelaxedTruncF32x4S, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU,        // Code section
      0x07U,        // Content size = 7
      0x01U,        // Vector length = 1
      0x05U,        // Code segment size = 5
      0x00U,        // Local vec(0)
      0xFDU,        // SIMD/relaxed-SIMD prefix.
      0x81U, 0x02U, // OpCode I32x4__relaxed_trunc_f32x4_s.
      0x0BU         // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  Instructions = {I8x16RelaxedTruncF32x4U, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected[6] = 0x82U; // OpCode I32x4__relaxed_trunc_f32x4_u
  EXPECT_EQ(Output, Expected);

  Instructions = {I32x4RelaxedTruncF64x2SZero, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected[6] = 0x83U; // OpCode I32x4__relaxed_trunc_f64x2_s_zero
  EXPECT_EQ(Output, Expected);

  Instructions = {I32x4RelaxedTruncF64x2UZero, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected[6] = 0x84U; // OpCode I32x4__relaxed_trunc_f64x2_u_zero
  EXPECT_EQ(Output, Expected);

  Instructions = {I32x4RelaxedTruncF64x2UZero, End};
  Output = {};
}

TEST(SerializeInstructionTest, SerializeMulAddInstruction) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  std::vector<WasmEdge::AST::Instruction> Instructions;

  // 20. Test multiply-add instruction.
  //
  //   1.  Serialize F32x4__relaxed_madd instruction.
  //   2.  Serialize F32x4__relaxed_nmadd instruction.
  //   3.  Serialize F64x2__relaxed_madd instruction.
  //   4.  Serialize F64x2__relaxed_nmadd instruction.

  WasmEdge::AST::Instruction F32x4RelaxedMadd(
      WasmEdge::OpCode::F32x4__relaxed_madd);
  WasmEdge::AST::Instruction F32x4RelaxedNMadd(
      WasmEdge::OpCode::F32x4__relaxed_nmadd);
  WasmEdge::AST::Instruction F64x2RelaxedMadd(
      WasmEdge::OpCode::F64x2__relaxed_madd);
  WasmEdge::AST::Instruction F64x2RelaxedNMadd(
      WasmEdge::OpCode::F64x2__relaxed_nmadd);
  WasmEdge::AST::Instruction End(WasmEdge::OpCode::End);

  Instructions = {F32x4RelaxedMadd, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU,        // Code section
      0x07U,        // Content size = 7
      0x01U,        // Vector length = 1
      0x05U,        // Code segment size = 5
      0x00U,        // Local vec(0)
      0xFDU,        // SIMD/relaxed-SIMD prefix.
      0x85U, 0x02U, // OpCode F32x4__relaxed_madd.
      0x0BU         // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  Instructions = {F32x4RelaxedNMadd, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected[6] = 0x86U; // OpCode F32x4__relaxed_nmadd.
  EXPECT_EQ(Output, Expected);

  Instructions = {F64x2RelaxedMadd, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected[6] = 0x87U; // OpCode F64x2__relaxed_madd.
  EXPECT_EQ(Output, Expected);

  Instructions = {F64x2RelaxedNMadd, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected[6] = 0x88U; // OpCode F64x2__relaxed_nmadd.
  EXPECT_EQ(Output, Expected);

  Instructions = {F64x2RelaxedNMadd, End};
  Output = {};
}

TEST(SerializeInstructionTest, SerializeLaneSelectInstruction) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  std::vector<WasmEdge::AST::Instruction> Instructions;

  // 21. Test laneselect instruction.
  //
  //   1.  Serialize I8x16__relaxed_laneselect instruction.
  //   2.  Serialize I16x8__relaxed_laneselect instruction.
  //   3.  Serialize I32x4__relaxed_laneselect instruction.
  //   4.  Serialize I64x2__relaxed_laneselect instruction.

  WasmEdge::AST::Instruction I8x16RelaxedLaneSelect(
      WasmEdge::OpCode::I8x16__relaxed_laneselect);
  WasmEdge::AST::Instruction I16x8RelaxedLaneSelect(
      WasmEdge::OpCode::I16x8__relaxed_laneselect);
  WasmEdge::AST::Instruction I32x4RelaxedLaneSelect(
      WasmEdge::OpCode::I32x4__relaxed_laneselect);
  WasmEdge::AST::Instruction I64x2RelaxedLaneSelect(
      WasmEdge::OpCode::I64x2__relaxed_laneselect);
  WasmEdge::AST::Instruction End(WasmEdge::OpCode::End);

  Instructions = {I8x16RelaxedLaneSelect, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU,        // Code section
      0x07U,        // Content size = 7
      0x01U,        // Vector length = 1
      0x05U,        // Code segment size = 5
      0x00U,        // Local vec(0)
      0xFDU,        // SIMD/relaxed-SIMD prefix.
      0x89U, 0x02U, // OpCode I8x16__relaxed_laneselect.
      0x0BU         // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  Instructions = {I16x8RelaxedLaneSelect, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected[6] = 0x8AU; // OpCode I16x8__relaxed_laneselect.
  EXPECT_EQ(Output, Expected);

  Instructions = {I32x4RelaxedLaneSelect, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected[6] = 0x8BU; // OpCode I32x4__relaxed_laneselect.
  EXPECT_EQ(Output, Expected);

  Instructions = {I64x2RelaxedLaneSelect, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected[6] = 0x8CU; // OpCode I64x2__relaxed_laneselect.

  Instructions = {I64x2RelaxedLaneSelect, End};
  Output = {};
}

TEST(SerializeInstructionTest, SerializeMinMaxInstruction) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  std::vector<WasmEdge::AST::Instruction> Instructions;

  // 22. Test laneselect instruction.
  //
  //   1.  Serialize F32x4__relaxed_min instruction.
  //   2.  Serialize F32x4__relaxed_max instruction.
  //   3.  Serialize F64x2__relaxed_min instruction.
  //   4.  Serialize F64x2__relaxed_max instruction.

  WasmEdge::AST::Instruction F32x4RelaxedMin(
      WasmEdge::OpCode::F32x4__relaxed_min);
  WasmEdge::AST::Instruction F32x4RelaxedMax(
      WasmEdge::OpCode::F32x4__relaxed_max);
  WasmEdge::AST::Instruction F64x2RelaxedMin(
      WasmEdge::OpCode::F64x2__relaxed_min);
  WasmEdge::AST::Instruction F64x2RelaxedMax(
      WasmEdge::OpCode::F64x2__relaxed_max);
  WasmEdge::AST::Instruction End(WasmEdge::OpCode::End);

  Instructions = {F32x4RelaxedMin, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU,        // Code section
      0x07U,        // Content size = 7
      0x01U,        // Vector length = 1
      0x05U,        // Code segment size = 5
      0x00U,        // Local vec(0)
      0xFDU,        // SIMD/relaxed-SIMD prefix.
      0x8DU, 0x02U, // OpCode F32x4__relaxed_min.
      0x0BU         // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  Instructions = {F32x4RelaxedMax, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected[6] = 0x8EU; // OpCode F32x4__relaxed_max.
  EXPECT_EQ(Output, Expected);

  Instructions = {F64x2RelaxedMin, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected[6] = 0x8FU; // OpCode F64x2__relaxed_min.
  EXPECT_EQ(Output, Expected);

  Instructions = {F64x2RelaxedMax, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected[6] = 0x90U; // OpCode F64x2__relaxed_max.
  EXPECT_EQ(Output, Expected);

  Instructions = {F64x2RelaxedMax, End};
  Output = {};
}

TEST(SerializeInstructionTest, SerializeQ15MulRInstruction) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  std::vector<WasmEdge::AST::Instruction> Instructions;

  // 23. Test rounding Q-format multiplication instruction.
  //
  //   1.  Serialize I16x8__relaxed_q15mulr_s instruction.

  WasmEdge::AST::Instruction I16x8RelaxedQ15MulRS(
      WasmEdge::OpCode::I16x8__relaxed_q15mulr_s);
  WasmEdge::AST::Instruction End(WasmEdge::OpCode::End);

  Instructions = {I16x8RelaxedQ15MulRS, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU,        // Code section
      0x07U,        // Content size = 7
      0x01U,        // Vector length = 1
      0x05U,        // Code segment size = 5
      0x00U,        // Local vec(0)
      0xFDU,        // SIMD/relaxed-SIMD prefix.
      0x91U, 0x02U, // OpCode I16x8__relaxed_q15mulr_s.
      0x0BU         // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  Instructions = {I16x8RelaxedQ15MulRS, End};
  Output = {};
}

TEST(SerializeInstructionTest, SerializeDotProductInstruction) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  std::vector<WasmEdge::AST::Instruction> Instructions;

  // 24. Test dot product instruction.
  //
  //   1.  Serialize I16x8__relaxed_dot_i8x16_i7x16_s instruction.
  //   2.  Serialize I32x4__relaxed_dot_i8x16_i7x16_add_s instruction.

  WasmEdge::AST::Instruction I16x8RelaxedDotI8x16i7x16S(
      WasmEdge::OpCode::I16x8__relaxed_dot_i8x16_i7x16_s);
  WasmEdge::AST::Instruction I16x8RelaxedDotI8x16i7x16AddS(
      WasmEdge::OpCode::I32x4__relaxed_dot_i8x16_i7x16_add_s);
  WasmEdge::AST::Instruction End(WasmEdge::OpCode::End);

  Instructions = {I16x8RelaxedDotI8x16i7x16S, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU,        // Code section
      0x07U,        // Content size = 7
      0x01U,        // Vector length = 1
      0x05U,        // Code segment size = 5
      0x00U,        // Local vec(0)
      0xFDU,        // SIMD/relaxed-SIMD prefix.
      0x92U, 0x02U, // OpCode I16x8__relaxed_dot_i8x16_i7x16_s.
      0x0BU         // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  Instructions = {I16x8RelaxedDotI8x16i7x16AddS, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected[6] = 0x93U; // OpCode I32x4__relaxed_dot_i8x16_i7x16_add_s.
  EXPECT_EQ(Output, Expected);

  Instructions = {I16x8RelaxedDotI8x16i7x16AddS, End};
  Output = {};
}

TEST(SerializeInstructionTest, SerializeAtomicInstruction) {

  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  std::vector<WasmEdge::AST::Instruction> Instructions;

  // 25. Test atomic instructions.
  //
  //   1.  Serialize memory_atomic_notify instruction with memarg.
  //   2.  Serialize memory_atomic_wait32 instruction with memarg.
  //   3.  Serialize i32_atomic_load instruction with memarg.
  //   4.  Serialize i32_atomic_store instruction with memarg.
  //   5.  Serialize i32_atomic_rmw_add instruction with memarg.

  WasmEdge::AST::Instruction MemoryAtomicNotify(
      WasmEdge::OpCode::Memory__atomic__notify);
  WasmEdge::AST::Instruction MemoryAtomicWait32(
      WasmEdge::OpCode::Memory__atomic__wait32);
  WasmEdge::AST::Instruction I32AtomicLoad(WasmEdge::OpCode::I32__atomic__load);
  WasmEdge::AST::Instruction I32AtomicStore(
      WasmEdge::OpCode::I32__atomic__store);
  WasmEdge::AST::Instruction I32AtomicRmwAdd(
      WasmEdge::OpCode::I32__atomic__rmw__add);
  WasmEdge::AST::Instruction End(WasmEdge::OpCode::End);

  MemoryAtomicNotify.getMemoryAlign() = 0x02U;
  MemoryAtomicNotify.getMemoryOffset() = 0x08U;
  Instructions = {MemoryAtomicNotify, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU,        // Code section
      0x08U,        // Content size = 8
      0x01U,        // Vector length = 1
      0x06U,        // Code segment size = 6
      0x00U,        // Local vec(0)
      0xFEU, 0x00U, // OpCode Memory__atomic__notify.
      0x02U,        // Align.
      0x08U,        // Offset.
      0x0BU         // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  MemoryAtomicWait32.getMemoryAlign() = 0x02U;
  MemoryAtomicWait32.getMemoryOffset() = 0x08U;
  Instructions = {MemoryAtomicWait32, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected[6] = 0x01U; // OpCode Memory__atomic__wait32.
  EXPECT_EQ(Output, Expected);

  I32AtomicLoad.getMemoryAlign() = 0x02U;
  I32AtomicLoad.getMemoryOffset() = 0x08U;
  Instructions = {I32AtomicLoad, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected[6] = 0x10U; // OpCode I32__atomic__load.
  EXPECT_EQ(Output, Expected);

  I32AtomicStore.getMemoryAlign() = 0x02U;
  I32AtomicStore.getMemoryOffset() = 0x08U;
  Instructions = {I32AtomicStore, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected[6] = 0x17U; // OpCode I32__atomic__store.
  EXPECT_EQ(Output, Expected);

  I32AtomicRmwAdd.getMemoryAlign() = 0x02U;
  I32AtomicRmwAdd.getMemoryOffset() = 0x08U;
  Instructions = {I32AtomicRmwAdd, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected[6] = 0x1EU; // OpCode I32__atomic__rmw__add.
  EXPECT_EQ(Output, Expected);

  // The remaining opcodes of this group, with their immediates zeroed.
  // The cases above cover the non-zero values.

  Instructions = {
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__atomic__load16_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__atomic__load8_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__atomic__rmw16__add_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__atomic__rmw16__and_u),
      WasmEdge::AST::Instruction(
          WasmEdge::OpCode::I32__atomic__rmw16__cmpxchg_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__atomic__rmw16__or_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__atomic__rmw16__sub_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__atomic__rmw16__xchg_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__atomic__rmw16__xor_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__atomic__rmw8__add_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__atomic__rmw8__and_u),
      WasmEdge::AST::Instruction(
          WasmEdge::OpCode::I32__atomic__rmw8__cmpxchg_u),
      End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU, 0x34U, 0x01U, 0x32U, 0x00U, // Framing.
      0xFEU, 0x13U, 0x00U, 0x00U,        // I32__atomic__load16_u.
      0xFEU, 0x12U, 0x00U, 0x00U,        // I32__atomic__load8_u.
      0xFEU, 0x21U, 0x00U, 0x00U,        // I32__atomic__rmw16__add_u.
      0xFEU, 0x2FU, 0x00U, 0x00U,        // I32__atomic__rmw16__and_u.
      0xFEU, 0x4BU, 0x00U, 0x00U,        // I32__atomic__rmw16__cmpxchg_u.
      0xFEU, 0x36U, 0x00U, 0x00U,        // I32__atomic__rmw16__or_u.
      0xFEU, 0x28U, 0x00U, 0x00U,        // I32__atomic__rmw16__sub_u.
      0xFEU, 0x44U, 0x00U, 0x00U,        // I32__atomic__rmw16__xchg_u.
      0xFEU, 0x3DU, 0x00U, 0x00U,        // I32__atomic__rmw16__xor_u.
      0xFEU, 0x20U, 0x00U, 0x00U,        // I32__atomic__rmw8__add_u.
      0xFEU, 0x2EU, 0x00U, 0x00U,        // I32__atomic__rmw8__and_u.
      0xFEU, 0x4AU, 0x00U, 0x00U,        // I32__atomic__rmw8__cmpxchg_u.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  Instructions = {
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__atomic__rmw8__or_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__atomic__rmw8__sub_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__atomic__rmw8__xchg_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__atomic__rmw8__xor_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__atomic__rmw__and),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__atomic__rmw__cmpxchg),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__atomic__rmw__or),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__atomic__rmw__sub),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__atomic__rmw__xchg),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__atomic__rmw__xor),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__atomic__store16),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__atomic__store8),
      End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU, 0x34U, 0x01U, 0x32U, 0x00U, // Framing.
      0xFEU, 0x35U, 0x00U, 0x00U,        // I32__atomic__rmw8__or_u.
      0xFEU, 0x27U, 0x00U, 0x00U,        // I32__atomic__rmw8__sub_u.
      0xFEU, 0x43U, 0x00U, 0x00U,        // I32__atomic__rmw8__xchg_u.
      0xFEU, 0x3CU, 0x00U, 0x00U,        // I32__atomic__rmw8__xor_u.
      0xFEU, 0x2CU, 0x00U, 0x00U,        // I32__atomic__rmw__and.
      0xFEU, 0x48U, 0x00U, 0x00U,        // I32__atomic__rmw__cmpxchg.
      0xFEU, 0x33U, 0x00U, 0x00U,        // I32__atomic__rmw__or.
      0xFEU, 0x25U, 0x00U, 0x00U,        // I32__atomic__rmw__sub.
      0xFEU, 0x41U, 0x00U, 0x00U,        // I32__atomic__rmw__xchg.
      0xFEU, 0x3AU, 0x00U, 0x00U,        // I32__atomic__rmw__xor.
      0xFEU, 0x1AU, 0x00U, 0x00U,        // I32__atomic__store16.
      0xFEU, 0x19U, 0x00U, 0x00U,        // I32__atomic__store8.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  Instructions = {
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__atomic__load16_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__atomic__load32_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__atomic__load8_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__atomic__rmw16__add_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__atomic__rmw16__and_u),
      WasmEdge::AST::Instruction(
          WasmEdge::OpCode::I64__atomic__rmw16__cmpxchg_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__atomic__rmw16__or_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__atomic__rmw16__sub_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__atomic__rmw16__xchg_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__atomic__rmw16__xor_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__atomic__rmw32__add_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__atomic__rmw32__and_u),
      End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU, 0x34U, 0x01U, 0x32U, 0x00U, // Framing.
      0xFEU, 0x15U, 0x00U, 0x00U,        // I64__atomic__load16_u.
      0xFEU, 0x16U, 0x00U, 0x00U,        // I64__atomic__load32_u.
      0xFEU, 0x14U, 0x00U, 0x00U,        // I64__atomic__load8_u.
      0xFEU, 0x23U, 0x00U, 0x00U,        // I64__atomic__rmw16__add_u.
      0xFEU, 0x31U, 0x00U, 0x00U,        // I64__atomic__rmw16__and_u.
      0xFEU, 0x4DU, 0x00U, 0x00U,        // I64__atomic__rmw16__cmpxchg_u.
      0xFEU, 0x38U, 0x00U, 0x00U,        // I64__atomic__rmw16__or_u.
      0xFEU, 0x2AU, 0x00U, 0x00U,        // I64__atomic__rmw16__sub_u.
      0xFEU, 0x46U, 0x00U, 0x00U,        // I64__atomic__rmw16__xchg_u.
      0xFEU, 0x3FU, 0x00U, 0x00U,        // I64__atomic__rmw16__xor_u.
      0xFEU, 0x24U, 0x00U, 0x00U,        // I64__atomic__rmw32__add_u.
      0xFEU, 0x32U, 0x00U, 0x00U,        // I64__atomic__rmw32__and_u.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  Instructions = {
      WasmEdge::AST::Instruction(
          WasmEdge::OpCode::I64__atomic__rmw32__cmpxchg_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__atomic__rmw32__or_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__atomic__rmw32__sub_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__atomic__rmw32__xchg_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__atomic__rmw32__xor_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__atomic__rmw8__add_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__atomic__rmw8__and_u),
      WasmEdge::AST::Instruction(
          WasmEdge::OpCode::I64__atomic__rmw8__cmpxchg_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__atomic__rmw8__or_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__atomic__rmw8__sub_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__atomic__rmw8__xchg_u),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__atomic__rmw8__xor_u),
      End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU, 0x34U, 0x01U, 0x32U, 0x00U, // Framing.
      0xFEU, 0x4EU, 0x00U, 0x00U,        // I64__atomic__rmw32__cmpxchg_u.
      0xFEU, 0x39U, 0x00U, 0x00U,        // I64__atomic__rmw32__or_u.
      0xFEU, 0x2BU, 0x00U, 0x00U,        // I64__atomic__rmw32__sub_u.
      0xFEU, 0x47U, 0x00U, 0x00U,        // I64__atomic__rmw32__xchg_u.
      0xFEU, 0x40U, 0x00U, 0x00U,        // I64__atomic__rmw32__xor_u.
      0xFEU, 0x22U, 0x00U, 0x00U,        // I64__atomic__rmw8__add_u.
      0xFEU, 0x30U, 0x00U, 0x00U,        // I64__atomic__rmw8__and_u.
      0xFEU, 0x4CU, 0x00U, 0x00U,        // I64__atomic__rmw8__cmpxchg_u.
      0xFEU, 0x37U, 0x00U, 0x00U,        // I64__atomic__rmw8__or_u.
      0xFEU, 0x29U, 0x00U, 0x00U,        // I64__atomic__rmw8__sub_u.
      0xFEU, 0x45U, 0x00U, 0x00U,        // I64__atomic__rmw8__xchg_u.
      0xFEU, 0x3EU, 0x00U, 0x00U,        // I64__atomic__rmw8__xor_u.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  Instructions = {
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__atomic__rmw__add),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__atomic__rmw__and),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__atomic__rmw__cmpxchg),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__atomic__rmw__or),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__atomic__rmw__sub),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__atomic__rmw__xchg),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__atomic__rmw__xor),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__atomic__store),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__atomic__store16),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__atomic__store32),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I64__atomic__store8),
      End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU, 0x30U, 0x01U, 0x2EU, 0x00U, // Framing.
      0xFEU, 0x1FU, 0x00U, 0x00U,        // I64__atomic__rmw__add.
      0xFEU, 0x2DU, 0x00U, 0x00U,        // I64__atomic__rmw__and.
      0xFEU, 0x49U, 0x00U, 0x00U,        // I64__atomic__rmw__cmpxchg.
      0xFEU, 0x34U, 0x00U, 0x00U,        // I64__atomic__rmw__or.
      0xFEU, 0x26U, 0x00U, 0x00U,        // I64__atomic__rmw__sub.
      0xFEU, 0x42U, 0x00U, 0x00U,        // I64__atomic__rmw__xchg.
      0xFEU, 0x3BU, 0x00U, 0x00U,        // I64__atomic__rmw__xor.
      0xFEU, 0x18U, 0x00U, 0x00U,        // I64__atomic__store.
      0xFEU, 0x1CU, 0x00U, 0x00U,        // I64__atomic__store16.
      0xFEU, 0x1DU, 0x00U, 0x00U,        // I64__atomic__store32.
      0xFEU, 0x1BU, 0x00U, 0x00U,        // I64__atomic__store8.
      0x0BU                              // Expression End.
  };
  EXPECT_EQ(Output, Expected);
}

TEST(SerializeInstructionTest, SerializeAtomicFenceAndWait64Instruction) {

  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  std::vector<WasmEdge::AST::Instruction> Instructions;

  // 26. Test atomic.fence and memory.atomic.wait64 serialization.
  //
  //   1.  Serialize atomic.fence (a reserved byte immediate, not a memarg).
  //   2.  Serialize memory.atomic.wait64 with memarg.
  //   3.  Serialize i64.atomic.load with memarg.

  WasmEdge::AST::Instruction End(WasmEdge::OpCode::End);

  WasmEdge::AST::Instruction AtomicFence(WasmEdge::OpCode::Atomic__fence);
  AtomicFence.getTargetIndex() = 0x00U;
  Instructions = {AtomicFence, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU,        // Code section
      0x07U,        // Content size = 7
      0x01U,        // Vector length = 1
      0x05U,        // Code segment size = 5
      0x00U,        // Local vec(0)
      0xFEU, 0x03U, // OpCode Atomic__fence.
      0x00U,        // Reserved zero byte.
      0x0BU         // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  WasmEdge::AST::Instruction MemoryAtomicWait64(
      WasmEdge::OpCode::Memory__atomic__wait64);
  MemoryAtomicWait64.getMemoryAlign() = 0x03U;
  MemoryAtomicWait64.getMemoryOffset() = 0x10U;
  Instructions = {MemoryAtomicWait64, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU,        // Code section
      0x08U,        // Content size = 8
      0x01U,        // Vector length = 1
      0x06U,        // Code segment size = 6
      0x00U,        // Local vec(0)
      0xFEU, 0x02U, // OpCode Memory__atomic__wait64.
      0x03U,        // Align.
      0x10U,        // Offset.
      0x0BU         // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  WasmEdge::AST::Instruction I64AtomicLoad(WasmEdge::OpCode::I64__atomic__load);
  I64AtomicLoad.getMemoryAlign() = 0x03U;
  I64AtomicLoad.getMemoryOffset() = 0x08U;
  Instructions = {I64AtomicLoad, End};
  Output = {};
  Ser.serializeSection(createCodeSec(Instructions), Output);
  Expected = {
      0x0AU,        // Code section
      0x08U,        // Content size = 8
      0x01U,        // Vector length = 1
      0x06U,        // Code segment size = 6
      0x00U,        // Local vec(0)
      0xFEU, 0x11U, // OpCode I64__atomic__load.
      0x03U,        // Align.
      0x08U,        // Offset.
      0x0BU         // Expression End.
  };
  EXPECT_EQ(Output, Expected);
}

} // namespace
