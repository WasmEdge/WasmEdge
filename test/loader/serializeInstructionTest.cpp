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
  WasmEdge::Configure ConfWASM2;
  ConfWASM2.setWASMStandard(WasmEdge::Standard::WASM_2);
  WasmEdge::Loader::Serializer SerWASM2(ConfWASM2);

  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  std::vector<WasmEdge::AST::Instruction> Instructions;

  // 3. Test branch control instructions.
  //
  //   1.  Serialize valid label index.
  //   2.  Serialize Br_on_null instruction with Func-Ref proposal.
  //   3.  Serialize Br_on_non_null instruction with Func-Ref proposal.
  //   4.  Serialize invalid Br_on_non_null instruction without Func-Ref
  //   proposal.
  //   5.  Serialize Br_on_cast instruction.
  //   6.  Serialize Br_on_cast_fail instruction.
  //   7.  Serialize Br_on_cast_fail instruction without the GC proposal.

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

  BrOnNull.getJump().TargetIndex = 0xFFFFFFFFU;
  Instructions = {BrOnNull, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected[5] = 0xD5U; // OpCode Br_on_null
  EXPECT_EQ(Output, Expected);

  BrOnNonNull.getJump().TargetIndex = 0xFFFFFFFFU;
  Instructions = {BrOnNonNull, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected[5] = 0xD6U; // OpCode Br_on_non_null
  EXPECT_EQ(Output, Expected);

  // Test without Func-Ref proposal
  Instructions = {BrOnNonNull, End};
  Output = {};
  EXPECT_FALSE(SerWASM2.serializeSection(createCodeSec(Instructions), Output));

  BrOnCast.setBrCast(0xFFFFFFFFU);
  BrOnCast.getBrCast().RType1 = WasmEdge::TypeCode::AnyRef;
  BrOnCast.getBrCast().RType2 = WasmEdge::TypeCode::EqRef;
  Instructions = {BrOnCast, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
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
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected[6] = 0x19; // OpCode Br_on_cast_fail.
  EXPECT_EQ(Output, Expected);

  // Test without GC proposal
  Output = {};
  EXPECT_FALSE(SerWASM2.serializeSection(createCodeSec(Instructions), Output));
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
  WasmEdge::Configure ConfWASM1;
  ConfWASM1.setWASMStandard(WasmEdge::Standard::WASM_1);
  WasmEdge::Loader::Serializer SerWASM1(ConfWASM1);

  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  std::vector<WasmEdge::AST::Instruction> Instructions;

  // 5. Test call control instructions.
  //
  //   1.  Serialize call instruction with valid type index.
  //   2.  Serialize call_indirect instruction with valid type and table index.
  //   3.  Serialize call_indirect instruction with invalid table index without
  //       Ref-Types proposal.
  //   4.  Serialize Call_ref instruction with valid type index.
  //   5.  Serialize Return_call_ref instruction with valid type and table.
  //   6.  Serialize Return_call_ref without Func-Ref proposal.

  WasmEdge::AST::Instruction Call(WasmEdge::OpCode::Call);
  WasmEdge::AST::Instruction CallIndirect(WasmEdge::OpCode::Call_indirect);
  WasmEdge::AST::Instruction CallRef(WasmEdge::OpCode::Call_ref);
  WasmEdge::AST::Instruction ReturnCallRef(WasmEdge::OpCode::Return_call_ref);
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

  EXPECT_FALSE(SerWASM1.serializeSection(createCodeSec(Instructions), Output));

  Conf.addProposal(WasmEdge::Proposal::FunctionReferences);
  CallRef.getTargetIndex() = 0xFFFFFFFFU;
  Instructions = {CallRef, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
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

  Conf.addProposal(WasmEdge::Proposal::TailCall);
  ReturnCallRef.getTargetIndex() = 0xFFFFFFFFU;
  Instructions = {ReturnCallRef, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected[5] = 0x15U; // OpCode Return_call_ref.
  EXPECT_EQ(Output, Expected);

  ReturnCallRef.getTargetIndex() = 0xFFFFFFFFU;
  Instructions = {ReturnCallRef, End};
  Output = {};
  EXPECT_FALSE(SerWASM1.serializeSection(createCodeSec(Instructions), Output));
}

TEST(SerializeInstructionTest, SerializeEHControlInstruction) {
  WasmEdge::Configure ConfWASM2;
  ConfWASM2.setWASMStandard(WasmEdge::Standard::WASM_2);
  WasmEdge::Loader::Serializer SerWASM2(ConfWASM2);

  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  std::vector<WasmEdge::AST::Instruction> Instructions;

  // 6. Test exception handling instruction.
  //
  //   1.  Serialize Throw_ref instruction.
  //   2.  Serialize Throw_ref instruction without the exception handling
  //   proposal.
  //   3.  Serialize Throw instruction.
  //   4.  Serialize Throw instruction without the exception handling proposal.
  //   5.  Serialize Try_table instruction.
  //   6.  Serialize Try_table instruction without the exception handling
  //   proposal.

  WasmEdge::AST::Instruction ThrowRef(WasmEdge::OpCode::Throw_ref);
  WasmEdge::AST::Instruction Throw(WasmEdge::OpCode::Throw);
  WasmEdge::AST::Instruction TryTable(WasmEdge::OpCode::Try_table);
  WasmEdge::AST::Instruction End(WasmEdge::OpCode::End);

  Conf.addProposal(WasmEdge::Proposal::ExceptionHandling);
  Instructions = {ThrowRef, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
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
  EXPECT_FALSE(SerWASM2.serializeSection(createCodeSec(Instructions), Output));

  Throw.getTargetIndex() = 0xFFFFFFFFU;
  Instructions = {Throw, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
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
  EXPECT_FALSE(SerWASM2.serializeSection(createCodeSec(Instructions), Output));

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
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
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

  Output = {};
  EXPECT_FALSE(SerWASM2.serializeSection(createCodeSec(Instructions), Output));
}

TEST(SerializeInstructionTest, SerializeReferenceInstruction) {
  WasmEdge::Configure ConfWASM1;
  ConfWASM1.setWASMStandard(WasmEdge::Standard::WASM_1);
  WasmEdge::Loader::Serializer SerWASM1(ConfWASM1);

  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  std::vector<WasmEdge::AST::Instruction> Instructions;

  // 7. Test reference instructions.
  //
  //   1.  Serialize function reference type.
  //   2.  Serialize invalid reference type without Ref-Types proposal.
  //   3.  Serialize Ref_as_non_null instruction with valid type index.
  //   4.  Serialize Ref_as_non_null instruction without Func-Ref proposal.
  //   5.  Serializs Ref__eq instruction.
  //   6.  Serialize Ref__i31 instruction.
  //   7.  Serialize Ref__test instruction.
  //   8.  Serialize Ref__test_null instruction.
  //   9.  Serialize Ref__cast instruction.
  //  10.  Serialize Ref__cast_null instruction.
  //  11.  Serialize Ref__cast_null instruction without the GC proposal.
  //  12.  Serialize Any__convert_extern instruction.
  //  13.  Serialize Extern__convert_any instruction.
  //  14.  Serialize Extern__convert_any instruction without the GC proposal
  //  15.  Serialize I31__get_s instruction.
  //  16.  Serialize I31__get_u instruction.
  //  17.  Serialize I31__get_u instruction without the GC proposal.
  //  18.  Serialize Struct__new instruction.
  //  19.  Serialize Struct__new_default instruction.
  //  20.  Serialize Struct__get instruction.
  //  21.  Serialize Struct__get_s instruction.
  //  22.  Serialize Struct__get_u instruction.
  //  23.  Serialize Struct__set instruction.
  //  24.  Serialize Struct__set instruction without the GC proposal.
  //  25.  Serialize Array__new instruction.
  //  26.  Serialize Array__new_default instruction.
  //  27.  Serialize Array__get instruction.
  //  28.  Serialize Array__get_s instruction.
  //  29.  Serialize Array__get_u instruction.
  //  30.  Serialize Array__set instruction.
  //  33.  Serialize Array__fill instruction.
  //  34.  Serialize Array__len instruction.
  //  35.  Serialize Array__new_fixed instruction.
  //  36.  Serialize Array__new_data instruction.
  //  37.  Serialize Array__new_elem instruction.
  //  38.  Serialize Array__copy instruction.
  //  39.  Serialize Array__init_data instruction.
  //  40.  Serialize Array__init_elem instruction.
  //  42.  Serialize Array__init_elem instruction without the GC proposal.

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
  EXPECT_FALSE(SerWASM1.serializeSection(createCodeSec(Instructions), Output));

  Conf.addProposal(WasmEdge::Proposal::FunctionReferences);
  Instructions = {RefAsNonNull, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
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

  // Test without Func-Ref proposal
  Instructions = {RefAsNonNull, End};
  Output = {};
  EXPECT_FALSE(SerWASM1.serializeSection(createCodeSec(Instructions), Output));

  Instructions = {RefEq, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected[5] = 0xD3U; // OpCode Ref__eq.
  EXPECT_EQ(Expected, Output);

  Instructions = {RefI31, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
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
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
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
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected[6] = 0x15U; // OpCode Ref__test_null.
  EXPECT_EQ(Output, Expected);

  RefCast.setValType(WasmEdge::TypeCode::FuncRef);
  Instructions = {RefCast, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected[6] = 0x16U; // OpCode Ref__cast.
  EXPECT_EQ(Output, Expected);

  RefCastNull.setValType(WasmEdge::TypeCode::FuncRef);
  Instructions = {RefCastNull, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected[6] = 0x17U; // OpCode Ref__cast_null.
  EXPECT_EQ(Output, Expected);

  Output = {};
  Instructions = {RefCastNull, End};
  EXPECT_FALSE(SerWASM1.serializeSection(createCodeSec(Instructions), Output));

  Instructions = {AnyConvertExtern, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
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
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected[6] = 0x1BU; // OpCode Extern__convert_any.
  EXPECT_EQ(Output, Expected);

  Output = {};
  EXPECT_FALSE(SerWASM1.serializeSection(createCodeSec(Instructions), Output));

  Instructions = {I31GetS, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
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
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected[6] = 0x1EU; // OpCode I31__get_u.
  EXPECT_EQ(Output, Expected);

  Output = {};
  EXPECT_FALSE(SerWASM1.serializeSection(createCodeSec(Instructions), Output));

  StructNew.getTargetIndex() = 0xFFFFFFFFU;
  Instructions = {StructNew, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
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
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected[6] = 0x01U; // OpCode Struct__new_default.
  EXPECT_EQ(Output, Expected);

  StructGet.getSourceIndex() = 0x05U;
  StructGet.getTargetIndex() = 0xFFFFFFFFU;
  Instructions = {StructGet, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
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
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected[6] = 0x03U; // OpCode Struct__get_s.
  EXPECT_EQ(Output, Expected);

  StructGetU.getSourceIndex() = 0x05U;
  StructGetU.getTargetIndex() = 0xFFFFFFFFU;
  Instructions = {StructGetU, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected[6] = 0x04U; // OpCode Struct__get_u.
  EXPECT_EQ(Output, Expected);

  StructSet.getSourceIndex() = 0x05U;
  StructSet.getTargetIndex() = 0xFFFFFFFFU;
  Instructions = {StructSet, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected[6] = 0x05U; // OpCode Struct__set.
  EXPECT_EQ(Output, Expected);

  Output = {};
  EXPECT_FALSE(SerWASM1.serializeSection(createCodeSec(Instructions), Output));

  ArrayNew.getTargetIndex() = 0xFFFFFFFFU;
  Instructions = {ArrayNew, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
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
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected[6] = 0x07U; // OpCode Array__new_default.
  EXPECT_EQ(Output, Expected);

  ArrayGet.getTargetIndex() = 0xFFFFFFFFU;
  Instructions = {ArrayGet, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected[6] = 0x0BU; // OpCode Array__get.
  EXPECT_EQ(Output, Expected);

  ArrayGetS.getTargetIndex() = 0xFFFFFFFFU;
  Instructions = {ArrayGetS, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected[6] = 0x0CU; // OpCode Array__get_s.
  EXPECT_EQ(Output, Expected);

  ArrayGetU.getTargetIndex() = 0xFFFFFFFFU;
  Instructions = {ArrayGetU, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected[6] = 0x0DU; // OpCode Array__get_u.
  EXPECT_EQ(Output, Expected);

  ArraySet.getTargetIndex() = 0xFFFFFFFFU;
  Instructions = {ArraySet, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected[6] = 0x0EU; // OpCode Array__set.
  EXPECT_EQ(Output, Expected);

  ArrayFill.getTargetIndex() = 0xFFFFFFFFU;
  Instructions = {ArrayFill, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected[6] = 0x10U; // OpCode Array__fill.
  EXPECT_EQ(Output, Expected);

  Instructions = {ArrayLen, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
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
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
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
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected[6] = 0x09U; // OpCode Array__new_data.
  EXPECT_EQ(Output, Expected);

  ArrayNewElem.getSourceIndex() = 0x05U;
  ArrayNewElem.getTargetIndex() = 0xFFFFFFFFU;
  Instructions = {ArrayNewElem, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected[6] = 0x0AU; // OpCode Array__new_elem.
  EXPECT_EQ(Output, Expected);

  ArrayCopy.getSourceIndex() = 0x05U;
  ArrayCopy.getTargetIndex() = 0xFFFFFFFFU;
  Instructions = {ArrayCopy, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected[6] = 0x11U; // OpCode Array__copy.
  EXPECT_EQ(Output, Expected);

  ArrayInitData.getSourceIndex() = 0x05U;
  ArrayInitData.getTargetIndex() = 0xFFFFFFFFU;
  Instructions = {ArrayInitData, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected[6] = 0x12U; // OpCode Array__init_data.
  EXPECT_EQ(Output, Expected);

  ArrayInitElem.getSourceIndex() = 0x05U;
  ArrayInitElem.getTargetIndex() = 0xFFFFFFFFU;
  Instructions = {ArrayInitElem, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected[6] = 0x13U; // OpCode Array__init_elem.
  EXPECT_EQ(Output, Expected);

  Output = {};
  EXPECT_FALSE(SerWASM1.serializeSection(createCodeSec(Instructions), Output));
}

TEST(SerializeInstructionTest, SerializeParametricInstruction) {
  WasmEdge::Configure ConfWASM1;
  ConfWASM1.setWASMStandard(WasmEdge::Standard::WASM_1);
  WasmEdge::Loader::Serializer SerWASM1(ConfWASM1);

  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  std::vector<WasmEdge::AST::Instruction> Instructions;

  // 8. Test parametric instructions.
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
  EXPECT_FALSE(SerWASM1.serializeSection(createCodeSec(Instructions), Output));
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

  // 10. Test table instructions.
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

  // 11. Test memory instructions.
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

  // 12. Test const numeric instructions.
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

TEST(SerializeInstructionTest, SerializeSwizzleInstruction) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  std::vector<WasmEdge::AST::Instruction> Instructions;

  // 13. Test swizzle instruction.
  //
  //   1.  Serialize I8x16__relaxed_swizzle instruction.
  //   2.  Serialize I8x16__relaxed_swizzle without RelaxSIMD proposal.

  Conf.addProposal(WasmEdge::Proposal::RelaxSIMD);
  WasmEdge::AST::Instruction I8x16RelaxedSwizzle(
      WasmEdge::OpCode::I8x16__relaxed_swizzle);
  WasmEdge::AST::Instruction End(WasmEdge::OpCode::End);

  Instructions = {I8x16RelaxedSwizzle, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
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

  Conf.removeProposal(WasmEdge::Proposal::RelaxSIMD);
  Output = {};
  Instructions = {I8x16RelaxedSwizzle, End};
  EXPECT_FALSE(Ser.serializeSection(createCodeSec(Instructions), Output));
}

TEST(SerializeInstructionTest, SerializeTruncInstruction) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  std::vector<WasmEdge::AST::Instruction> Instructions;

  // 14. Test trunc instruction.
  //
  //   1.  Serialize I32x4__relaxed_trunc_f32x4_s instruction.
  //   2.  Serialize I32x4__relaxed_trunc_f32x4_u instruction.
  //   3.  Serialize I32x4__relaxed_trunc_f64x2_s_zero instruction.
  //   4.  Serialize I32x4__relaxed_trunc_f64x2_u_zero instruction.
  //   5.  Serialize I32x4__relaxed_trunc_f64x2_u_zero without RelaxSIMD
  //   proposal.

  Conf.addProposal(WasmEdge::Proposal::RelaxSIMD);
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
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
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
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected[6] = 0x82U; // OpCode I32x4__relaxed_trunc_f32x4_u
  EXPECT_EQ(Output, Expected);

  Instructions = {I32x4RelaxedTruncF64x2SZero, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected[6] = 0x83U; // OpCode I32x4__relaxed_trunc_f64x2_s_zero
  EXPECT_EQ(Output, Expected);

  Instructions = {I32x4RelaxedTruncF64x2UZero, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected[6] = 0x84U; // OpCode I32x4__relaxed_trunc_f64x2_u_zero
  EXPECT_EQ(Output, Expected);

  Conf.removeProposal(WasmEdge::Proposal::RelaxSIMD);
  Instructions = {I32x4RelaxedTruncF64x2UZero, End};
  Output = {};
  EXPECT_FALSE(Ser.serializeSection(createCodeSec(Instructions), Output));
}

TEST(SerializeInstructionTest, SerializeMulAddInstruction) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  std::vector<WasmEdge::AST::Instruction> Instructions;

  // 15. Test multiply-add instruction.
  //
  //   1.  Serialize F32x4__relaxed_madd instruction.
  //   2.  Serialize F32x4__relaxed_nmadd instruction.
  //   3.  Serialize F64x2__relaxed_madd instruction.
  //   4.  Serialize F64x2__relaxed_nmadd instruction.
  //   5.  Serialize F64x2__relaxed_nmadd without RelaxSIMD proposal.

  Conf.addProposal(WasmEdge::Proposal::RelaxSIMD);
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
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
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
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected[6] = 0x86U; // OpCode F32x4__relaxed_nmadd.
  EXPECT_EQ(Output, Expected);

  Instructions = {F64x2RelaxedMadd, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected[6] = 0x87U; // OpCode F64x2__relaxed_madd.
  EXPECT_EQ(Output, Expected);

  Instructions = {F64x2RelaxedNMadd, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected[6] = 0x88U; // OpCode F64x2__relaxed_nmadd.
  EXPECT_EQ(Output, Expected);

  Conf.removeProposal(WasmEdge::Proposal::RelaxSIMD);
  Instructions = {F64x2RelaxedNMadd, End};
  Output = {};
  EXPECT_FALSE(Ser.serializeSection(createCodeSec(Instructions), Output));
}

TEST(SerializeInstructionTest, SerializeLaneSelectInstruction) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  std::vector<WasmEdge::AST::Instruction> Instructions;

  // 16. Test laneselect instruction.
  //
  //   1.  Serialize I8x16__relaxed_laneselect instruction.
  //   2.  Serialize I16x8__relaxed_laneselect instruction.
  //   3.  Serialize I32x4__relaxed_laneselect instruction.
  //   4.  Serialize I64x2__relaxed_laneselect instruction.
  //   5.  Serialize I64x2__relaxed_laneselect without RelaxSIMD proposal.

  Conf.addProposal(WasmEdge::Proposal::RelaxSIMD);
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
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
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
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected[6] = 0x8AU; // OpCode I16x8__relaxed_laneselect.
  EXPECT_EQ(Output, Expected);

  Instructions = {I32x4RelaxedLaneSelect, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected[6] = 0x8BU; // OpCode I32x4__relaxed_laneselect.
  EXPECT_EQ(Output, Expected);

  Instructions = {I64x2RelaxedLaneSelect, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected[6] = 0x8CU; // OpCode I64x2__relaxed_laneselect.

  Conf.removeProposal(WasmEdge::Proposal::RelaxSIMD);
  Instructions = {I64x2RelaxedLaneSelect, End};
  Output = {};
  EXPECT_FALSE(Ser.serializeSection(createCodeSec(Instructions), Output));
}

TEST(SerializeInstructionTest, SerializeMinMaxInstruction) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  std::vector<WasmEdge::AST::Instruction> Instructions;

  // 17. Test laneselect instruction.
  //
  //   1.  Serialize F32x4__relaxed_min instruction.
  //   2.  Serialize F32x4__relaxed_max instruction.
  //   3.  Serialize F64x2__relaxed_min instruction.
  //   4.  Serialize F64x2__relaxed_max instruction.
  //   5.  Serialize F64x2__relaxed_max without RelaxSIMD proposal.

  Conf.addProposal(WasmEdge::Proposal::RelaxSIMD);
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
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
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
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected[6] = 0x8EU; // OpCode F32x4__relaxed_max.
  EXPECT_EQ(Output, Expected);

  Instructions = {F64x2RelaxedMin, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected[6] = 0x8FU; // OpCode F64x2__relaxed_min.
  EXPECT_EQ(Output, Expected);

  Instructions = {F64x2RelaxedMax, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected[6] = 0x90U; // OpCode F64x2__relaxed_max.
  EXPECT_EQ(Output, Expected);

  Conf.removeProposal(WasmEdge::Proposal::RelaxSIMD);
  Instructions = {F64x2RelaxedMax, End};
  Output = {};
  EXPECT_FALSE(Ser.serializeSection(createCodeSec(Instructions), Output));
}

TEST(SerializeInstructionTest, SerializeQ15MulRInstruction) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  std::vector<WasmEdge::AST::Instruction> Instructions;

  // 18. Test rounding Q-format multiplication instruction.
  //
  //   1.  Serialize I16x8__relaxed_q15mulr_s instruction.
  //   2.  Serialize I16x8__relaxed_q15mulr_s instruction without RelaxSIMD
  //   proposal.

  Conf.addProposal(WasmEdge::Proposal::RelaxSIMD);
  WasmEdge::AST::Instruction I16x8RelaxedQ15MulRS(
      WasmEdge::OpCode::I16x8__relaxed_q15mulr_s);
  WasmEdge::AST::Instruction End(WasmEdge::OpCode::End);

  Instructions = {I16x8RelaxedQ15MulRS, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
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

  Conf.removeProposal(WasmEdge::Proposal::RelaxSIMD);
  Instructions = {I16x8RelaxedQ15MulRS, End};
  Output = {};
  EXPECT_FALSE(Ser.serializeSection(createCodeSec(Instructions), Output));
}

TEST(SerializeInstructionTest, SerializeDotProductInstruction) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;
  std::vector<WasmEdge::AST::Instruction> Instructions;

  // 19. Test dot product instruction.
  //
  //   1.  Serialize I16x8__relaxed_dot_i8x16_i7x16_s instruction.
  //   2.  Serialize I32x4__relaxed_dot_i8x16_i7x16_add_s instruction.
  //   3.  Serialize I32x4__relaxed_dot_i8x16_i7x16_add_s instruction without
  //   RelaxSIMD proposal.

  Conf.addProposal(WasmEdge::Proposal::RelaxSIMD);
  WasmEdge::AST::Instruction I16x8RelaxedDotI8x16i7x16S(
      WasmEdge::OpCode::I16x8__relaxed_dot_i8x16_i7x16_s);
  WasmEdge::AST::Instruction I16x8RelaxedDotI8x16i7x16AddS(
      WasmEdge::OpCode::I32x4__relaxed_dot_i8x16_i7x16_add_s);
  WasmEdge::AST::Instruction End(WasmEdge::OpCode::End);

  Instructions = {I16x8RelaxedDotI8x16i7x16S, End};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
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
  EXPECT_TRUE(Ser.serializeSection(createCodeSec(Instructions), Output));
  Expected[6] = 0x93U; // OpCode I32x4__relaxed_dot_i8x16_i7x16_add_s.
  EXPECT_EQ(Output, Expected);

  Conf.removeProposal(WasmEdge::Proposal::RelaxSIMD);
  Instructions = {I16x8RelaxedDotI8x16i7x16AddS, End};
  Output = {};
  EXPECT_FALSE(Ser.serializeSection(createCodeSec(Instructions), Output));
}
} // namespace
