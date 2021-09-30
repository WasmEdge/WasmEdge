// SPDX-License-Identifier: Apache-2.0
//===-- wasmedge/test/ast/instructionTest.cpp - Instruction unit tests ----===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contents unit tests of Instruction nodes.
///
//===----------------------------------------------------------------------===//

#include "ast/instruction.h"

#include "ast/expression.h"
#include "gtest/gtest.h"

namespace {

WasmEdge::FileMgr Mgr;
WasmEdge::Configure Conf;

TEST(InstructionTest, LoadBlockControlInstruction) {
  /// 1. Test block control instructions.
  ///
  ///   1.  Load invalid empty-body block.
  ///   2.  Load block with only end operation.
  ///   3.  Load loop with only end operation.
  ///   4.  Load block with invalid operations.
  ///   5.  Load loop with invalid operations.
  ///   6.  Load block with instructions.
  ///   7.  Load loop with instructions.
  Mgr.setCode(std::vector<uint8_t>());
  WasmEdge::AST::Expression Exp1;
  EXPECT_FALSE(Exp1.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec2 = {
      0x02U, /// OpCode Block.
      0x40U, /// Block type.
      0x0BU, /// OpCode End.
      0x0BU  /// Expression End.
  };
  Mgr.setCode(Vec2);
  WasmEdge::AST::Expression Exp2;
  EXPECT_TRUE(Exp2.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);

  std::vector<unsigned char> Vec3 = {
      0x03U, /// OpCode Loop.
      0x40U, /// Block type.
      0x0BU, /// OpCode End.
      0x0BU  /// Expression End.
  };
  Mgr.setCode(Vec3);
  WasmEdge::AST::Expression Exp3;
  EXPECT_TRUE(Exp3.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);

  std::vector<unsigned char> Vec4 = {
      0x02U,               /// OpCode Block.
      0x40U,               /// Block type.
      0x45U, 0x46U, 0x47U, /// Valid OpCodes.
      0xEDU, 0xEEU, 0xEFU, /// Invalid OpCodes.
      0x0BU,               /// OpCode End.
      0x0BU                /// Expression End.
  };
  Mgr.setCode(Vec4);
  WasmEdge::AST::Expression Exp4;
  EXPECT_FALSE(Exp4.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec5 = {
      0x03U,               /// OpCode Loop.
      0x40U,               /// Block type.
      0x45U, 0x46U, 0x47U, /// Valid OpCodes.
      0xEDU, 0xEEU, 0xEFU, /// Invalid OpCodes.
      0x0BU,               /// OpCode End.
      0x0BU                /// Expression End.
  };
  Mgr.setCode(Vec5);
  WasmEdge::AST::Expression Exp5;
  EXPECT_FALSE(Exp5.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec6 = {
      0x02U,               /// OpCode Block.
      0x40U,               /// Block type.
      0x45U, 0x46U, 0x47U, /// Valid OpCodes.
      0x0BU,               /// OpCode End.
      0x0BU                /// Expression End.
  };
  Mgr.setCode(Vec6);
  WasmEdge::AST::Expression Exp6;
  EXPECT_TRUE(Exp6.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);

  std::vector<unsigned char> Vec7 = {
      0x03U,               /// OpCode Loop.
      0x40U,               /// Block type.
      0x45U, 0x46U, 0x47U, /// Valid OpCodes.
      0x0BU,               /// OpCode End.
      0x0BU                /// Expression End.
  };
  Mgr.setCode(Vec7);
  WasmEdge::AST::Expression Exp7;
  EXPECT_TRUE(Exp7.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);
}

TEST(InstructionTest, LoadIfElseControlInstruction) {
  /// 2. Test load if-else control instruction.
  ///
  ///   1.  Load invalid empty-body if statement.
  ///   2.  Load if statement with only end operation.
  ///   3.  Load if and else statements with only end operation.
  ///   4.  Load if statement with invalid operations.
  ///   5.  Load if and else statements with invalid operations.
  ///   6.  Load if statement with instructions.
  ///   7.  Load if and else statements with instructions.
  ///   8.  Load invalid else instruction out of block.
  ///   9.  Load invalid else instruction out of if statement.
  ///   10. Load invalid else instruction duplicated in if statement.
  WasmEdge::AST::Expression Exp1;
  std::vector<unsigned char> Vec1 = {
      0x04U /// OpCode If.
  };
  Mgr.setCode(Vec1);
  EXPECT_FALSE(Exp1.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec2 = {
      0x04U, /// OpCode If.
      0x40U, /// Block type.
      0x0BU, /// OpCode End.
      0x0BU  /// Expression End.
  };
  Mgr.setCode(Vec2);
  WasmEdge::AST::Expression Exp2;
  EXPECT_TRUE(Exp2.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);

  std::vector<unsigned char> Vec3 = {
      0x04U, /// OpCode If.
      0x40U, /// Block type.
      0x05U, /// OpCode Else
      0x0BU, /// OpCode End.
      0x0BU  /// Expression End.
  };
  Mgr.setCode(Vec3);
  WasmEdge::AST::Expression Exp3;
  EXPECT_TRUE(Exp3.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);

  std::vector<unsigned char> Vec4 = {
      0x04U,               /// OpCode If.
      0x40U,               /// Block type.
      0xEDU, 0xEEU, 0xEFU, /// Invalid OpCodes in if statement.
      0x0BU,               /// OpCode End.
      0x0BU                /// Expression End.
  };
  Mgr.setCode(Vec4);
  WasmEdge::AST::Expression Exp4;
  EXPECT_FALSE(Exp4.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec5 = {
      0x04U,               /// OpCode If.
      0x40U,               /// Block type.
      0x45U, 0x46U, 0x47U, /// Valid OpCodes in if statement.
      0x05U,               /// OpCode Else
      0xEDU, 0xEEU, 0xEFU, /// Invalid OpCodes in else statement.
      0x0BU,               /// OpCode End.
      0x0BU                /// Expression End.
  };
  Mgr.setCode(Vec5);
  WasmEdge::AST::Expression Exp5;
  EXPECT_FALSE(Exp5.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec6 = {
      0x04U,               /// OpCode If.
      0x40U,               /// Block type.
      0x45U, 0x46U, 0x47U, /// Valid OpCodes in if statement.
      0x0BU,               /// OpCode End.
      0x0BU                /// Expression End.
  };
  Mgr.setCode(Vec6);
  WasmEdge::AST::Expression Exp6;
  EXPECT_TRUE(Exp6.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);

  std::vector<unsigned char> Vec7 = {
      0x04U,               /// OpCode If.
      0x40U,               /// Block type.
      0x45U, 0x46U, 0x47U, /// Valid OpCodes in if statement.
      0x05U,               /// OpCode Else
      0x45U, 0x46U, 0x47U, /// Valid OpCodes in else statement.
      0x0BU,               /// OpCode End.
      0x0BU                /// Expression End.
  };
  Mgr.setCode(Vec7);
  WasmEdge::AST::Expression Exp7;
  EXPECT_TRUE(Exp7.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);

  WasmEdge::AST::Expression Exp8;
  std::vector<unsigned char> Vec8 = {
      0x45U, 0x46U, 0x47U, /// Valid OpCodes.
      0x05U,               /// OpCode Else.
      0x45U, 0x46U, 0x47U, /// Valid OpCodes.
      0x0BU                /// Expression End.
  };
  Mgr.setCode(Vec8);
  EXPECT_FALSE(Exp8.loadBinary(Mgr, Conf));

  WasmEdge::AST::Expression Exp9;
  std::vector<unsigned char> Vec9 = {
      0x02U,               /// OpCode Block.
      0x40U,               /// Block type.
      0x45U, 0x46U, 0x47U, /// Valid OpCodes.
      0x05U,               /// OpCode Else.
      0x45U, 0x46U, 0x47U, /// Valid OpCodes.
      0x0BU,               /// OpCode End.
      0x0BU                /// Expression End.
  };
  Mgr.setCode(Vec9);
  EXPECT_FALSE(Exp9.loadBinary(Mgr, Conf));

  WasmEdge::AST::Expression Exp10;
  std::vector<unsigned char> Vec10 = {
      0x04U,               /// OpCode If.
      0x40U,               /// Block type.
      0x45U, 0x46U, 0x47U, /// Valid OpCodes in if statement.
      0x05U,               /// OpCode Else
      0x05U,               /// Duplicated OpCode Else
      0x45U, 0x46U, 0x47U, /// Valid OpCodes in else statement.
      0x0BU,               /// OpCode End.
      0x0BU                /// Expression End.
  };
  Mgr.setCode(Vec10);
  EXPECT_FALSE(Exp10.loadBinary(Mgr, Conf));
}

TEST(InstructionTest, LoadBrControlInstruction) {
  /// 3. Test branch control instructions.
  ///
  ///   1.  Load invalid empty label index.
  ///   2.  Load valid label index.
  Mgr.setCode(std::vector<uint8_t>());
  WasmEdge::AST::Instruction Ins1(WasmEdge::OpCode::Br);
  EXPECT_FALSE(Ins1.loadBinary(Mgr, Conf));
  Mgr.setCode(std::vector<uint8_t>());
  WasmEdge::AST::Instruction Ins2(WasmEdge::OpCode::Br_if);
  EXPECT_FALSE(Ins2.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec2 = {
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU /// Label index.
  };
  Mgr.setCode(Vec2);
  WasmEdge::AST::Instruction Ins3(WasmEdge::OpCode::Br);
  EXPECT_TRUE(Ins3.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);
  Mgr.setCode(Vec2);
  WasmEdge::AST::Instruction Ins4(WasmEdge::OpCode::Br_if);
  EXPECT_TRUE(Ins4.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);
}

TEST(InstructionTest, LoadBrTableControlInstruction) {
  /// 4. Test branch table control instruction.
  ///
  ///   1.  Load invalid empty instruction body.
  ///   2.  Load instruction with empty label vector.
  ///   3.  Load instruction with label vector.
  ///   4.  Load instruction with wrong length of label vector.
  Mgr.setCode(std::vector<uint8_t>());
  WasmEdge::AST::Instruction Ins1(WasmEdge::OpCode::Br_table);
  EXPECT_FALSE(Ins1.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec2 = {
      0x00U,                            /// Vector length = 0
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU /// Label index.
  };
  Mgr.setCode(Vec2);
  WasmEdge::AST::Instruction Ins2(WasmEdge::OpCode::Br_table);
  EXPECT_TRUE(Ins2.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);

  std::vector<unsigned char> Vec3 = {
      0x03U,                             /// Vector length = 3
      0xF1U, 0xFFU, 0xFFU, 0xFFU, 0x0FU, /// vec[0]
      0xF2U, 0xFFU, 0xFFU, 0xFFU, 0x0FU, /// vec[1]
      0xF3U, 0xFFU, 0xFFU, 0xFFU, 0x0FU, /// vec[2]
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU  /// Label index.
  };
  Mgr.setCode(Vec3);
  WasmEdge::AST::Instruction Ins3(WasmEdge::OpCode::Br_table);
  EXPECT_TRUE(Ins3.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);

  std::vector<unsigned char> Vec4 = {
      0x03U, /// Vector length = 3
      0x01U, /// vec[0]
      0x02U  /// vec[1]
             /// Missed vec[2] and label index
  };
  Mgr.setCode(Vec4);
  WasmEdge::AST::Instruction Ins4(WasmEdge::OpCode::Br_table);
  EXPECT_FALSE(Ins4.loadBinary(Mgr, Conf));
}

TEST(InstructionTest, LoadCallControlInstruction) {
  /// 5. Test call control instructions.
  ///
  ///   1.  Load invalid empty call instruction body.
  ///   2.  Load invalid empty call_indirect instruction body.
  ///   3.  Load call instruction with valid type index.
  ///   4.  Load call_indirect instruction with valid type and table index.
  ///   5.  Load call_indirect instruction with unexpected end of type index.
  ///   6.  Load call_indirect instruction with unexpected end of table index.
  ///   7.  Load call_indirect instruction with invalid table index without
  ///       Ref-Types proposal.
  Mgr.setCode(std::vector<uint8_t>());
  WasmEdge::AST::Instruction Ins1(WasmEdge::OpCode::Call);
  EXPECT_FALSE(Ins1.loadBinary(Mgr, Conf));

  Mgr.setCode(std::vector<uint8_t>());
  WasmEdge::AST::Instruction Ins2(WasmEdge::OpCode::Call_indirect);
  EXPECT_FALSE(Ins2.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec3 = {
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU /// Function index.
  };
  Mgr.setCode(Vec3);
  WasmEdge::AST::Instruction Ins3(WasmEdge::OpCode::Call);
  EXPECT_TRUE(Ins3.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);

  std::vector<unsigned char> Vec4 = {
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU, /// Type index.
      0x05U                              /// Table index.
  };
  Mgr.setCode(Vec4);
  WasmEdge::AST::Instruction Ins4(WasmEdge::OpCode::Call_indirect);
  EXPECT_TRUE(Ins4.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);

  Mgr.setCode(std::vector<uint8_t>());
  WasmEdge::AST::Instruction Ins5(WasmEdge::OpCode::Call_indirect);
  EXPECT_FALSE(Ins5.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec6 = {
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU /// Type index.
      /// 0x00U                         /// Missed table index.
  };
  Mgr.setCode(Vec6);
  WasmEdge::AST::Instruction Ins6(WasmEdge::OpCode::Call_indirect);
  EXPECT_FALSE(Ins6.loadBinary(Mgr, Conf));

  Conf.removeProposal(WasmEdge::Proposal::ReferenceTypes);

  std::vector<unsigned char> Vec7 = {
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU, /// Type index.
      0x05U                              /// Table index.
  };
  Mgr.setCode(Vec7);
  WasmEdge::AST::Instruction Ins7(WasmEdge::OpCode::Call_indirect);
  EXPECT_FALSE(Ins7.loadBinary(Mgr, Conf));

  Conf.addProposal(WasmEdge::Proposal::ReferenceTypes);
}

TEST(InstructionTest, LoadReferenceInstruction) {
  /// 6. Test reference instructions.
  ///
  ///   1.  Load invalid empty reference type.
  ///   2.  Load invalid reference type without Ref-Types proposal.
  Mgr.setCode(std::vector<uint8_t>());
  WasmEdge::AST::Instruction Ins1(WasmEdge::OpCode::Ref__null);
  EXPECT_FALSE(Ins1.loadBinary(Mgr, Conf));

  Conf.removeProposal(WasmEdge::Proposal::ReferenceTypes);

  std::vector<unsigned char> Vec2 = {
      0x6FU /// ExternRef
  };
  Mgr.setCode(Vec2);
  WasmEdge::AST::Instruction Ins2(WasmEdge::OpCode::Ref__null);
  EXPECT_FALSE(Ins2.loadBinary(Mgr, Conf));

  Conf.addProposal(WasmEdge::Proposal::ReferenceTypes);
}

TEST(InstructionTest, LoadParametricInstruction) {
  /// 7. Test parametric instructions.
  ///
  ///   1.  Load valid select_t instruction with value type list.
  ///   2.  Load invalid empty value type list.
  ///   3.  Load invalid unexpected end of value type list.
  ///   4.  Load invalid value type list without Ref-Types proposal.
  std::vector<unsigned char> Vec1 = {
      0x02U,       /// Vector length = 2
      0x7FU, 0x7EU /// Value types
  };
  Mgr.setCode(Vec1);
  WasmEdge::AST::Instruction Ins1(WasmEdge::OpCode::Select_t);
  EXPECT_TRUE(Ins1.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);

  Mgr.setCode(std::vector<uint8_t>());
  WasmEdge::AST::Instruction Ins2(WasmEdge::OpCode::Select_t);
  EXPECT_FALSE(Ins2.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec3 = {
      0x03U,       /// Vector length = 3
      0x7FU, 0x7EU /// Value types list only in 2
  };
  Mgr.setCode(Vec3);
  WasmEdge::AST::Instruction Ins3(WasmEdge::OpCode::Select_t);
  EXPECT_FALSE(Ins3.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec4 = {
      0x02U,       /// Vector length = 2
      0x7BU, 0x7BU /// Value types with v128
  };
  Mgr.setCode(Vec4);
  WasmEdge::AST::Instruction Ins4(WasmEdge::OpCode::Select_t);
  EXPECT_FALSE(Ins4.loadBinary(Mgr, Conf));
}

TEST(InstructionTest, LoadVariableInstruction) {
  /// 8. Test variable instructions.
  ///
  ///   1.  Load invalid empty local or global index.
  ///   2.  Load valid local or global index.
  Mgr.setCode(std::vector<uint8_t>());
  WasmEdge::AST::Instruction Ins1(WasmEdge::OpCode::Local__get);
  EXPECT_FALSE(Ins1.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec2 = {
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU /// Local index.
  };
  Mgr.setCode(Vec2);
  WasmEdge::AST::Instruction Ins2(WasmEdge::OpCode::Local__get);
  EXPECT_TRUE(Ins2.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);
}

TEST(InstructionTest, LoadTableInstruction) {
  /// 9. Test table instructions.
  ///
  ///   1.  Load table_get instruction with unexpected end of table index.
  ///   2.  Load table_init instruction with unexpected end of table index.
  Mgr.setCode(std::vector<uint8_t>());
  WasmEdge::AST::Instruction Ins1(WasmEdge::OpCode::Table__get);
  EXPECT_FALSE(Ins1.loadBinary(Mgr, Conf));

  Mgr.setCode(std::vector<uint8_t>());
  WasmEdge::AST::Instruction Ins2(WasmEdge::OpCode::Table__init);
  EXPECT_FALSE(Ins2.loadBinary(Mgr, Conf));
}

TEST(InstructionTest, LoadMemoryInstruction) {
  /// 10. Test memory instructions.
  ///
  ///   1.  Load invalid empty memory args.
  ///   2.  Load memory_grow instruction with invalid empty checking byte.
  ///   3.  Load memory_grow instruction with invalid checking byte.
  ///   4.  Load valid memory args.
  ///   5.  Load memory_grow instruction with valid checking byte.
  ///   6.  Load memory_copy instruction with invalid checking byte.
  ///   7.  Load memory_init instruction with unexpected end of data index.
  Mgr.setCode(std::vector<uint8_t>());
  WasmEdge::AST::Instruction Ins1(WasmEdge::OpCode::I32__load);
  EXPECT_FALSE(Ins1.loadBinary(Mgr, Conf));

  Mgr.setCode(std::vector<uint8_t>());
  WasmEdge::AST::Instruction Ins2(WasmEdge::OpCode::Memory__grow);
  EXPECT_FALSE(Ins2.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec3 = {
      0xFFU /// Invalid checking byte.
  };
  Mgr.setCode(Vec3);
  WasmEdge::AST::Instruction Ins3(WasmEdge::OpCode::Memory__grow);
  EXPECT_FALSE(Ins3.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec4 = {
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU, /// Align.
      0xFEU, 0xFFU, 0xFFU, 0xFFU, 0x0FU  /// Offset.
  };
  Mgr.setCode(Vec4);
  WasmEdge::AST::Instruction Ins4(WasmEdge::OpCode::I32__load);
  EXPECT_TRUE(Ins4.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);

  std::vector<unsigned char> Vec5 = {
      0x00U /// Valid checking byte.
  };
  Mgr.setCode(Vec5);
  WasmEdge::AST::Instruction Ins5(WasmEdge::OpCode::Memory__grow);
  EXPECT_TRUE(Ins5.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);

  std::vector<unsigned char> Vec6 = {
      0x44U, /// Invalid checking byte 1.
      0x00U  /// Valid checking byte 2.
  };
  Mgr.setCode(Vec6);
  WasmEdge::AST::Instruction Ins6(WasmEdge::OpCode::Memory__copy);
  EXPECT_FALSE(Ins6.loadBinary(Mgr, Conf));

  Mgr.setCode(std::vector<uint8_t>());
  Conf.getRuntimeConfigure().addDataCountSection();
  WasmEdge::AST::Instruction Ins7(WasmEdge::OpCode::Memory__init);
  EXPECT_FALSE(Ins7.loadBinary(Mgr, Conf));
}

TEST(InstructionTest, LoadConstInstruction) {
  /// 11. Test const numeric instructions.
  ///
  ///   1.  Load I32 const numeric instruction.
  ///   2.  Load I64 const numeric instruction.
  ///   3.  Load F32 const numeric instruction.
  ///   4.  Load F64 const numeric instruction.
  ///   5.  Load invalid unexpected end of F32 const numeric instruction.
  ///   6.  Load invalid unexpected end of F64 const numeric instruction.
  std::vector<unsigned char> Vec1 = {
      0xC0U, 0xBBU, 0x78U /// I32 -123456.
  };
  Mgr.setCode(Vec1);
  WasmEdge::AST::Instruction Ins1(WasmEdge::OpCode::I32__const);
  EXPECT_TRUE(Ins1.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);

  std::vector<unsigned char> Vec2 = {
      0xC2U, 0x8EU, 0xF6U, 0xF2U, 0xDDU, 0x7CU /// I64 -112233445566
  };
  Mgr.setCode(Vec2);
  WasmEdge::AST::Instruction Ins2(WasmEdge::OpCode::I64__const);
  EXPECT_TRUE(Ins2.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);

  std::vector<unsigned char> Vec3 = {
      0xDA, 0x0F, 0x49, 0xC0 /// F32 -3.1415926
  };
  Mgr.setCode(Vec3);
  WasmEdge::AST::Instruction Ins3(WasmEdge::OpCode::F32__const);
  EXPECT_TRUE(Ins3.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);

  std::vector<unsigned char> Vec4 = {
      0x18, 0x2D, 0x44, 0x54, 0xFB, 0x21, 0x09, 0xC0 /// F64 -3.1415926535897932
  };
  Mgr.setCode(Vec4);
  WasmEdge::AST::Instruction Ins4(WasmEdge::OpCode::F64__const);
  EXPECT_TRUE(Ins4.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);

  std::vector<unsigned char> Vec5 = {
      0xDA, 0x0F /// F32 -3.1415926
                 /// 0x49, 0xC0  /// Missed 2 bytes
  };
  Mgr.setCode(Vec5);
  WasmEdge::AST::Instruction Ins5(WasmEdge::OpCode::F32__const);
  EXPECT_FALSE(Ins5.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec6 = {
      0x18, 0x2D, 0x44, 0x54, 0xFB /// F64 -3.1415926535897932
      /// 0x21, 0x09, 0xC0         /// Missed 3 bytes
  };
  Mgr.setCode(Vec6);
  WasmEdge::AST::Instruction Ins6(WasmEdge::OpCode::F64__const);
  EXPECT_FALSE(Ins6.loadBinary(Mgr, Conf));
}

TEST(InstructionTest, Proposals) {
  /// 12. Test ValTypes and instructions with disabled proposals
  ///
  ///   1.  Load if instruction with/without SIMD proposal.
  ///   2.  Load if instruction with reference instructions with/without
  ///       Ref-Types and Bulk-Mem proposals.
  ///   3.  Load select_t instruction with/without SIMD proposal.
  ///   4.  Load select_t instruction with reference instructions with/without
  ///       Ref-Types and Bulk-Mem proposals.
  ///   5.  Load if instruction with BlockType as result type with/without
  ///       Multi-Value proposal.
  ///   6.  Load saturating truncation instructions with/without NonTrap-Conv
  ///       proposal.
  ///   7.  Load sign extension instructions with/without Sign-Ext proposal.
  WasmEdge::AST::Expression Exp1;
  std::vector<unsigned char> Vec1 = {
      0x04U,                      /// OpCode If.
      0x7BU,                      /// Block type V128.
      0xFDU, 0x0CU,               /// OpCode V128__const.
      0x01U, 0x00U, 0x00U, 0x00U, /// 1.
      0x00U, 0x00U, 0x00U, 0x00U, /// 0.
      0x00U, 0x00U, 0x00U, 0x00U, /// 0.
      0x00U, 0x00U, 0x00U, 0x00U, /// 0.
      0x05U,                      /// OpCode Else.
      0xFDU, 0x0CU,               /// OpCode V128__const.
      0x02U, 0x00U, 0x00U, 0x00U, /// 2.
      0x00U, 0x00U, 0x00U, 0x00U, /// 0.
      0x00U, 0x00U, 0x00U, 0x00U, /// 0.
      0x00U, 0x00U, 0x00U, 0x00U, /// 0.
      0x0BU,                      /// OpCode End.
      0x0BU,                      /// Expression End.
  };
  Mgr.setCode(Vec1);
  EXPECT_FALSE(Exp1.loadBinary(Mgr, Conf));
  Conf.addProposal(WasmEdge::Proposal::SIMD);
  Mgr.setCode(Vec1);
  EXPECT_TRUE(Exp1.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);
  Conf.removeProposal(WasmEdge::Proposal::SIMD);

  WasmEdge::AST::Expression Exp2;
  std::vector<unsigned char> Vec2 = {
      0x04U,        /// OpCode If.
      0x70U,        /// Block type FuncRef.
      0xD0U, 0x70U, /// OpCode Ref__null func.
      0x05U,        /// OpCode Else.
      0xD0U, 0x70U, /// OpCode Ref__null func.
      0x0BU,        /// OpCode End.
      0x0BU,        /// Expression End.
  };
  Mgr.setCode(Vec2);
  EXPECT_TRUE(Exp2.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);
  Conf.removeProposal(WasmEdge::Proposal::ReferenceTypes);
  Conf.removeProposal(WasmEdge::Proposal::BulkMemoryOperations);
  Mgr.setCode(Vec2);
  EXPECT_FALSE(Exp2.loadBinary(Mgr, Conf));
  Conf.addProposal(WasmEdge::Proposal::ReferenceTypes);
  Conf.addProposal(WasmEdge::Proposal::BulkMemoryOperations);

  WasmEdge::AST::Expression Exp3;
  std::vector<unsigned char> Vec3 = {
      0xFDU, 0x0CU,               /// OpCode V128__const.
      0x01U, 0x00U, 0x00U, 0x00U, /// 1.
      0x00U, 0x00U, 0x00U, 0x00U, /// 0.
      0x00U, 0x00U, 0x00U, 0x00U, /// 0.
      0x00U, 0x00U, 0x00U, 0x00U, /// 0.
      0xFDU, 0x0CU,               /// OpCode V128__const.
      0x02U, 0x00U, 0x00U, 0x00U, /// 2.
      0x00U, 0x00U, 0x00U, 0x00U, /// 0.
      0x00U, 0x00U, 0x00U, 0x00U, /// 0.
      0x00U, 0x00U, 0x00U, 0x00U, /// 0.
      0x41U, 0x01U,               /// OpCode I32__const 1.
      0x1CU,                      /// OpCode Select_t.
      0x01U, 0x7BU,               /// Select type V128.
      0x0BU,                      /// Expression End.
  };
  Mgr.setCode(Vec3);
  EXPECT_FALSE(Exp3.loadBinary(Mgr, Conf));
  Conf.addProposal(WasmEdge::Proposal::SIMD);
  Mgr.setCode(Vec3);
  EXPECT_TRUE(Exp3.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);
  Conf.removeProposal(WasmEdge::Proposal::SIMD);

  WasmEdge::AST::Expression Exp4;
  std::vector<unsigned char> Vec4 = {
      0xD0U, 0x70U, /// OpCode Ref__null func.
      0xD0U, 0x70U, /// OpCode Ref__null func.
      0x41U, 0x01U, /// OpCode I32__const 1.
      0x1CU,        /// OpCode Select_t.
      0x01U, 0x70U, /// Select type FuncRef.
      0x0BU,        /// Expression End.
  };
  Mgr.setCode(Vec4);
  EXPECT_TRUE(Exp4.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);
  Conf.removeProposal(WasmEdge::Proposal::ReferenceTypes);
  Conf.removeProposal(WasmEdge::Proposal::BulkMemoryOperations);
  Mgr.setCode(Vec4);
  EXPECT_FALSE(Exp4.loadBinary(Mgr, Conf));
  Conf.addProposal(WasmEdge::Proposal::ReferenceTypes);
  Conf.addProposal(WasmEdge::Proposal::BulkMemoryOperations);

  WasmEdge::AST::Expression Exp5;
  std::vector<unsigned char> Vec5 = {
      0x04U,        /// OpCode If.
      0x01U,        /// Block type function index 1.
      0xD0U, 0x70U, /// OpCode Ref__null func.
      0x05U,        /// OpCode Else.
      0xD0U, 0x70U, /// OpCode Ref__null func.
      0x0BU,        /// OpCode End.
      0x0BU,        /// Expression End.
  };
  Mgr.setCode(Vec5);
  EXPECT_TRUE(Exp5.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);
  Conf.removeProposal(WasmEdge::Proposal::MultiValue);
  Mgr.setCode(Vec5);
  EXPECT_FALSE(Exp5.loadBinary(Mgr, Conf));
  Conf.addProposal(WasmEdge::Proposal::MultiValue);

  WasmEdge::AST::Expression Exp6;
  std::vector<unsigned char> Vec6 = {
      0xFCU, 0x00U, /// OpCode I32__trunc_sat_f32_s.
      0xFCU, 0x01U, /// OpCode I32__trunc_sat_f32_u.
      0x0BU,        /// Expression End.
  };
  Mgr.setCode(Vec6);
  EXPECT_TRUE(Exp6.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);
  Conf.removeProposal(WasmEdge::Proposal::NonTrapFloatToIntConversions);
  Mgr.setCode(Vec6);
  EXPECT_FALSE(Exp6.loadBinary(Mgr, Conf));
  Conf.addProposal(WasmEdge::Proposal::NonTrapFloatToIntConversions);

  WasmEdge::AST::Expression Exp7;
  std::vector<unsigned char> Vec7 = {
      0xC0U, /// OpCode I32__extend8_s.
      0xC1U, /// OpCode I32__extend16_s.
      0xC2U, /// OpCode I64__extend8_s.
      0xC3U, /// OpCode I64__extend16_s.
      0xC4U, /// OpCode I64__extend32_s.
      0x0BU, /// Expression End.
  };
  Mgr.setCode(Vec7);
  EXPECT_TRUE(Exp7.loadBinary(Mgr, Conf) && Mgr.getRemainSize() == 0);
  Conf.removeProposal(WasmEdge::Proposal::SignExtensionOperators);
  Mgr.setCode(Vec7);
  EXPECT_FALSE(Exp7.loadBinary(Mgr, Conf));
  Conf.addProposal(WasmEdge::Proposal::SignExtensionOperators);
}

TEST(InstructionTest, LoadSIMDInstruction) {
  /// 13. Test SIMD instructions.
  ///
  ///   1.  Load invalid unexpected end memory align of V128__load.
  ///   2.  Load invalid unexpected end memory offset of V128__load.
  ///   3.  Load invalid unexpected end memory align of V128__load8_lane.
  ///   4.  Load invalid unexpected end memory offset of V128__load8_lane.
  ///   5.  Load invalid unexpected end lane index of V128__load8_lane.
  ///   6.  Load invalid unexpected end value list of I8x16__shuffle.
  ///   7.  Load invalid unexpected end lane index of I8x16__extract_lane_s.
  Mgr.setCode(std::vector<uint8_t>());
  WasmEdge::AST::Instruction Ins1(WasmEdge::OpCode::V128__load);
  EXPECT_FALSE(Ins1.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec2 = {
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU /// Align
      /// 0xFEU, 0xFFU, 0xFFU, 0xFFU, 0x0FU  /// Missed Offset
  };
  Mgr.setCode(Vec2);
  WasmEdge::AST::Instruction Ins2(WasmEdge::OpCode::V128__load);
  EXPECT_FALSE(Ins2.loadBinary(Mgr, Conf));

  Mgr.setCode(std::vector<uint8_t>());
  WasmEdge::AST::Instruction Ins3(WasmEdge::OpCode::V128__load8_lane);
  EXPECT_FALSE(Ins3.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec4 = {
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU /// Align
      /// 0xFEU, 0xFFU, 0xFFU, 0xFFU, 0x0FU  /// Missed Offset
  };
  Mgr.setCode(Vec4);
  WasmEdge::AST::Instruction Ins4(WasmEdge::OpCode::V128__load8_lane);
  EXPECT_FALSE(Ins4.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec5 = {
      0xFFU, 0xFFU, 0xFFU,
      0xFFU, 0x0FU, /// Align
      0xFEU, 0xFFU, 0xFFU,
      0xFFU, 0x0FU /// Offset
                   /// 0x22U  /// Missed lane index
  };
  Mgr.setCode(Vec5);
  WasmEdge::AST::Instruction Ins5(WasmEdge::OpCode::V128__load8_lane);
  EXPECT_FALSE(Ins5.loadBinary(Mgr, Conf));

  std::vector<unsigned char> Vec6 = {
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
      0xFFU, 0xFFU, 0xFFU, 0xFFU /// Value list
      /// 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU /// Missed 7 bytes
  };
  Mgr.setCode(Vec6);
  WasmEdge::AST::Instruction Ins6(WasmEdge::OpCode::I8x16__shuffle);
  EXPECT_FALSE(Ins6.loadBinary(Mgr, Conf));

  Mgr.setCode(std::vector<uint8_t>());
  WasmEdge::AST::Instruction Ins7(WasmEdge::OpCode::I8x16__extract_lane_s);
  EXPECT_FALSE(Ins7.loadBinary(Mgr, Conf));
}

} // namespace
