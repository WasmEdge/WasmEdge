// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "loader/serialize.h"

#include <cstdint>
#include <gtest/gtest.h>
#include <memory>
#include <vector>

namespace {

WasmEdge::Loader::Serializer Ser;

WasmEdge::AST::CodeSection createCodeSec(WasmEdge::AST::InstrVec Instrs) {
  WasmEdge::AST::CodeSection CodeSec;
  WasmEdge::AST::CodeSegment CodeSeg;
  WasmEdge::AST::Expression Expr;
  Expr.getInstrs() = std::move(Instrs);
  CodeSeg.getExpr() = Expr;
  CodeSec.getContent().push_back(CodeSeg);
  return CodeSec;
}

TEST(SerializeInstructionTest, SerializeBlockControlInstruction) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  WasmEdge::AST::InstrVec Instrs;
  WasmEdge::BlockType Type;

  // 1. Test block control instructions.
  //
  //   1.  Serialize block with only end operation.
  //   2.  Serialize loop with only end operation.
  //   3.  Serialize block with instructions.
  //   4.  Serialize loop with instructions.

  Type.setEmpty();
  Instrs = {};
  Instrs.appendBlock(WasmEdge::OpCode::Block, Type);
  Instrs.appendEnd();
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.appendBlock(WasmEdge::OpCode::Loop, Type);
  Instrs.appendEnd();
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.appendBlock(WasmEdge::OpCode::Block, Type);
  Instrs.append(WasmEdge::OpCode::I32__eqz);
  Instrs.append(WasmEdge::OpCode::I32__eq);
  Instrs.append(WasmEdge::OpCode::I32__ne);
  Instrs.appendEnd();
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.appendBlock(WasmEdge::OpCode::Loop, Type);
  Instrs.append(WasmEdge::OpCode::I32__eqz);
  Instrs.append(WasmEdge::OpCode::I32__eq);
  Instrs.append(WasmEdge::OpCode::I32__ne);
  Instrs.appendEnd();
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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
  Type.setData(WasmEdge::ValType(WasmEdge::TypeCode::I32));
  Instrs = {};
  Instrs.appendBlock(WasmEdge::OpCode::Block, Type);
  Instrs.appendEnd();
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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
  Type.setData(64U);
  Instrs = {};
  Instrs.appendBlock(WasmEdge::OpCode::Block, Type);
  Instrs.appendEnd();
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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
  WasmEdge::AST::InstrVec Instrs;
  WasmEdge::BlockType Type;

  // 2. Test serialize if-else control instruction.
  //
  //   1.  Serialize if statement with only end operation.
  //   2.  Serialize if and else statements with only end operation.
  //   3.  Serialize if statement with instructions.
  //   4.  Serialize if and else statements with instructions.

  Type.setEmpty();
  Instrs = {};
  Instrs.appendBlock(WasmEdge::OpCode::If, Type);
  Instrs.appendEnd();
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.appendBlock(WasmEdge::OpCode::If, Type);
  Instrs.appendElse();
  Instrs.appendEnd();
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.appendBlock(WasmEdge::OpCode::If, Type);
  Instrs.append(WasmEdge::OpCode::I32__eqz);
  Instrs.append(WasmEdge::OpCode::I32__eq);
  Instrs.append(WasmEdge::OpCode::I32__ne);
  Instrs.appendEnd();
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.appendBlock(WasmEdge::OpCode::If, Type);
  Instrs.append(WasmEdge::OpCode::I32__eqz);
  Instrs.append(WasmEdge::OpCode::I32__eq);
  Instrs.append(WasmEdge::OpCode::I32__ne);
  Instrs.appendElse();
  Instrs.append(WasmEdge::OpCode::I32__eqz);
  Instrs.append(WasmEdge::OpCode::I32__eq);
  Instrs.append(WasmEdge::OpCode::I32__ne);
  Instrs.appendEnd();
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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
  WasmEdge::AST::InstrVec Instrs;

  // 3. Test branch control instructions.
  //
  //   1.  Serialize valid label index.
  //   2.  Serialize Br_on_null instruction.
  //   3.  Serialize Br_on_non_null instruction.
  //   4.  Serialize Br_on_cast instruction.
  //   5.  Serialize Br_on_cast_fail instruction.

  Instrs = {};
  Instrs.appendJump(WasmEdge::OpCode::Br, 0xFFFFFFFFU);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.appendJump(WasmEdge::OpCode::Br_if, 0xFFFFFFFFU);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
  Expected[5] = 0x0DU; // OpCode Br_if.
  EXPECT_EQ(Output, Expected);

  Instrs = {};
  Instrs.appendJump(WasmEdge::OpCode::Br_on_null, 0xFFFFFFFFU);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
  Expected[5] = 0xD5U; // OpCode Br_on_null
  EXPECT_EQ(Output, Expected);

  Instrs = {};
  Instrs.appendJump(WasmEdge::OpCode::Br_on_non_null, 0xFFFFFFFFU);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
  Expected[5] = 0xD6U; // OpCode Br_on_non_null
  EXPECT_EQ(Output, Expected);

  Instrs = {};
  Instrs.appendJump(WasmEdge::OpCode::Br_on_non_null, 0xFFFFFFFFU);
  Instrs.appendEnd();
  Output = {};

  Instrs = {};
  Instrs.appendBrCast(WasmEdge::OpCode::Br_on_cast, 0xFFFFFFFFU,
                      WasmEdge::TypeCode::AnyRef, WasmEdge::TypeCode::EqRef);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.appendBrCast(WasmEdge::OpCode::Br_on_cast_fail, 0xFFFFFFFFU,
                      WasmEdge::TypeCode::AnyRef, WasmEdge::TypeCode::EqRef);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
  Expected[6] = 0x19; // OpCode Br_on_cast_fail.
  EXPECT_EQ(Output, Expected);
  // Only the first type is nullable here, which separates the two flag bits.
  Instrs = {};
  Instrs.appendBrCast(
      WasmEdge::OpCode::Br_on_cast, 0xFFFFFFFFU, WasmEdge::TypeCode::AnyRef,
      WasmEdge::ValType(WasmEdge::TypeCode::Ref, WasmEdge::TypeCode::EqRef, 0));
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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
  WasmEdge::AST::InstrVec Instrs;

  // 4. Test branch table control instruction.
  //
  //   1.  Serialize instruction with empty label vector.
  //   2.  Serialize instruction with label vector.

  Instrs = {};
  Instrs.appendBrTable(std::vector<uint32_t>{0xFFFFFFFFU});
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.appendBrTable(std::vector<uint32_t>{0xFFFFFFF1U, 0xFFFFFFF2U,
                                             0xFFFFFFF3U, 0xFFFFFFFFU});
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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
  WasmEdge::AST::InstrVec Instrs;

  // 5. Test call control instructions.
  //
  //   1.  Serialize call instruction with valid type index.
  //   2.  Serialize call_indirect instruction with valid type and table index.
  //   3.  Serialize Call_ref instruction with valid type index.
  //   4.  Serialize Return_call_ref instruction with valid type and table.

  Instrs = {};
  Instrs.appendIndex(WasmEdge::OpCode::Call, 0xFFFFFFFFU);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.appendIndices(WasmEdge::OpCode::Call_indirect, 0xFFFFFFFFU, 0x05U);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.appendIndex(WasmEdge::OpCode::Call_ref, 0xFFFFFFFFU);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.appendIndex(WasmEdge::OpCode::Return_call_ref, 0xFFFFFFFFU);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
  Expected[5] = 0x15U; // OpCode Return_call_ref.
  EXPECT_EQ(Output, Expected);

  Instrs = {};
  Instrs.appendIndex(WasmEdge::OpCode::Return_call_ref, 0xFFFFFFFFU);
  Instrs.appendEnd();

  // The remaining opcodes of this group, with their immediates zeroed.
  // The cases above cover the non-zero values.

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::Return);
  Instrs.appendIndex(WasmEdge::OpCode::Return_call, 0U);
  Instrs.appendIndices(WasmEdge::OpCode::Return_call_indirect, 0U, 0U);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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
  WasmEdge::AST::InstrVec Instrs;

  // 6. Test exception handling instruction.
  //
  //   1.  Serialize Throw_ref instruction.
  //   2.  Serialize Throw instruction.
  //   3.  Serialize Try_table instruction.

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::Throw_ref);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.appendIndex(WasmEdge::OpCode::Throw, 0xFFFFFFFFU);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  WasmEdge::BlockType Type;
  Type.setEmpty();
  std::vector<WasmEdge::AST::Instruction::CatchDescriptor> Catch(4);
  Catch[0].TagIndex = 0xFF3F1F0FU;
  Catch[0].LabelIndex = 0xFFFFFFFFU;
  Catch[1].IsRef = true;
  Catch[1].TagIndex = 0xFF3F1F0FU;
  Catch[1].LabelIndex = 0xFFFFFFFFU;
  Catch[2].IsAll = true;
  Catch[2].LabelIndex = 0xFFFFFFFFU;
  Catch[3].IsRef = true;
  Catch[3].IsAll = true;
  Catch[3].LabelIndex = 0xFFFFFFFFU;
  Instrs = {};
  Instrs.appendTryTable(Type, Catch);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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
  WasmEdge::AST::InstrVec Instrs;

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

  Instrs = {};
  Instrs.appendValType(WasmEdge::OpCode::Ref__null,
                       WasmEdge::TypeCode::FuncRef);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.appendValType(WasmEdge::OpCode::Ref__null,
                       WasmEdge::TypeCode::ExternRef);
  Instrs.appendEnd();

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::Ref__as_non_null);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::Ref__as_non_null);
  Instrs.appendEnd();
  Output = {};

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::Ref__eq);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
  Expected[5] = 0xD3U; // OpCode Ref__eq.
  EXPECT_EQ(Expected, Output);

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::Ref__i31);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.appendValType(WasmEdge::OpCode::Ref__test,
                       WasmEdge::TypeCode::FuncRef);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.appendValType(WasmEdge::OpCode::Ref__test_null,
                       WasmEdge::TypeCode::FuncRef);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
  Expected[6] = 0x15U; // OpCode Ref__test_null.
  EXPECT_EQ(Output, Expected);

  Instrs = {};
  Instrs.appendValType(WasmEdge::OpCode::Ref__cast,
                       WasmEdge::TypeCode::FuncRef);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
  Expected[6] = 0x16U; // OpCode Ref__cast.
  EXPECT_EQ(Output, Expected);

  Instrs = {};
  Instrs.appendValType(WasmEdge::OpCode::Ref__cast_null,
                       WasmEdge::TypeCode::FuncRef);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
  Expected[6] = 0x17U; // OpCode Ref__cast_null.
  EXPECT_EQ(Output, Expected);

  Output = {};
  Instrs = {};
  Instrs.appendValType(WasmEdge::OpCode::Ref__cast_null,
                       WasmEdge::TypeCode::FuncRef);
  Instrs.appendEnd();

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::Any__convert_extern);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::Extern__convert_any);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
  Expected[6] = 0x1BU; // OpCode Extern__convert_any.
  EXPECT_EQ(Output, Expected);

  Output = {};

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::I31__get_s);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::I31__get_u);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
  Expected[6] = 0x1EU; // OpCode I31__get_u.
  EXPECT_EQ(Output, Expected);

  Output = {};

  Instrs = {};
  Instrs.appendIndex(WasmEdge::OpCode::Struct__new, 0xFFFFFFFFU);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.appendIndex(WasmEdge::OpCode::Struct__new_default, 0xFFFFFFFFU);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
  Expected[6] = 0x01U; // OpCode Struct__new_default.
  EXPECT_EQ(Output, Expected);

  Instrs = {};
  Instrs.appendIndices(WasmEdge::OpCode::Struct__get, 0xFFFFFFFFU, 0x05U);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.appendIndices(WasmEdge::OpCode::Struct__get_s, 0xFFFFFFFFU, 0x05U);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
  Expected[6] = 0x03U; // OpCode Struct__get_s.
  EXPECT_EQ(Output, Expected);

  Instrs = {};
  Instrs.appendIndices(WasmEdge::OpCode::Struct__get_u, 0xFFFFFFFFU, 0x05U);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
  Expected[6] = 0x04U; // OpCode Struct__get_u.
  EXPECT_EQ(Output, Expected);

  Instrs = {};
  Instrs.appendIndices(WasmEdge::OpCode::Struct__set, 0xFFFFFFFFU, 0x05U);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
  Expected[6] = 0x05U; // OpCode Struct__set.
  EXPECT_EQ(Output, Expected);

  Output = {};

  Instrs = {};
  Instrs.appendIndex(WasmEdge::OpCode::Array__new, 0xFFFFFFFFU);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.appendIndex(WasmEdge::OpCode::Array__new_default, 0xFFFFFFFFU);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
  Expected[6] = 0x07U; // OpCode Array__new_default.
  EXPECT_EQ(Output, Expected);

  Instrs = {};
  Instrs.appendIndex(WasmEdge::OpCode::Array__get, 0xFFFFFFFFU);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
  Expected[6] = 0x0BU; // OpCode Array__get.
  EXPECT_EQ(Output, Expected);

  Instrs = {};
  Instrs.appendIndex(WasmEdge::OpCode::Array__get_s, 0xFFFFFFFFU);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
  Expected[6] = 0x0CU; // OpCode Array__get_s.
  EXPECT_EQ(Output, Expected);

  Instrs = {};
  Instrs.appendIndex(WasmEdge::OpCode::Array__get_u, 0xFFFFFFFFU);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
  Expected[6] = 0x0DU; // OpCode Array__get_u.
  EXPECT_EQ(Output, Expected);

  Instrs = {};
  Instrs.appendIndex(WasmEdge::OpCode::Array__set, 0xFFFFFFFFU);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
  Expected[6] = 0x0EU; // OpCode Array__set.
  EXPECT_EQ(Output, Expected);

  Instrs = {};
  Instrs.appendIndex(WasmEdge::OpCode::Array__fill, 0xFFFFFFFFU);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
  Expected[6] = 0x10U; // OpCode Array__fill.
  EXPECT_EQ(Output, Expected);

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::Array__len);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.appendIndices(WasmEdge::OpCode::Array__new_fixed, 0xFFFFFFFFU, 0x05U);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.appendIndices(WasmEdge::OpCode::Array__new_data, 0xFFFFFFFFU, 0x05U);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
  Expected[6] = 0x09U; // OpCode Array__new_data.
  EXPECT_EQ(Output, Expected);

  Instrs = {};
  Instrs.appendIndices(WasmEdge::OpCode::Array__new_elem, 0xFFFFFFFFU, 0x05U);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
  Expected[6] = 0x0AU; // OpCode Array__new_elem.
  EXPECT_EQ(Output, Expected);

  Instrs = {};
  Instrs.appendIndices(WasmEdge::OpCode::Array__copy, 0xFFFFFFFFU, 0x05U);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
  Expected[6] = 0x11U; // OpCode Array__copy.
  EXPECT_EQ(Output, Expected);

  Instrs = {};
  Instrs.appendIndices(WasmEdge::OpCode::Array__init_data, 0xFFFFFFFFU, 0x05U);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
  Expected[6] = 0x12U; // OpCode Array__init_data.
  EXPECT_EQ(Output, Expected);

  Instrs = {};
  Instrs.appendIndices(WasmEdge::OpCode::Array__init_elem, 0xFFFFFFFFU, 0x05U);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
  Expected[6] = 0x13U; // OpCode Array__init_elem.
  EXPECT_EQ(Output, Expected);
  // A concrete heap type is an s33 type index, so 64 needs a continuation
  // byte.
  Instrs = {};
  Instrs.appendValType(WasmEdge::OpCode::Ref__null,
                       WasmEdge::ValType(WasmEdge::TypeCode::RefNull,
                                         WasmEdge::TypeCode::TypeIndex, 64));
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.appendIndex(WasmEdge::OpCode::Ref__func, 0U);
  Instrs.append(WasmEdge::OpCode::Ref__is_null);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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
  WasmEdge::AST::InstrVec Instrs;

  // 8. Test parametric instructions.
  //
  //   1.  Serialize valid select_t instruction with value type list.

  Instrs = {};
  Instrs.appendSelectT(std::vector<WasmEdge::ValType>{WasmEdge::TypeCode::I32,
                                                      WasmEdge::TypeCode::I64});
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.appendSelectT(std::vector<WasmEdge::ValType>{
      WasmEdge::TypeCode::V128, WasmEdge::TypeCode::V128});
  Instrs.appendEnd();

  // The remaining opcodes of this group, with their immediates zeroed.
  // The cases above cover the non-zero values.

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::Drop);
  Instrs.append(WasmEdge::OpCode::Select);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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
  WasmEdge::AST::InstrVec Instrs;

  // 9. Test variable instructions.
  //
  //   1.  Serialize valid local or global index.

  Instrs = {};
  Instrs.appendIndices(WasmEdge::OpCode::Local__get, 0xFFFFFFFFU, 0U);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.appendIndex(WasmEdge::OpCode::Global__get, 0U);
  Instrs.appendIndex(WasmEdge::OpCode::Global__set, 0U);
  Instrs.appendIndices(WasmEdge::OpCode::Local__set, 0U, 0U);
  Instrs.appendIndices(WasmEdge::OpCode::Local__tee, 0U, 0U);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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
  WasmEdge::AST::InstrVec Instrs;

  // 10. Test table instructions.
  //
  //   1.  Serialize table_get instruction.
  //   2.  Serialize table_init instruction.
  //   3.  Serialize table_copy instruction with distinct source and
  //   destination.

  Instrs = {};
  Instrs.appendIndex(WasmEdge::OpCode::Table__get, 0xFFFFFFFFU);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.appendIndices(WasmEdge::OpCode::Table__init, 0xFFFFFFFFU, 0x05U);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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
  Instrs = {};
  Instrs.appendIndices(WasmEdge::OpCode::Table__copy, 0x01U, 0x02U);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.appendIndex(WasmEdge::OpCode::Elem__drop, 0U);
  Instrs.appendIndex(WasmEdge::OpCode::Table__fill, 0U);
  Instrs.appendIndex(WasmEdge::OpCode::Table__grow, 0U);
  Instrs.appendIndex(WasmEdge::OpCode::Table__set, 0U);
  Instrs.appendIndex(WasmEdge::OpCode::Table__size, 0U);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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
  WasmEdge::AST::InstrVec Instrs;

  // 11. Test memory instructions.
  //
  //   1.  Serialize memory_grow instruction.
  //   2.  Serialize i32_load instruction.
  //   3.  Serialize memory_init with a non-zero data segment index.
  //   4.  Serialize memory_copy with both memory indices zero.

  Instrs = {};
  Instrs.appendIndex(WasmEdge::OpCode::Memory__grow, 0U);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.appendMemory(WasmEdge::OpCode::I32__load, 0U, 0xFFFFFFFFU,
                      0xFFFFFFFEU);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.appendMemory(WasmEdge::OpCode::I32__load, 0U, 0xFFFFFFFFU,
                      0xFFFFFFFEU);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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
  Instrs = {};
  Instrs.appendIndices(WasmEdge::OpCode::Memory__init, 0x00U, 0x05U);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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
  Instrs = {};
  Instrs.appendIndices(WasmEdge::OpCode::Memory__copy, 0x00U, 0x00U);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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
  Instrs = {};
  Instrs.appendMemory(WasmEdge::OpCode::I32__load, 0x00U, 0x02U,
                      0x1FF3F1F0FULL);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.appendIndex(WasmEdge::OpCode::Data__drop, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::F32__load, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::F32__store, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::F64__load, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::F64__store, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I32__load16_s, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I32__load16_u, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I32__load8_s, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I32__load8_u, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I32__store16, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I32__store8, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I64__load, 0U, 0U, 0U);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.appendMemory(WasmEdge::OpCode::I64__load16_s, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I64__load16_u, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I64__load32_s, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I64__load32_u, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I64__load8_s, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I64__load8_u, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I64__store, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I64__store16, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I64__store32, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I64__store8, 0U, 0U, 0U);
  Instrs.appendIndex(WasmEdge::OpCode::Memory__fill, 0U);
  Instrs.appendIndex(WasmEdge::OpCode::Memory__size, 0U);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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
  WasmEdge::AST::InstrVec Instrs;

  // 12. Test multi-memory memarg encoding.
  //
  //   1.  Serialize i32_load with a non-zero memory index.
  //   2.  Serialize i32_store with a non-zero memory index.
  //   3.  Serialize i32_load with memory index 0 (single-memory encoding).
  //   4.  Serialize i32_load with a non-zero memory index again.

  Instrs = {};
  Instrs.appendMemory(WasmEdge::OpCode::I32__load, 0x03U, 0x02U, 0x10U);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.appendMemory(WasmEdge::OpCode::I32__store, 0x03U, 0x02U, 0x10U);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.appendMemory(WasmEdge::OpCode::I32__load, 0x00U, 0x02U, 0x10U);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.appendMemory(WasmEdge::OpCode::I32__load, 0x03U, 0x02U, 0x10U);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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
  WasmEdge::AST::InstrVec Instrs;

  // 13. Test const numeric instructions.
  //
  //   1.  Serialize I32 const numeric instruction.
  //   2.  Serialize I64 const numeric instruction.
  //   3.  Serialize F32 const numeric instruction.
  //   4.  Serialize F64 const numeric instruction.

  Instrs = {};
  Instrs.appendNum(WasmEdge::OpCode::I32__const, -123456);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.appendNum(WasmEdge::OpCode::I64__const,
                   static_cast<uint64_t>(-112233445566L));
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.appendNum(WasmEdge::OpCode::F32__const,
                   static_cast<float>(-0x1.921fb4p+1));
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.appendNum(WasmEdge::OpCode::F64__const, -3.1415926535897932);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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
  WasmEdge::AST::InstrVec Instrs;

  // Test numeric instructions.

  // Every opcode of this group, with its immediates zeroed.

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::F32__abs);
  Instrs.append(WasmEdge::OpCode::F32__add);
  Instrs.append(WasmEdge::OpCode::F32__ceil);
  Instrs.append(WasmEdge::OpCode::F32__convert_i32_s);
  Instrs.append(WasmEdge::OpCode::F32__convert_i32_u);
  Instrs.append(WasmEdge::OpCode::F32__convert_i64_s);
  Instrs.append(WasmEdge::OpCode::F32__convert_i64_u);
  Instrs.append(WasmEdge::OpCode::F32__copysign);
  Instrs.append(WasmEdge::OpCode::F32__demote_f64);
  Instrs.append(WasmEdge::OpCode::F32__div);
  Instrs.append(WasmEdge::OpCode::F32__eq);
  Instrs.append(WasmEdge::OpCode::F32__floor);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::F32__ge);
  Instrs.append(WasmEdge::OpCode::F32__gt);
  Instrs.append(WasmEdge::OpCode::F32__le);
  Instrs.append(WasmEdge::OpCode::F32__lt);
  Instrs.append(WasmEdge::OpCode::F32__max);
  Instrs.append(WasmEdge::OpCode::F32__min);
  Instrs.append(WasmEdge::OpCode::F32__mul);
  Instrs.append(WasmEdge::OpCode::F32__ne);
  Instrs.append(WasmEdge::OpCode::F32__nearest);
  Instrs.append(WasmEdge::OpCode::F32__neg);
  Instrs.append(WasmEdge::OpCode::F32__reinterpret_i32);
  Instrs.append(WasmEdge::OpCode::F32__sqrt);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::F32__sub);
  Instrs.append(WasmEdge::OpCode::F32__trunc);
  Instrs.append(WasmEdge::OpCode::F64__abs);
  Instrs.append(WasmEdge::OpCode::F64__add);
  Instrs.append(WasmEdge::OpCode::F64__ceil);
  Instrs.append(WasmEdge::OpCode::F64__convert_i32_s);
  Instrs.append(WasmEdge::OpCode::F64__convert_i32_u);
  Instrs.append(WasmEdge::OpCode::F64__convert_i64_s);
  Instrs.append(WasmEdge::OpCode::F64__convert_i64_u);
  Instrs.append(WasmEdge::OpCode::F64__copysign);
  Instrs.append(WasmEdge::OpCode::F64__div);
  Instrs.append(WasmEdge::OpCode::F64__eq);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::F64__floor);
  Instrs.append(WasmEdge::OpCode::F64__ge);
  Instrs.append(WasmEdge::OpCode::F64__gt);
  Instrs.append(WasmEdge::OpCode::F64__le);
  Instrs.append(WasmEdge::OpCode::F64__lt);
  Instrs.append(WasmEdge::OpCode::F64__max);
  Instrs.append(WasmEdge::OpCode::F64__min);
  Instrs.append(WasmEdge::OpCode::F64__mul);
  Instrs.append(WasmEdge::OpCode::F64__ne);
  Instrs.append(WasmEdge::OpCode::F64__nearest);
  Instrs.append(WasmEdge::OpCode::F64__neg);
  Instrs.append(WasmEdge::OpCode::F64__promote_f32);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::F64__reinterpret_i64);
  Instrs.append(WasmEdge::OpCode::F64__sqrt);
  Instrs.append(WasmEdge::OpCode::F64__sub);
  Instrs.append(WasmEdge::OpCode::F64__trunc);
  Instrs.append(WasmEdge::OpCode::I32__add);
  Instrs.append(WasmEdge::OpCode::I32__and);
  Instrs.append(WasmEdge::OpCode::I32__clz);
  Instrs.append(WasmEdge::OpCode::I32__ctz);
  Instrs.append(WasmEdge::OpCode::I32__div_s);
  Instrs.append(WasmEdge::OpCode::I32__div_u);
  Instrs.append(WasmEdge::OpCode::I32__extend16_s);
  Instrs.append(WasmEdge::OpCode::I32__extend8_s);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::I32__ge_s);
  Instrs.append(WasmEdge::OpCode::I32__ge_u);
  Instrs.append(WasmEdge::OpCode::I32__gt_s);
  Instrs.append(WasmEdge::OpCode::I32__gt_u);
  Instrs.append(WasmEdge::OpCode::I32__le_s);
  Instrs.append(WasmEdge::OpCode::I32__le_u);
  Instrs.append(WasmEdge::OpCode::I32__lt_s);
  Instrs.append(WasmEdge::OpCode::I32__lt_u);
  Instrs.append(WasmEdge::OpCode::I32__mul);
  Instrs.append(WasmEdge::OpCode::I32__or);
  Instrs.append(WasmEdge::OpCode::I32__popcnt);
  Instrs.append(WasmEdge::OpCode::I32__reinterpret_f32);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::I32__rem_s);
  Instrs.append(WasmEdge::OpCode::I32__rem_u);
  Instrs.append(WasmEdge::OpCode::I32__rotl);
  Instrs.append(WasmEdge::OpCode::I32__rotr);
  Instrs.append(WasmEdge::OpCode::I32__shl);
  Instrs.append(WasmEdge::OpCode::I32__shr_s);
  Instrs.append(WasmEdge::OpCode::I32__shr_u);
  Instrs.append(WasmEdge::OpCode::I32__sub);
  Instrs.append(WasmEdge::OpCode::I32__trunc_f32_s);
  Instrs.append(WasmEdge::OpCode::I32__trunc_f32_u);
  Instrs.append(WasmEdge::OpCode::I32__trunc_f64_s);
  Instrs.append(WasmEdge::OpCode::I32__trunc_f64_u);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::I32__wrap_i64);
  Instrs.append(WasmEdge::OpCode::I32__xor);
  Instrs.append(WasmEdge::OpCode::I64__add);
  Instrs.append(WasmEdge::OpCode::I64__and);
  Instrs.append(WasmEdge::OpCode::I64__clz);
  Instrs.append(WasmEdge::OpCode::I64__ctz);
  Instrs.append(WasmEdge::OpCode::I64__div_s);
  Instrs.append(WasmEdge::OpCode::I64__div_u);
  Instrs.append(WasmEdge::OpCode::I64__eq);
  Instrs.append(WasmEdge::OpCode::I64__eqz);
  Instrs.append(WasmEdge::OpCode::I64__extend16_s);
  Instrs.append(WasmEdge::OpCode::I64__extend32_s);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::I64__extend8_s);
  Instrs.append(WasmEdge::OpCode::I64__extend_i32_s);
  Instrs.append(WasmEdge::OpCode::I64__extend_i32_u);
  Instrs.append(WasmEdge::OpCode::I64__ge_s);
  Instrs.append(WasmEdge::OpCode::I64__ge_u);
  Instrs.append(WasmEdge::OpCode::I64__gt_s);
  Instrs.append(WasmEdge::OpCode::I64__gt_u);
  Instrs.append(WasmEdge::OpCode::I64__le_s);
  Instrs.append(WasmEdge::OpCode::I64__le_u);
  Instrs.append(WasmEdge::OpCode::I64__lt_s);
  Instrs.append(WasmEdge::OpCode::I64__lt_u);
  Instrs.append(WasmEdge::OpCode::I64__mul);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::I64__ne);
  Instrs.append(WasmEdge::OpCode::I64__or);
  Instrs.append(WasmEdge::OpCode::I64__popcnt);
  Instrs.append(WasmEdge::OpCode::I64__reinterpret_f64);
  Instrs.append(WasmEdge::OpCode::I64__rem_s);
  Instrs.append(WasmEdge::OpCode::I64__rem_u);
  Instrs.append(WasmEdge::OpCode::I64__rotl);
  Instrs.append(WasmEdge::OpCode::I64__rotr);
  Instrs.append(WasmEdge::OpCode::I64__shl);
  Instrs.append(WasmEdge::OpCode::I64__shr_s);
  Instrs.append(WasmEdge::OpCode::I64__shr_u);
  Instrs.append(WasmEdge::OpCode::I64__sub);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::I64__trunc_f32_s);
  Instrs.append(WasmEdge::OpCode::I64__trunc_f32_u);
  Instrs.append(WasmEdge::OpCode::I64__trunc_f64_s);
  Instrs.append(WasmEdge::OpCode::I64__trunc_f64_u);
  Instrs.append(WasmEdge::OpCode::I64__xor);
  Instrs.append(WasmEdge::OpCode::Nop);
  Instrs.append(WasmEdge::OpCode::Unreachable);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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
  WasmEdge::AST::InstrVec Instrs;

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

  // 1. i32.trunc_sat_f32_s
  Instrs = {};
  Instrs.append(WasmEdge::OpCode::I32__trunc_sat_f32_s);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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
  Instrs = {};
  Instrs.append(WasmEdge::OpCode::I32__trunc_sat_f32_u);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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
  Instrs = {};
  Instrs.append(WasmEdge::OpCode::I32__trunc_sat_f64_s);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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
  Instrs = {};
  Instrs.append(WasmEdge::OpCode::I32__trunc_sat_f64_u);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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
  Instrs = {};
  Instrs.append(WasmEdge::OpCode::I64__trunc_sat_f32_s);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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
  Instrs = {};
  Instrs.append(WasmEdge::OpCode::I64__trunc_sat_f32_u);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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
  Instrs = {};
  Instrs.append(WasmEdge::OpCode::I64__trunc_sat_f64_s);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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
  Instrs = {};
  Instrs.append(WasmEdge::OpCode::I64__trunc_sat_f64_u);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

TEST(SerializeInstructionTest, SerializeWideArithmeticInstruction) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  WasmEdge::AST::InstrVec Instrs;

  // 15. Test wide-arithmetic instructions (0xFC prefix, no immediates).
  //
  //   1.  Serialize i64.add128 instruction.
  //   2.  Serialize i64.sub128 instruction.
  //   3.  Serialize i64.mul_wide_s instruction.
  //   4.  Serialize i64.mul_wide_u instruction.

  // 1. i64.add128
  Instrs = {};
  Instrs.append(WasmEdge::OpCode::I64__add128);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
  Expected = {
      0x0AU,        // Code section
      0x06U,        // Content size = 6
      0x01U,        // Vector length = 1
      0x04U,        // Code segment size = 4
      0x00U,        // Local vec(0)
      0xFCU, 0x13U, // OpCode I64__add128.
      0x0BU         // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  // 2. i64.sub128
  Instrs = {};
  Instrs.append(WasmEdge::OpCode::I64__sub128);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
  Expected = {
      0x0AU,        // Code section
      0x06U,        // Content size = 6
      0x01U,        // Vector length = 1
      0x04U,        // Code segment size = 4
      0x00U,        // Local vec(0)
      0xFCU, 0x14U, // OpCode I64__sub128.
      0x0BU         // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  // 3. i64.mul_wide_s
  Instrs = {};
  Instrs.append(WasmEdge::OpCode::I64__mul_wide_s);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
  Expected = {
      0x0AU,        // Code section
      0x06U,        // Content size = 6
      0x01U,        // Vector length = 1
      0x04U,        // Code segment size = 4
      0x00U,        // Local vec(0)
      0xFCU, 0x15U, // OpCode I64__mul_wide_s.
      0x0BU         // Expression End.
  };
  EXPECT_EQ(Output, Expected);

  // 4. i64.mul_wide_u
  Instrs = {};
  Instrs.append(WasmEdge::OpCode::I64__mul_wide_u);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
  Expected = {
      0x0AU,        // Code section
      0x06U,        // Content size = 6
      0x01U,        // Vector length = 1
      0x04U,        // Code segment size = 4
      0x00U,        // Local vec(0)
      0xFCU, 0x16U, // OpCode I64__mul_wide_u.
      0x0BU         // Expression End.
  };
  EXPECT_EQ(Output, Expected);
}

TEST(SerializeInstructionTest, SerializeSIMDConstInstruction) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  WasmEdge::AST::InstrVec Instrs;

  // 16. Test SIMD const and shuffle instructions.
  //
  //   1.  Serialize V128__const instruction.
  //   2.  Serialize I8x16__shuffle instruction.

  // Use an asymmetric 16-byte pattern so byte-order bugs are detectable.
  WasmEdge::uint128_t Value = 0U;
  for (uint32_t I = 0; I < 16; ++I) {
    Value |= static_cast<WasmEdge::uint128_t>(I + 1) << (I * 8);
  }

  Instrs = {};
  Instrs.appendNum(WasmEdge::OpCode::V128__const, Value);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.appendNum(WasmEdge::OpCode::I8x16__shuffle, Value);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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
  WasmEdge::AST::InstrVec Instrs;

  // 17. Test base SIMD memory instructions.
  //
  //   1.  Serialize v128_load instruction with memarg.
  //   2.  Serialize v128_store instruction with memarg.
  //   3.  Serialize v128_load8_lane instruction with memarg and lane.

  Instrs = {};
  Instrs.appendMemory(WasmEdge::OpCode::V128__load, 0U, 0x04U, 0x10U);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.appendMemory(WasmEdge::OpCode::V128__store, 0U, 0x04U, 0x10U);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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
  Instrs = {};
  Instrs.appendMemoryLane(WasmEdge::OpCode::V128__load8_lane, 0U, 0x00U, 0x10U,
                          0x0FU);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.appendMemoryLane(WasmEdge::OpCode::V128__load16_lane, 0U, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::V128__load16_splat, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::V128__load16x4_s, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::V128__load16x4_u, 0U, 0U, 0U);
  Instrs.appendMemoryLane(WasmEdge::OpCode::V128__load32_lane, 0U, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::V128__load32_splat, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::V128__load32_zero, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::V128__load32x2_s, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::V128__load32x2_u, 0U, 0U, 0U);
  Instrs.appendMemoryLane(WasmEdge::OpCode::V128__load64_lane, 0U, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::V128__load64_splat, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::V128__load64_zero, 0U, 0U, 0U);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.appendMemory(WasmEdge::OpCode::V128__load8_splat, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::V128__load8x8_s, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::V128__load8x8_u, 0U, 0U, 0U);
  Instrs.appendMemoryLane(WasmEdge::OpCode::V128__store16_lane, 0U, 0U, 0U, 0U);
  Instrs.appendMemoryLane(WasmEdge::OpCode::V128__store32_lane, 0U, 0U, 0U, 0U);
  Instrs.appendMemoryLane(WasmEdge::OpCode::V128__store64_lane, 0U, 0U, 0U, 0U);
  Instrs.appendMemoryLane(WasmEdge::OpCode::V128__store8_lane, 0U, 0U, 0U, 0U);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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
  WasmEdge::AST::InstrVec Instrs;

  // 18. Test base SIMD lane and numeric instructions.
  //
  //   1.  Serialize i8x16_splat instruction.
  //   2.  Serialize i8x16_add instruction.
  //   3.  Serialize i32x4_mul instruction (two-byte LEB128 opcode suffix).
  //   4.  Serialize f32x4_add instruction (two-byte LEB128 opcode suffix).
  //   5.  Serialize i8x16_extract_lane_s instruction with lane immediate.
  //   6.  Serialize i8x16_replace_lane instruction with lane immediate.

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::I8x16__splat);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::I8x16__add);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::I32x4__mul);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::F32x4__add);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.appendLane(WasmEdge::OpCode::I8x16__extract_lane_s, 0x05U);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.appendLane(WasmEdge::OpCode::I8x16__replace_lane, 0x05U);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.appendLane(WasmEdge::OpCode::F32x4__extract_lane, 0U);
  Instrs.appendLane(WasmEdge::OpCode::F32x4__replace_lane, 0U);
  Instrs.appendLane(WasmEdge::OpCode::F64x2__extract_lane, 0U);
  Instrs.appendLane(WasmEdge::OpCode::F64x2__replace_lane, 0U);
  Instrs.appendLane(WasmEdge::OpCode::I16x8__extract_lane_s, 0U);
  Instrs.appendLane(WasmEdge::OpCode::I16x8__extract_lane_u, 0U);
  Instrs.appendLane(WasmEdge::OpCode::I16x8__replace_lane, 0U);
  Instrs.appendLane(WasmEdge::OpCode::I32x4__extract_lane, 0U);
  Instrs.appendLane(WasmEdge::OpCode::I32x4__replace_lane, 0U);
  Instrs.appendLane(WasmEdge::OpCode::I64x2__extract_lane, 0U);
  Instrs.appendLane(WasmEdge::OpCode::I64x2__replace_lane, 0U);
  Instrs.appendLane(WasmEdge::OpCode::I8x16__extract_lane_u, 0U);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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
  WasmEdge::AST::InstrVec Instrs;

  // Test SIMD numeric instructions.

  // Every opcode of this group, with its immediates zeroed.

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::F32x4__abs);
  Instrs.append(WasmEdge::OpCode::F32x4__ceil);
  Instrs.append(WasmEdge::OpCode::F32x4__convert_i32x4_s);
  Instrs.append(WasmEdge::OpCode::F32x4__convert_i32x4_u);
  Instrs.append(WasmEdge::OpCode::F32x4__demote_f64x2_zero);
  Instrs.append(WasmEdge::OpCode::F32x4__div);
  Instrs.append(WasmEdge::OpCode::F32x4__eq);
  Instrs.append(WasmEdge::OpCode::F32x4__floor);
  Instrs.append(WasmEdge::OpCode::F32x4__ge);
  Instrs.append(WasmEdge::OpCode::F32x4__gt);
  Instrs.append(WasmEdge::OpCode::F32x4__le);
  Instrs.append(WasmEdge::OpCode::F32x4__lt);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::F32x4__max);
  Instrs.append(WasmEdge::OpCode::F32x4__min);
  Instrs.append(WasmEdge::OpCode::F32x4__mul);
  Instrs.append(WasmEdge::OpCode::F32x4__ne);
  Instrs.append(WasmEdge::OpCode::F32x4__nearest);
  Instrs.append(WasmEdge::OpCode::F32x4__neg);
  Instrs.append(WasmEdge::OpCode::F32x4__pmax);
  Instrs.append(WasmEdge::OpCode::F32x4__pmin);
  Instrs.append(WasmEdge::OpCode::F32x4__splat);
  Instrs.append(WasmEdge::OpCode::F32x4__sqrt);
  Instrs.append(WasmEdge::OpCode::F32x4__sub);
  Instrs.append(WasmEdge::OpCode::F32x4__trunc);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::F64x2__abs);
  Instrs.append(WasmEdge::OpCode::F64x2__add);
  Instrs.append(WasmEdge::OpCode::F64x2__ceil);
  Instrs.append(WasmEdge::OpCode::F64x2__convert_low_i32x4_s);
  Instrs.append(WasmEdge::OpCode::F64x2__convert_low_i32x4_u);
  Instrs.append(WasmEdge::OpCode::F64x2__div);
  Instrs.append(WasmEdge::OpCode::F64x2__eq);
  Instrs.append(WasmEdge::OpCode::F64x2__floor);
  Instrs.append(WasmEdge::OpCode::F64x2__ge);
  Instrs.append(WasmEdge::OpCode::F64x2__gt);
  Instrs.append(WasmEdge::OpCode::F64x2__le);
  Instrs.append(WasmEdge::OpCode::F64x2__lt);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::F64x2__max);
  Instrs.append(WasmEdge::OpCode::F64x2__min);
  Instrs.append(WasmEdge::OpCode::F64x2__mul);
  Instrs.append(WasmEdge::OpCode::F64x2__ne);
  Instrs.append(WasmEdge::OpCode::F64x2__nearest);
  Instrs.append(WasmEdge::OpCode::F64x2__neg);
  Instrs.append(WasmEdge::OpCode::F64x2__pmax);
  Instrs.append(WasmEdge::OpCode::F64x2__pmin);
  Instrs.append(WasmEdge::OpCode::F64x2__promote_low_f32x4);
  Instrs.append(WasmEdge::OpCode::F64x2__splat);
  Instrs.append(WasmEdge::OpCode::F64x2__sqrt);
  Instrs.append(WasmEdge::OpCode::F64x2__sub);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::F64x2__trunc);
  Instrs.append(WasmEdge::OpCode::I16x8__abs);
  Instrs.append(WasmEdge::OpCode::I16x8__add);
  Instrs.append(WasmEdge::OpCode::I16x8__add_sat_s);
  Instrs.append(WasmEdge::OpCode::I16x8__add_sat_u);
  Instrs.append(WasmEdge::OpCode::I16x8__all_true);
  Instrs.append(WasmEdge::OpCode::I16x8__avgr_u);
  Instrs.append(WasmEdge::OpCode::I16x8__bitmask);
  Instrs.append(WasmEdge::OpCode::I16x8__eq);
  Instrs.append(WasmEdge::OpCode::I16x8__extadd_pairwise_i8x16_s);
  Instrs.append(WasmEdge::OpCode::I16x8__extadd_pairwise_i8x16_u);
  Instrs.append(WasmEdge::OpCode::I16x8__extend_high_i8x16_s);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::I16x8__extend_high_i8x16_u);
  Instrs.append(WasmEdge::OpCode::I16x8__extend_low_i8x16_s);
  Instrs.append(WasmEdge::OpCode::I16x8__extend_low_i8x16_u);
  Instrs.append(WasmEdge::OpCode::I16x8__extmul_high_i8x16_s);
  Instrs.append(WasmEdge::OpCode::I16x8__extmul_high_i8x16_u);
  Instrs.append(WasmEdge::OpCode::I16x8__extmul_low_i8x16_s);
  Instrs.append(WasmEdge::OpCode::I16x8__extmul_low_i8x16_u);
  Instrs.append(WasmEdge::OpCode::I16x8__ge_s);
  Instrs.append(WasmEdge::OpCode::I16x8__ge_u);
  Instrs.append(WasmEdge::OpCode::I16x8__gt_s);
  Instrs.append(WasmEdge::OpCode::I16x8__gt_u);
  Instrs.append(WasmEdge::OpCode::I16x8__le_s);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::I16x8__le_u);
  Instrs.append(WasmEdge::OpCode::I16x8__lt_s);
  Instrs.append(WasmEdge::OpCode::I16x8__lt_u);
  Instrs.append(WasmEdge::OpCode::I16x8__max_s);
  Instrs.append(WasmEdge::OpCode::I16x8__max_u);
  Instrs.append(WasmEdge::OpCode::I16x8__min_s);
  Instrs.append(WasmEdge::OpCode::I16x8__min_u);
  Instrs.append(WasmEdge::OpCode::I16x8__mul);
  Instrs.append(WasmEdge::OpCode::I16x8__narrow_i32x4_s);
  Instrs.append(WasmEdge::OpCode::I16x8__narrow_i32x4_u);
  Instrs.append(WasmEdge::OpCode::I16x8__ne);
  Instrs.append(WasmEdge::OpCode::I16x8__neg);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::I16x8__q15mulr_sat_s);
  Instrs.append(WasmEdge::OpCode::I16x8__shl);
  Instrs.append(WasmEdge::OpCode::I16x8__shr_s);
  Instrs.append(WasmEdge::OpCode::I16x8__shr_u);
  Instrs.append(WasmEdge::OpCode::I16x8__splat);
  Instrs.append(WasmEdge::OpCode::I16x8__sub);
  Instrs.append(WasmEdge::OpCode::I16x8__sub_sat_s);
  Instrs.append(WasmEdge::OpCode::I16x8__sub_sat_u);
  Instrs.append(WasmEdge::OpCode::I32x4__abs);
  Instrs.append(WasmEdge::OpCode::I32x4__add);
  Instrs.append(WasmEdge::OpCode::I32x4__all_true);
  Instrs.append(WasmEdge::OpCode::I32x4__bitmask);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::I32x4__dot_i16x8_s);
  Instrs.append(WasmEdge::OpCode::I32x4__eq);
  Instrs.append(WasmEdge::OpCode::I32x4__extadd_pairwise_i16x8_s);
  Instrs.append(WasmEdge::OpCode::I32x4__extadd_pairwise_i16x8_u);
  Instrs.append(WasmEdge::OpCode::I32x4__extend_high_i16x8_s);
  Instrs.append(WasmEdge::OpCode::I32x4__extend_high_i16x8_u);
  Instrs.append(WasmEdge::OpCode::I32x4__extend_low_i16x8_s);
  Instrs.append(WasmEdge::OpCode::I32x4__extend_low_i16x8_u);
  Instrs.append(WasmEdge::OpCode::I32x4__extmul_high_i16x8_s);
  Instrs.append(WasmEdge::OpCode::I32x4__extmul_high_i16x8_u);
  Instrs.append(WasmEdge::OpCode::I32x4__extmul_low_i16x8_s);
  Instrs.append(WasmEdge::OpCode::I32x4__extmul_low_i16x8_u);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::I32x4__ge_s);
  Instrs.append(WasmEdge::OpCode::I32x4__ge_u);
  Instrs.append(WasmEdge::OpCode::I32x4__gt_s);
  Instrs.append(WasmEdge::OpCode::I32x4__gt_u);
  Instrs.append(WasmEdge::OpCode::I32x4__le_s);
  Instrs.append(WasmEdge::OpCode::I32x4__le_u);
  Instrs.append(WasmEdge::OpCode::I32x4__lt_s);
  Instrs.append(WasmEdge::OpCode::I32x4__lt_u);
  Instrs.append(WasmEdge::OpCode::I32x4__max_s);
  Instrs.append(WasmEdge::OpCode::I32x4__max_u);
  Instrs.append(WasmEdge::OpCode::I32x4__min_s);
  Instrs.append(WasmEdge::OpCode::I32x4__min_u);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::I32x4__ne);
  Instrs.append(WasmEdge::OpCode::I32x4__neg);
  Instrs.append(WasmEdge::OpCode::I32x4__shl);
  Instrs.append(WasmEdge::OpCode::I32x4__shr_s);
  Instrs.append(WasmEdge::OpCode::I32x4__shr_u);
  Instrs.append(WasmEdge::OpCode::I32x4__splat);
  Instrs.append(WasmEdge::OpCode::I32x4__sub);
  Instrs.append(WasmEdge::OpCode::I32x4__trunc_sat_f32x4_s);
  Instrs.append(WasmEdge::OpCode::I32x4__trunc_sat_f32x4_u);
  Instrs.append(WasmEdge::OpCode::I32x4__trunc_sat_f64x2_s_zero);
  Instrs.append(WasmEdge::OpCode::I32x4__trunc_sat_f64x2_u_zero);
  Instrs.append(WasmEdge::OpCode::I64x2__abs);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::I64x2__add);
  Instrs.append(WasmEdge::OpCode::I64x2__all_true);
  Instrs.append(WasmEdge::OpCode::I64x2__bitmask);
  Instrs.append(WasmEdge::OpCode::I64x2__eq);
  Instrs.append(WasmEdge::OpCode::I64x2__extend_high_i32x4_s);
  Instrs.append(WasmEdge::OpCode::I64x2__extend_high_i32x4_u);
  Instrs.append(WasmEdge::OpCode::I64x2__extend_low_i32x4_s);
  Instrs.append(WasmEdge::OpCode::I64x2__extend_low_i32x4_u);
  Instrs.append(WasmEdge::OpCode::I64x2__extmul_high_i32x4_s);
  Instrs.append(WasmEdge::OpCode::I64x2__extmul_high_i32x4_u);
  Instrs.append(WasmEdge::OpCode::I64x2__extmul_low_i32x4_s);
  Instrs.append(WasmEdge::OpCode::I64x2__extmul_low_i32x4_u);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::I64x2__ge_s);
  Instrs.append(WasmEdge::OpCode::I64x2__gt_s);
  Instrs.append(WasmEdge::OpCode::I64x2__le_s);
  Instrs.append(WasmEdge::OpCode::I64x2__lt_s);
  Instrs.append(WasmEdge::OpCode::I64x2__mul);
  Instrs.append(WasmEdge::OpCode::I64x2__ne);
  Instrs.append(WasmEdge::OpCode::I64x2__neg);
  Instrs.append(WasmEdge::OpCode::I64x2__shl);
  Instrs.append(WasmEdge::OpCode::I64x2__shr_s);
  Instrs.append(WasmEdge::OpCode::I64x2__shr_u);
  Instrs.append(WasmEdge::OpCode::I64x2__splat);
  Instrs.append(WasmEdge::OpCode::I64x2__sub);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::I8x16__abs);
  Instrs.append(WasmEdge::OpCode::I8x16__add_sat_s);
  Instrs.append(WasmEdge::OpCode::I8x16__add_sat_u);
  Instrs.append(WasmEdge::OpCode::I8x16__all_true);
  Instrs.append(WasmEdge::OpCode::I8x16__avgr_u);
  Instrs.append(WasmEdge::OpCode::I8x16__bitmask);
  Instrs.append(WasmEdge::OpCode::I8x16__eq);
  Instrs.append(WasmEdge::OpCode::I8x16__ge_s);
  Instrs.append(WasmEdge::OpCode::I8x16__ge_u);
  Instrs.append(WasmEdge::OpCode::I8x16__gt_s);
  Instrs.append(WasmEdge::OpCode::I8x16__gt_u);
  Instrs.append(WasmEdge::OpCode::I8x16__le_s);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::I8x16__le_u);
  Instrs.append(WasmEdge::OpCode::I8x16__lt_s);
  Instrs.append(WasmEdge::OpCode::I8x16__lt_u);
  Instrs.append(WasmEdge::OpCode::I8x16__max_s);
  Instrs.append(WasmEdge::OpCode::I8x16__max_u);
  Instrs.append(WasmEdge::OpCode::I8x16__min_s);
  Instrs.append(WasmEdge::OpCode::I8x16__min_u);
  Instrs.append(WasmEdge::OpCode::I8x16__narrow_i16x8_s);
  Instrs.append(WasmEdge::OpCode::I8x16__narrow_i16x8_u);
  Instrs.append(WasmEdge::OpCode::I8x16__ne);
  Instrs.append(WasmEdge::OpCode::I8x16__neg);
  Instrs.append(WasmEdge::OpCode::I8x16__popcnt);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::I8x16__shl);
  Instrs.append(WasmEdge::OpCode::I8x16__shr_s);
  Instrs.append(WasmEdge::OpCode::I8x16__shr_u);
  Instrs.append(WasmEdge::OpCode::I8x16__sub);
  Instrs.append(WasmEdge::OpCode::I8x16__sub_sat_s);
  Instrs.append(WasmEdge::OpCode::I8x16__sub_sat_u);
  Instrs.append(WasmEdge::OpCode::I8x16__swizzle);
  Instrs.append(WasmEdge::OpCode::V128__and);
  Instrs.append(WasmEdge::OpCode::V128__andnot);
  Instrs.append(WasmEdge::OpCode::V128__any_true);
  Instrs.append(WasmEdge::OpCode::V128__bitselect);
  Instrs.append(WasmEdge::OpCode::V128__not);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::V128__or);
  Instrs.append(WasmEdge::OpCode::V128__xor);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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
  WasmEdge::AST::InstrVec Instrs;

  // 19. Test swizzle instruction.
  //
  //   1.  Serialize I8x16__relaxed_swizzle instruction.

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::I8x16__relaxed_swizzle);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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
  Instrs = {};
  Instrs.append(WasmEdge::OpCode::I8x16__relaxed_swizzle);
  Instrs.appendEnd();
}

TEST(SerializeInstructionTest, SerializeTruncInstruction) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  WasmEdge::AST::InstrVec Instrs;

  // 20. Test trunc instruction.
  //
  //   1.  Serialize I32x4__relaxed_trunc_f32x4_s instruction.
  //   2.  Serialize I32x4__relaxed_trunc_f32x4_u instruction.
  //   3.  Serialize I32x4__relaxed_trunc_f64x2_s_zero instruction.
  //   4.  Serialize I32x4__relaxed_trunc_f64x2_u_zero instruction.

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::I32x4__relaxed_trunc_f32x4_s);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::I32x4__relaxed_trunc_f32x4_u);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
  Expected[6] = 0x82U; // OpCode I32x4__relaxed_trunc_f32x4_u
  EXPECT_EQ(Output, Expected);

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::I32x4__relaxed_trunc_f64x2_s_zero);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
  Expected[6] = 0x83U; // OpCode I32x4__relaxed_trunc_f64x2_s_zero
  EXPECT_EQ(Output, Expected);

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::I32x4__relaxed_trunc_f64x2_u_zero);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
  Expected[6] = 0x84U; // OpCode I32x4__relaxed_trunc_f64x2_u_zero
  EXPECT_EQ(Output, Expected);

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::I32x4__relaxed_trunc_f64x2_u_zero);
  Instrs.appendEnd();
  Output = {};
}

TEST(SerializeInstructionTest, SerializeMulAddInstruction) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  WasmEdge::AST::InstrVec Instrs;

  // 21. Test multiply-add instruction.
  //
  //   1.  Serialize F32x4__relaxed_madd instruction.
  //   2.  Serialize F32x4__relaxed_nmadd instruction.
  //   3.  Serialize F64x2__relaxed_madd instruction.
  //   4.  Serialize F64x2__relaxed_nmadd instruction.

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::F32x4__relaxed_madd);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::F32x4__relaxed_nmadd);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
  Expected[6] = 0x86U; // OpCode F32x4__relaxed_nmadd.
  EXPECT_EQ(Output, Expected);

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::F64x2__relaxed_madd);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
  Expected[6] = 0x87U; // OpCode F64x2__relaxed_madd.
  EXPECT_EQ(Output, Expected);

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::F64x2__relaxed_nmadd);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
  Expected[6] = 0x88U; // OpCode F64x2__relaxed_nmadd.
  EXPECT_EQ(Output, Expected);

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::F64x2__relaxed_nmadd);
  Instrs.appendEnd();
  Output = {};
}

TEST(SerializeInstructionTest, SerializeLaneSelectInstruction) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  WasmEdge::AST::InstrVec Instrs;

  // 22. Test laneselect instruction.
  //
  //   1.  Serialize I8x16__relaxed_laneselect instruction.
  //   2.  Serialize I16x8__relaxed_laneselect instruction.
  //   3.  Serialize I32x4__relaxed_laneselect instruction.
  //   4.  Serialize I64x2__relaxed_laneselect instruction.

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::I8x16__relaxed_laneselect);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::I16x8__relaxed_laneselect);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
  Expected[6] = 0x8AU; // OpCode I16x8__relaxed_laneselect.
  EXPECT_EQ(Output, Expected);

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::I32x4__relaxed_laneselect);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
  Expected[6] = 0x8BU; // OpCode I32x4__relaxed_laneselect.
  EXPECT_EQ(Output, Expected);

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::I64x2__relaxed_laneselect);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
  Expected[6] = 0x8CU; // OpCode I64x2__relaxed_laneselect.

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::I64x2__relaxed_laneselect);
  Instrs.appendEnd();
  Output = {};
}

TEST(SerializeInstructionTest, SerializeMinMaxInstruction) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  WasmEdge::AST::InstrVec Instrs;

  // 23. Test laneselect instruction.
  //
  //   1.  Serialize F32x4__relaxed_min instruction.
  //   2.  Serialize F32x4__relaxed_max instruction.
  //   3.  Serialize F64x2__relaxed_min instruction.
  //   4.  Serialize F64x2__relaxed_max instruction.

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::F32x4__relaxed_min);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::F32x4__relaxed_max);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
  Expected[6] = 0x8EU; // OpCode F32x4__relaxed_max.
  EXPECT_EQ(Output, Expected);

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::F64x2__relaxed_min);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
  Expected[6] = 0x8FU; // OpCode F64x2__relaxed_min.
  EXPECT_EQ(Output, Expected);

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::F64x2__relaxed_max);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
  Expected[6] = 0x90U; // OpCode F64x2__relaxed_max.
  EXPECT_EQ(Output, Expected);

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::F64x2__relaxed_max);
  Instrs.appendEnd();
  Output = {};
}

TEST(SerializeInstructionTest, SerializeQ15MulRInstruction) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  WasmEdge::AST::InstrVec Instrs;

  // 24. Test rounding Q-format multiplication instruction.
  //
  //   1.  Serialize I16x8__relaxed_q15mulr_s instruction.

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::I16x8__relaxed_q15mulr_s);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::I16x8__relaxed_q15mulr_s);
  Instrs.appendEnd();
  Output = {};
}

TEST(SerializeInstructionTest, SerializeDotProductInstruction) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  WasmEdge::AST::InstrVec Instrs;

  // 25. Test dot product instruction.
  //
  //   1.  Serialize I16x8__relaxed_dot_i8x16_i7x16_s instruction.
  //   2.  Serialize I32x4__relaxed_dot_i8x16_i7x16_add_s instruction.

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::I16x8__relaxed_dot_i8x16_i7x16_s);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::I32x4__relaxed_dot_i8x16_i7x16_add_s);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
  Expected[6] = 0x93U; // OpCode I32x4__relaxed_dot_i8x16_i7x16_add_s.
  EXPECT_EQ(Output, Expected);

  Instrs = {};
  Instrs.append(WasmEdge::OpCode::I32x4__relaxed_dot_i8x16_i7x16_add_s);
  Instrs.appendEnd();
  Output = {};
}

TEST(SerializeInstructionTest, SerializeAtomicInstruction) {

  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  WasmEdge::AST::InstrVec Instrs;

  // 26. Test atomic instructions.
  //
  //   1.  Serialize memory_atomic_notify instruction with memarg.
  //   2.  Serialize memory_atomic_wait32 instruction with memarg.
  //   3.  Serialize i32_atomic_load instruction with memarg.
  //   4.  Serialize i32_atomic_store instruction with memarg.
  //   5.  Serialize i32_atomic_rmw_add instruction with memarg.

  Instrs = {};
  Instrs.appendMemory(WasmEdge::OpCode::Memory__atomic__notify, 0U, 0x02U,
                      0x08U);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.appendMemory(WasmEdge::OpCode::Memory__atomic__wait32, 0U, 0x02U,
                      0x08U);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
  Expected[6] = 0x01U; // OpCode Memory__atomic__wait32.
  EXPECT_EQ(Output, Expected);

  Instrs = {};
  Instrs.appendMemory(WasmEdge::OpCode::I32__atomic__load, 0U, 0x02U, 0x08U);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
  Expected[6] = 0x10U; // OpCode I32__atomic__load.
  EXPECT_EQ(Output, Expected);

  Instrs = {};
  Instrs.appendMemory(WasmEdge::OpCode::I32__atomic__store, 0U, 0x02U, 0x08U);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
  Expected[6] = 0x17U; // OpCode I32__atomic__store.
  EXPECT_EQ(Output, Expected);

  Instrs = {};
  Instrs.appendMemory(WasmEdge::OpCode::I32__atomic__rmw__add, 0U, 0x02U,
                      0x08U);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
  Expected[6] = 0x1EU; // OpCode I32__atomic__rmw__add.
  EXPECT_EQ(Output, Expected);

  // The remaining opcodes of this group, with their immediates zeroed.
  // The cases above cover the non-zero values.

  Instrs = {};
  Instrs.appendMemory(WasmEdge::OpCode::I32__atomic__load16_u, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I32__atomic__load8_u, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I32__atomic__rmw16__add_u, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I32__atomic__rmw16__and_u, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I32__atomic__rmw16__cmpxchg_u, 0U, 0U,
                      0U);
  Instrs.appendMemory(WasmEdge::OpCode::I32__atomic__rmw16__or_u, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I32__atomic__rmw16__sub_u, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I32__atomic__rmw16__xchg_u, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I32__atomic__rmw16__xor_u, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I32__atomic__rmw8__add_u, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I32__atomic__rmw8__and_u, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I32__atomic__rmw8__cmpxchg_u, 0U, 0U,
                      0U);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.appendMemory(WasmEdge::OpCode::I32__atomic__rmw8__or_u, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I32__atomic__rmw8__sub_u, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I32__atomic__rmw8__xchg_u, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I32__atomic__rmw8__xor_u, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I32__atomic__rmw__and, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I32__atomic__rmw__cmpxchg, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I32__atomic__rmw__or, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I32__atomic__rmw__sub, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I32__atomic__rmw__xchg, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I32__atomic__rmw__xor, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I32__atomic__store16, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I32__atomic__store8, 0U, 0U, 0U);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.appendMemory(WasmEdge::OpCode::I64__atomic__load16_u, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I64__atomic__load32_u, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I64__atomic__load8_u, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I64__atomic__rmw16__add_u, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I64__atomic__rmw16__and_u, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I64__atomic__rmw16__cmpxchg_u, 0U, 0U,
                      0U);
  Instrs.appendMemory(WasmEdge::OpCode::I64__atomic__rmw16__or_u, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I64__atomic__rmw16__sub_u, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I64__atomic__rmw16__xchg_u, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I64__atomic__rmw16__xor_u, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I64__atomic__rmw32__add_u, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I64__atomic__rmw32__and_u, 0U, 0U, 0U);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.appendMemory(WasmEdge::OpCode::I64__atomic__rmw32__cmpxchg_u, 0U, 0U,
                      0U);
  Instrs.appendMemory(WasmEdge::OpCode::I64__atomic__rmw32__or_u, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I64__atomic__rmw32__sub_u, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I64__atomic__rmw32__xchg_u, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I64__atomic__rmw32__xor_u, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I64__atomic__rmw8__add_u, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I64__atomic__rmw8__and_u, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I64__atomic__rmw8__cmpxchg_u, 0U, 0U,
                      0U);
  Instrs.appendMemory(WasmEdge::OpCode::I64__atomic__rmw8__or_u, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I64__atomic__rmw8__sub_u, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I64__atomic__rmw8__xchg_u, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I64__atomic__rmw8__xor_u, 0U, 0U, 0U);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.appendMemory(WasmEdge::OpCode::I64__atomic__rmw__add, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I64__atomic__rmw__and, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I64__atomic__rmw__cmpxchg, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I64__atomic__rmw__or, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I64__atomic__rmw__sub, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I64__atomic__rmw__xchg, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I64__atomic__rmw__xor, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I64__atomic__store, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I64__atomic__store16, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I64__atomic__store32, 0U, 0U, 0U);
  Instrs.appendMemory(WasmEdge::OpCode::I64__atomic__store8, 0U, 0U, 0U);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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
  WasmEdge::AST::InstrVec Instrs;

  // 27. Test atomic.fence and memory.atomic.wait64 serialization.
  //
  //   1.  Serialize atomic.fence (a reserved byte immediate, not a memarg).
  //   2.  Serialize memory.atomic.wait64 with memarg.
  //   3.  Serialize i64.atomic.load with memarg.

  Instrs = {};
  Instrs.appendIndex(WasmEdge::OpCode::Atomic__fence, 0x00U);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.appendMemory(WasmEdge::OpCode::Memory__atomic__wait64, 0U, 0x03U,
                      0x10U);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

  Instrs = {};
  Instrs.appendMemory(WasmEdge::OpCode::I64__atomic__load, 0U, 0x03U, 0x08U);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
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

TEST(SerializeInstructionTest, SerializeOversizedListInstruction) {

  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  WasmEdge::AST::InstrVec Instrs;

  // 28. Test list instructions whose records exceed the 16-bit size field.
  //
  //   1.  Serialize Br_table instruction with 4096 labels.
  //   2.  Serialize Select_t instruction with 8191 value types.
  //   3.  Serialize Try_table instruction with 2340 catch clauses.
  //   4.  Reject Br_table instruction over the buffer size limit.

  Instrs = {};
  Instrs.appendBrTable(std::vector<uint32_t>(4096, 0x02U));
  Instrs.append(WasmEdge::OpCode::Unreachable);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
  Expected = {
      0x0AU,        // Code section
      0x89U, 0x20U, // Content size = 4105
      0x01U,        // Vector length = 1
      0x86U, 0x20U, // Code segment size = 4102
      0x00U,        // Local vec(0)
      0x0EU,        // OpCode Br_table.
      0xFFU, 0x1FU  // Vector length = 4095
  };
  // The labels and the label index, then Unreachable and the expression End.
  Expected.insert(Expected.end(), 4096, static_cast<uint8_t>(0x02U));
  Expected.insert(Expected.end(), {0x00U, 0x0BU});
  EXPECT_EQ(Output, Expected);

  Instrs = {};
  Instrs.appendSelectT(
      std::vector<WasmEdge::ValType>(8191, WasmEdge::TypeCode::I32));
  Instrs.append(WasmEdge::OpCode::Unreachable);
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
  Expected = {
      0x0AU,        // Code section
      0x88U, 0x40U, // Content size = 8200
      0x01U,        // Vector length = 1
      0x85U, 0x40U, // Code segment size = 8197
      0x00U,        // Local vec(0)
      0x1CU,        // OpCode Select_t.
      0xFFU, 0x3FU  // Vector length = 8191
  };
  // The value types, then Unreachable and the expression End.
  Expected.insert(Expected.end(), 8191, static_cast<uint8_t>(0x7FU));
  Expected.insert(Expected.end(), {0x00U, 0x0BU});
  EXPECT_EQ(Output, Expected);

  WasmEdge::BlockType Type;
  Type.setEmpty();
  WasmEdge::AST::Instruction::CatchDescriptor Clause = {};
  Clause.IsAll = true;
  Clause.LabelIndex = 0x01U;
  Instrs = {};
  Instrs.appendTryTable(
      Type,
      std::vector<WasmEdge::AST::Instruction::CatchDescriptor>(2340, Clause));
  Instrs.append(WasmEdge::OpCode::Unreachable);
  Instrs.appendEnd();
  Instrs.appendEnd();
  Output = {};
  Ser.serializeSection(createCodeSec(Instrs), Output);
  Expected = {
      0x0AU,        // Code section
      0xD3U, 0x24U, // Content size = 4691
      0x01U,        // Vector length = 1
      0xD0U, 0x24U, // Code segment size = 4688
      0x00U,        // Local vec(0)
      0x1FU,        // OpCode Try_table.
      0x40U,        // OpCode Epsilon.
      0xA4U, 0x12U  // Vector length = 2340
  };
  // The catch flags and label indices, then Unreachable and both Ends.
  for (uint32_t I = 0; I < 2340; ++I) {
    Expected.insert(Expected.end(), {0x02U, 0x01U});
  }
  Expected.insert(Expected.end(), {0x00U, 0x0BU, 0x0BU});
  EXPECT_EQ(Output, Expected);

  const size_t LabelNum =
      WasmEdge::AST::InstrVec::MaxSize /
          sizeof(WasmEdge::AST::Instruction::JumpDescriptor) +
      1;
  const std::unique_ptr<uint32_t[]> Labels(new uint32_t[LabelNum]);
  Instrs = {};
  EXPECT_EQ(Instrs.appendBrTable(
                WasmEdge::Span<const uint32_t>(Labels.get(), LabelNum)),
            WasmEdge::AST::InstrVec::InvalidPos);
  EXPECT_TRUE(Instrs.empty());
}

} // namespace
