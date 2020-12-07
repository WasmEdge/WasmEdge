// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/test/ast/instructionTest.cpp - Instruction unit tests --------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contents unit tests of Instruction nodes.
///
//===----------------------------------------------------------------------===//

#include "ast/instruction.h"
#include "ast/expression.h"
#include "loader/filemgr.h"
#include "gtest/gtest.h"

namespace {

SSVM::FileMgrVector Mgr;
SSVM::ProposalConfigure PConf;

TEST(InstructionTest, LoadBlockControlInstruction) {
  /// TODO:
  /// 1. Test load block control instruction.
  ///
  ///   1.  Load invalid empty-body block.
  ///   2.  Load block with only end operation.
  ///   3.  Load loop with only end operation.
  ///   4.  Load block with invalid operations.
  ///   5.  Load loop with invalid operations.
  ///   6.  Load block with instructions.
  ///   7.  Load loop with instructions.

  Mgr.clearBuffer();
  SSVM::AST::Expression Exp1;
  EXPECT_FALSE(Exp1.loadBinary(Mgr, PConf));

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec2 = {
      0x02U, /// OpCode Block.
      0x40U, /// Block type.
      0x0BU, /// OpCode End.
      0x0BU  /// Expression End.
  };
  Mgr.setCode(Vec2);
  SSVM::AST::Expression Exp2;
  EXPECT_TRUE(Exp2.loadBinary(Mgr, PConf) && Mgr.getRemainSize() == 0);

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec3 = {
      0x03U, /// OpCode Loop.
      0x40U, /// Block type.
      0x0BU, /// OpCode End.
      0x0BU  /// Expression End.
  };
  Mgr.setCode(Vec3);
  SSVM::AST::Expression Exp3;
  EXPECT_TRUE(Exp3.loadBinary(Mgr, PConf) && Mgr.getRemainSize() == 0);

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec4 = {
      0x02U,               /// OpCode Block.
      0x40U,               /// Block type.
      0x45U, 0x46U, 0x47U, /// Valid OpCodes.
      0xEDU, 0xEEU, 0xEFU, /// Invalid OpCodes.
      0x0BU,               /// OpCode End.
      0x0BU                /// Expression End.
  };
  Mgr.setCode(Vec4);
  SSVM::AST::Expression Exp4;
  EXPECT_FALSE(Exp4.loadBinary(Mgr, PConf));

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec5 = {
      0x03U,               /// OpCode Loop.
      0x40U,               /// Block type.
      0x45U, 0x46U, 0x47U, /// Valid OpCodes.
      0xEDU, 0xEEU, 0xEFU, /// Invalid OpCodes.
      0x0BU,               /// OpCode End.
      0x0BU                /// Expression End.
  };
  Mgr.setCode(Vec5);
  SSVM::AST::Expression Exp5;
  EXPECT_FALSE(Exp5.loadBinary(Mgr, PConf));

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec6 = {
      0x02U,               /// OpCode Block.
      0x40U,               /// Block type.
      0x45U, 0x46U, 0x47U, /// Valid OpCodes.
      0x0BU,               /// OpCode End.
      0x0BU                /// Expression End.
  };
  Mgr.setCode(Vec6);
  SSVM::AST::Expression Exp6;
  EXPECT_TRUE(Exp6.loadBinary(Mgr, PConf) && Mgr.getRemainSize() == 0);

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec7 = {
      0x03U,               /// OpCode Loop.
      0x40U,               /// Block type.
      0x45U, 0x46U, 0x47U, /// Valid OpCodes.
      0x0BU,               /// OpCode End.
      0x0BU                /// Expression End.
  };
  Mgr.setCode(Vec7);
  SSVM::AST::Expression Exp7;
  EXPECT_TRUE(Exp7.loadBinary(Mgr, PConf) && Mgr.getRemainSize() == 0);
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

  Mgr.clearBuffer();
  SSVM::AST::Expression Exp1;
  EXPECT_FALSE(Exp1.loadBinary(Mgr, PConf));

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec2 = {
      0x04U, /// OpCode If.
      0x40U, /// Block type.
      0x0BU, /// OpCode End.
      0x0BU  /// Expression End.
  };
  Mgr.setCode(Vec2);
  SSVM::AST::Expression Exp2;
  EXPECT_TRUE(Exp2.loadBinary(Mgr, PConf) && Mgr.getRemainSize() == 0);

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec3 = {
      0x04U, /// OpCode If.
      0x40U, /// Block type.
      0x05U, /// OpCode Else
      0x0BU, /// OpCode End.
      0x0BU  /// Expression End.
  };
  Mgr.setCode(Vec3);
  SSVM::AST::Expression Exp3;
  EXPECT_TRUE(Exp3.loadBinary(Mgr, PConf) && Mgr.getRemainSize() == 0);

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec4 = {
      0x04U,               /// OpCode If.
      0x40U,               /// Block type.
      0xEDU, 0xEEU, 0xEFU, /// Invalid OpCodes in if statement.
      0x0BU,               /// OpCode End.
      0x0BU                /// Expression End.
  };
  Mgr.setCode(Vec4);
  SSVM::AST::Expression Exp4;
  EXPECT_FALSE(Exp4.loadBinary(Mgr, PConf));

  Mgr.clearBuffer();
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
  SSVM::AST::Expression Exp5;
  EXPECT_FALSE(Exp5.loadBinary(Mgr, PConf));

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec6 = {
      0x04U,               /// OpCode If.
      0x40U,               /// Block type.
      0x45U, 0x46U, 0x47U, /// Valid OpCodes in if statement.
      0x0BU,               /// OpCode End.
      0x0BU                /// Expression End.
  };
  Mgr.setCode(Vec6);
  SSVM::AST::Expression Exp6;
  EXPECT_TRUE(Exp6.loadBinary(Mgr, PConf) && Mgr.getRemainSize() == 0);

  Mgr.clearBuffer();
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
  SSVM::AST::Expression Exp7;
  EXPECT_TRUE(Exp7.loadBinary(Mgr, PConf) && Mgr.getRemainSize() == 0);
}

TEST(InstructionTest, LoadBrControlInstruction) {
  /// 3. Test branch control instruction.
  ///
  ///   1.  Load invalid empty label index.
  ///   2.  Load valid label index.
  SSVM::OpCode Op1 = SSVM::OpCode::Br;
  SSVM::OpCode Op2 = SSVM::OpCode::Br_if;

  Mgr.clearBuffer();
  SSVM::AST::Instruction Ins1(Op1);
  EXPECT_FALSE(Ins1.loadBinary(Mgr, PConf));
  Mgr.clearBuffer();
  SSVM::AST::Instruction Ins2(Op2);
  EXPECT_FALSE(Ins2.loadBinary(Mgr, PConf));

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec2 = {
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU /// Label index.
  };
  Mgr.setCode(Vec2);
  SSVM::AST::Instruction Ins3(Op1);
  EXPECT_TRUE(Ins3.loadBinary(Mgr, PConf) && Mgr.getRemainSize() == 0);
  Mgr.clearBuffer();
  Mgr.setCode(Vec2);
  SSVM::AST::Instruction Ins4(Op2);
  EXPECT_TRUE(Ins4.loadBinary(Mgr, PConf) && Mgr.getRemainSize() == 0);
}

TEST(InstructionTest, LoadBrTableControlInstruction) {
  /// 4. Test branch table control instruction.
  ///
  ///   1.  Load invalid empty instruction body.
  ///   2.  Load instruction with empty label vector.
  ///   3.  Load instruction with label vector.
  SSVM::OpCode Op = SSVM::OpCode::Br_table;

  Mgr.clearBuffer();
  SSVM::AST::Instruction Ins1(Op);
  EXPECT_FALSE(Ins1.loadBinary(Mgr, PConf));

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec2 = {
      0x00U,                            /// Vector length = 0
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU /// Label index.
  };
  Mgr.setCode(Vec2);
  SSVM::AST::Instruction Ins2(Op);
  EXPECT_TRUE(Ins2.loadBinary(Mgr, PConf) && Mgr.getRemainSize() == 0);

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec3 = {
      0x03U,                             /// Vector length = 3
      0xF1U, 0xFFU, 0xFFU, 0xFFU, 0x0FU, /// vec[0]
      0xF2U, 0xFFU, 0xFFU, 0xFFU, 0x0FU, /// vec[1]
      0xF3U, 0xFFU, 0xFFU, 0xFFU, 0x0FU, /// vec[2]
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU  /// Label index.
  };
  Mgr.setCode(Vec3);
  SSVM::AST::Instruction Ins3(Op);
  EXPECT_TRUE(Ins3.loadBinary(Mgr, PConf) && Mgr.getRemainSize() == 0);
}

TEST(InstructionTest, LoadCallControlInstruction) {
  /// 5. Test call control instruction.
  ///
  ///   1.  Load invalid empty instruction body.
  ///   2.  Load valid type index.
  ///   3.  Load valid function index.
  SSVM::OpCode Op1 = SSVM::OpCode::Call;
  SSVM::OpCode Op2 = SSVM::OpCode::Call_indirect;

  Mgr.clearBuffer();
  SSVM::AST::Instruction Ins1(Op1);
  EXPECT_FALSE(Ins1.loadBinary(Mgr, PConf));
  Mgr.clearBuffer();
  SSVM::AST::Instruction Ins2(Op2);
  EXPECT_FALSE(Ins2.loadBinary(Mgr, PConf));

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec2 = {
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU /// Function index.
  };
  Mgr.setCode(Vec2);
  SSVM::AST::Instruction Ins3(Op1);
  EXPECT_TRUE(Ins3.loadBinary(Mgr, PConf) && Mgr.getRemainSize() == 0);

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec3 = {
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU, /// Type index.
      0x00U                              /// 0x00 for ending
  };
  Mgr.setCode(Vec3);
  SSVM::AST::Instruction Ins4(Op2);
  EXPECT_TRUE(Ins4.loadBinary(Mgr, PConf) && Mgr.getRemainSize() == 0);
}

TEST(InstructionTest, LoadVariableInstruction) {
  /// 6. Test variable instruction.
  ///
  ///   1.  Load invalid empty local or global index.
  ///   2.  Load valid empty local or global index.
  SSVM::OpCode Op = SSVM::OpCode::Local__get;

  Mgr.clearBuffer();
  SSVM::AST::Instruction Ins1(Op);
  EXPECT_FALSE(Ins1.loadBinary(Mgr, PConf));

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec2 = {
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU /// Local index.
  };
  Mgr.setCode(Vec2);
  SSVM::AST::Instruction Ins2(Op);
  EXPECT_TRUE(Ins2.loadBinary(Mgr, PConf) && Mgr.getRemainSize() == 0);
}

TEST(InstructionTest, LoadMemoryInstruction) {
  /// 7. Test memory instruction.
  ///
  ///   1.  Load invalid empty memory args.
  ///   2.  Load invalid memory size or grow instruction.
  ///   3.  Load valid memory args.
  ///   4.  Load valid memory size instruction.
  SSVM::OpCode Op1 = SSVM::OpCode::I32__load;
  SSVM::OpCode Op2 = SSVM::OpCode::Memory__grow;

  Mgr.clearBuffer();
  SSVM::AST::Instruction Ins1(Op1);
  EXPECT_FALSE(Ins1.loadBinary(Mgr, PConf));
  Mgr.clearBuffer();
  SSVM::AST::Instruction Ins2(Op2);
  EXPECT_FALSE(Ins2.loadBinary(Mgr, PConf));

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec2 = {
      0xFFU /// Invalid memory size instruction content.
  };
  Mgr.setCode(Vec2);
  SSVM::AST::Instruction Ins3(Op2);
  EXPECT_FALSE(Ins3.loadBinary(Mgr, PConf));

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec3 = {
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU, /// Align.
      0xFEU, 0xFFU, 0xFFU, 0xFFU, 0x0FU  /// Offset.
  };
  Mgr.setCode(Vec3);
  SSVM::AST::Instruction Ins4(Op1);
  EXPECT_TRUE(Ins4.loadBinary(Mgr, PConf) && Mgr.getRemainSize() == 0);

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec4 = {
      0x00U /// Memory size instruction content.
  };
  Mgr.setCode(Vec4);
  SSVM::AST::Instruction Ins5(Op2);
  EXPECT_TRUE(Ins5.loadBinary(Mgr, PConf) && Mgr.getRemainSize() == 0);
}

TEST(InstructionTest, LoadConstInstruction) {
  /// 8. Test const numeric instructions.
  ///
  ///   1.  Load invalid empty const numeric instruction.
  ///   2.  Load I32 const numeric instruction.
  ///   3.  Load I64 const numeric instruction.
  ///   4.  Load F32 const numeric instruction.
  ///   5.  Load F64 const numeric instruction.
  SSVM::OpCode Op1 = SSVM::OpCode::I32__const;
  SSVM::OpCode Op2 = SSVM::OpCode::I64__const;
  SSVM::OpCode Op3 = SSVM::OpCode::F32__const;
  SSVM::OpCode Op4 = SSVM::OpCode::F64__const;

  Mgr.clearBuffer();
  SSVM::AST::Instruction Ins1(Op1);
  EXPECT_FALSE(Ins1.loadBinary(Mgr, PConf));

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec2 = {
      0xC0U, 0xBBU, 0x78U /// I32 -123456.
  };
  Mgr.setCode(Vec2);
  SSVM::AST::Instruction Ins2(Op1);
  EXPECT_TRUE(Ins2.loadBinary(Mgr, PConf) && Mgr.getRemainSize() == 0);

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec3 = {
      0xC2U, 0x8EU, 0xF6U, 0xF2U, 0xDDU, 0x7CU /// I64 -112233445566
  };
  Mgr.setCode(Vec3);
  SSVM::AST::Instruction Ins3(Op2);
  EXPECT_TRUE(Ins3.loadBinary(Mgr, PConf) && Mgr.getRemainSize() == 0);

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec4 = {
      0xDA, 0x0F, 0x49, 0xC0 /// F32 -3.1415926
  };
  Mgr.setCode(Vec4);
  SSVM::AST::Instruction Ins4(Op3);
  EXPECT_TRUE(Ins4.loadBinary(Mgr, PConf) && Mgr.getRemainSize() == 0);

  Mgr.clearBuffer();
  std::vector<unsigned char> Vec5 = {
      0x18, 0x2D, 0x44, 0x54, 0xFB, 0x21, 0x09, 0xC0 /// F64 -3.1415926535897932
  };
  Mgr.setCode(Vec5);
  SSVM::AST::Instruction Ins5(Op4);
  EXPECT_TRUE(Ins5.loadBinary(Mgr, PConf) && Mgr.getRemainSize() == 0);
}

} // namespace
