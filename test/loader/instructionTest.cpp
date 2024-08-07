// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/test/loader/instructionTest.cpp - Instruction unit tests -===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contents unit tests of loading Instruction nodes.
///
//===----------------------------------------------------------------------===//

#include "loader/loader.h"

#include <cstdint>
#include <gtest/gtest.h>
#include <vector>

namespace {

WasmEdge::Configure Conf;
WasmEdge::Loader::Loader Ldr(Conf);
std::vector<uint8_t> prefixedVec(const std::vector<uint8_t> &Vec) {
  std::vector<uint8_t> PrefixVec = {
      0x00U, 0x61U, 0x73U, 0x6DU, // Magic
      0x01U, 0x00U, 0x00U, 0x00U, // Version
      0x03U,                      // Function section
      0x02U,                      // Content size = 2
      0x01U,                      // Vector length = 1
      0x00U,                      // vec[0]
  };
  PrefixVec.reserve(PrefixVec.size() + Vec.size());
  PrefixVec.insert(PrefixVec.end(), Vec.begin(), Vec.end());
  return PrefixVec;
}

TEST(InstructionTest, LoadBlockControlInstruction) {
  std::vector<uint8_t> Vec;

  // 1. Test block control instructions.
  //
  //   1.  Load block with only end operation.
  //   2.  Load loop with only end operation.
  //   3.  Load block with invalid operations.
  //   4.  Load loop with invalid operations.
  //   5.  Load block with instructions.
  //   6.  Load loop with instructions.

  Vec = {
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
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
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
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU,               // Code section
      0x0DU,               // Content size = 13
      0x01U,               // Vector length = 1
      0x0BU,               // Code segment size = 11
      0x00U,               // Local vec(0)
      0x02U,               // OpCode Block.
      0x40U,               // Block type.
      0x45U, 0x46U, 0x47U, // Valid OpCodes.
      0xEDU, 0xEEU, 0xEFU, // Invalid OpCodes.
      0x0BU,               // OpCode End.
      0x0BU                // Expression End.
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU,               // Code section
      0x0DU,               // Content size = 13
      0x01U,               // Vector length = 1
      0x0BU,               // Code segment size = 11
      0x00U,               // Local vec(0)
      0x03U,               // OpCode Loop.
      0x40U,               // Block type.
      0x45U, 0x46U, 0x47U, // Valid OpCodes.
      0xEDU, 0xEEU, 0xEFU, // Invalid OpCodes.
      0x0BU,               // OpCode End.
      0x0BU                // Expression End.
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
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
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
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
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));
}

TEST(InstructionTest, LoadIfElseControlInstruction) {
  std::vector<uint8_t> Vec;

  // 2. Test load if-else control instruction.
  //
  //   1.  Load invalid empty-body if statement.
  //   2.  Load if statement with only end operation.
  //   3.  Load if and else statements with only end operation.
  //   4.  Load if statement with invalid operations.
  //   5.  Load if and else statements with invalid operations.
  //   6.  Load if statement with instructions.
  //   7.  Load if and else statements with instructions.
  //   8.  Load invalid else instruction out of block.
  //   9.  Load invalid else instruction out of if statement.
  //   10. Load invalid else instruction duplicated in if statement.

  Vec = {
      0x0AU, // Code section
      0x04U, // Content size = 4
      0x01U, // Vector length = 1
      0x02U, // Code segment size = 2
      0x00U, // Local vec(0)
      0x04U  // OpCode If.
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
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
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
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
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU,               // Code section
      0x0AU,               // Content size = 10
      0x01U,               // Vector length = 1
      0x08U,               // Code segment size = 8
      0x00U,               // Local vec(0)
      0x04U,               // OpCode If.
      0x40U,               // Block type.
      0xEDU, 0xEEU, 0xEFU, // Invalid OpCodes in if statement.
      0x0BU,               // OpCode End.
      0x0BU                // Expression End.
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU,               // Code section
      0x0EU,               // Content size = 14
      0x01U,               // Vector length = 1
      0x0CU,               // Code segment size = 12
      0x00U,               // Local vec(0)
      0x04U,               // OpCode If.
      0x40U,               // Block type.
      0x45U, 0x46U, 0x47U, // Valid OpCodes in if statement.
      0x05U,               // OpCode Else
      0xEDU, 0xEEU, 0xEFU, // Invalid OpCodes in else statement.
      0x0BU,               // OpCode End.
      0x0BU                // Expression End.
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
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
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
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
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU,               // Code section
      0x0BU,               // Content size = 11
      0x01U,               // Vector length = 1
      0x09U,               // Code segment size = 9
      0x00U,               // Local vec(0)
      0x45U, 0x46U, 0x47U, // Valid OpCodes.
      0x05U,               // OpCode Else.
      0x45U, 0x46U, 0x47U, // Valid OpCodes.
      0x0BU                // Expression End.
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU,               // Code section
      0x0EU,               // Content size = 14
      0x01U,               // Vector length = 1
      0x0CU,               // Code segment size = 12
      0x00U,               // Local vec(0)
      0x02U,               // OpCode Block.
      0x40U,               // Block type.
      0x45U, 0x46U, 0x47U, // Valid OpCodes.
      0x05U,               // OpCode Else.
      0x45U, 0x46U, 0x47U, // Valid OpCodes.
      0x0BU,               // OpCode End.
      0x0BU                // Expression End.
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU,               // Code section
      0x0FU,               // Content size = 15
      0x01U,               // Vector length = 1
      0x0DU,               // Code segment size = 13
      0x00U,               // Local vec(0)
      0x04U,               // OpCode If.
      0x40U,               // Block type.
      0x45U, 0x46U, 0x47U, // Valid OpCodes in if statement.
      0x05U,               // OpCode Else
      0x05U,               // Duplicated OpCode Else
      0x45U, 0x46U, 0x47U, // Valid OpCodes in else statement.
      0x0BU,               // OpCode End.
      0x0BU                // Expression End.
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));
}

TEST(InstructionTest, LoadBrControlInstruction) {
  std::vector<uint8_t> Vec;

  // 3. Test branch control instructions.
  //
  //   1.  Load invalid empty label index.
  //   2.  Load valid label index.

  Vec = {
      0x0AU, // Code section
      0x04U, // Content size = 4
      0x01U, // Vector length = 1
      0x02U, // Code segment size = 2
      0x00U, // Local vec(0)
      0x0CU  // OpCode Br.
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));
  Vec[5] = 0x0DU; // OpCode Br_if.
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU,                             // Code section
      0x0AU,                             // Content size = 10
      0x01U,                             // Vector length = 1
      0x08U,                             // Code segment size = 8
      0x00U,                             // Local vec(0)
      0x0CU,                             // OpCode Br.
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU, // Label index.
      0x0BU                              // Expression End.
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));
  Vec[5] = 0x0DU; // OpCode Br_if.
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));
}

TEST(InstructionTest, LoadBrTableControlInstruction) {
  std::vector<uint8_t> Vec;

  // 4. Test branch table control instruction.
  //
  //   1.  Load invalid empty instruction body.
  //   2.  Load instruction with empty label vector.
  //   3.  Load instruction with label vector.
  //   4.  Load instruction with wrong length of label vector.

  Vec = {
      0x0AU, // Code section
      0x04U, // Content size = 4
      0x01U, // Vector length = 1
      0x02U, // Code segment size = 2
      0x00U, // Local vec(0)
      0x0EU  // OpCode Br_table.
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
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
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
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
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU, // Code section
      0x07U, // Content size = 7
      0x01U, // Vector length = 1
      0x05U, // Code segment size = 5
      0x00U, // Local vec(0)
      0x0EU, // OpCode Br_table.
      0x03U, // Vector length = 3
      0x01U, // vec[0]
      0x02U  // vec[1]
             // Missed vec[2] and label index
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));
}

TEST(InstructionTest, LoadCallControlInstruction) {
  std::vector<uint8_t> Vec;

  Conf.removeProposal(WasmEdge::Proposal::ReferenceTypes);
  WasmEdge::Loader::Loader LdrNoRefType(Conf);
  Conf.addProposal(WasmEdge::Proposal::ReferenceTypes);

  // 5. Test call control instructions.
  //
  //   1.  Load invalid empty call or call_indirect instruction body.
  //   2.  Load call instruction with valid type index.
  //   3.  Load call_indirect instruction with valid type and table index.
  //   4.  Load call_indirect instruction with unexpected end of table index.
  //   5.  Load call_indirect instruction with invalid table index without
  //       Ref-Types proposal.

  Vec = {
      0x0AU, // Code section
      0x04U, // Content size = 4
      0x01U, // Vector length = 1
      0x02U, // Code segment size = 2
      0x00U, // Local vec(0)
      0x10U  // OpCode Call.
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));
  Vec[5] = 0x11U; // OpCode Call_indirect.
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU,                             // Code section
      0x0AU,                             // Content size = 10
      0x01U,                             // Vector length = 1
      0x08U,                             // Code segment size = 8
      0x00U,                             // Local vec(0)
      0x10U,                             // OpCode Call.
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU, // Function type index.
      0x0BU                              // Expression End.
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
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
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU, // Code section
      0x09U, // Content size = 9
      0x01U, // Vector length = 1
      0x07U, // Code segment size = 7
      0x00U, // Local vec(0)
      0x11U, // OpCode Call_indirect.
      0xFFU, 0xFFU, 0xFFU,
      0xFFU, 0x0FU // Type index.
                   // 0x00U  // Missed table index.
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
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
  EXPECT_FALSE(LdrNoRefType.parseModule(prefixedVec(Vec)));
}

TEST(InstructionTest, LoadReferenceInstruction) {
  std::vector<uint8_t> Vec;

  Conf.removeProposal(WasmEdge::Proposal::ReferenceTypes);
  WasmEdge::Loader::Loader LdrNoRefType(Conf);
  Conf.addProposal(WasmEdge::Proposal::ReferenceTypes);

  // 6. Test reference instructions.
  //
  //   1.  Load invalid empty reference type.
  //   2.  Load invalid reference type without Ref-Types proposal.

  Vec = {
      0x0AU, // Code section
      0x04U, // Content size = 4
      0x01U, // Vector length = 1
      0x02U, // Code segment size = 2
      0x00U, // Local vec(0)
      0xD0U  // OpCode Ref__null.
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU, // Code section
      0x06U, // Content size = 6
      0x01U, // Vector length = 1
      0x04U, // Code segment size = 4
      0x00U, // Local vec(0)
      0xD0U, // OpCode Ref__null.
      0x6FU, // ExternRef
      0x0BU  // Expression End.
  };
  EXPECT_FALSE(LdrNoRefType.parseModule(prefixedVec(Vec)));
}

TEST(InstructionTest, LoadParametricInstruction) {
  std::vector<uint8_t> Vec;

  Conf.removeProposal(WasmEdge::Proposal::SIMD);
  WasmEdge::Loader::Loader LdrNoSIMD(Conf);
  Conf.addProposal(WasmEdge::Proposal::SIMD);

  // 7. Test parametric instructions.
  //
  //   1.  Load valid select_t instruction with value type list.
  //   2.  Load invalid empty value type list.
  //   3.  Load invalid unexpected end of value type list.
  //   4.  Load invalid value type list without SIMD proposal.

  Vec = {
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
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU, // Code section
      0x04U, // Content size = 4
      0x01U, // Vector length = 1
      0x02U, // Code segment size = 2
      0x00U, // Local vec(0)
      0x1CU  // OpCode Select_t.
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU,       // Code section
      0x07U,       // Content size = 7
      0x01U,       // Vector length = 1
      0x05U,       // Code segment size = 5
      0x00U,       // Local vec(0)
      0x1CU,       // OpCode Select_t.
      0x03U,       // Vector length = 3
      0x7FU, 0x7EU // Value types list only in 2
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU,        // Code section
      0x08U,        // Content size = 8
      0x01U,        // Vector length = 1
      0x06U,        // Code segment size = 6
      0x00U,        // Local vec(0)
      0x1CU,        // OpCode Select_t.
      0x02U,        // Vector length = 2
      0x7BU, 0x7BU, // Value types with v128
      0x0BU         // Expression End.
  };
  EXPECT_FALSE(LdrNoSIMD.parseModule(prefixedVec(Vec)));
}

TEST(InstructionTest, LoadVariableInstruction) {
  std::vector<uint8_t> Vec;

  // 8. Test variable instructions.
  //
  //   1.  Load invalid empty local or global index.
  //   2.  Load valid local or global index.

  Vec = {
      0x0AU, // Code section
      0x04U, // Content size = 4
      0x01U, // Vector length = 1
      0x02U, // Code segment size = 2
      0x00U, // Local vec(0)
      0x20U  // OpCode Local__get.
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU,                             // Code section
      0x0AU,                             // Content size = 10
      0x01U,                             // Vector length = 1
      0x08U,                             // Code segment size = 8
      0x00U,                             // Local vec(0)
      0x20U,                             // OpCode Local__get.
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU, // Local index.
      0x0BU                              // Expression End.
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));
}

TEST(InstructionTest, LoadTableInstruction) {
  std::vector<uint8_t> Vec;

  // 9. Test table instructions.
  //
  //   1.  Load table_get instruction with unexpected end of table index.
  //   2.  Load table_init instruction with unexpected end of table index.
  //   3.  Load table_copy instruction with unexpected end of destination index.

  Vec = {
      0x0AU, // Code section
      0x04U, // Content size = 4
      0x01U, // Vector length = 1
      0x02U, // Code segment size = 2
      0x00U, // Local vec(0)
      0x25U  // OpCode Table__get.
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU,       // Code section
      0x05U,       // Content size = 5
      0x01U,       // Vector length = 1
      0x03U,       // Code segment size = 3
      0x00U,       // Local vec(0)
      0xFCU, 0x0CU // OpCode Table__init.
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU,       // Code section
      0x05U,       // Content size = 5
      0x01U,       // Vector length = 1
      0x03U,       // Code segment size = 3
      0x00U,       // Local vec(0)
      0xFCU, 0x0EU // OpCode Table__copy.
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));
}

TEST(InstructionTest, LoadMemoryInstruction) {
  std::vector<uint8_t> Vec;

  Conf.removeProposal(WasmEdge::Proposal::MultiMemories);
  WasmEdge::Loader::Loader LdrMultiMem(Conf);
  Conf.addProposal(WasmEdge::Proposal::MultiMemories);

  // 10. Test memory instructions.
  //
  //   1.  Load invalid empty memory args.
  //   2.  Load memory_grow instruction with invalid empty checking byte.
  //   3.  Load memory_grow instruction with invalid checking byte.
  //   4.  Load valid memory args.
  //   5.  Load memory_grow instruction with valid checking byte.
  //   6.  Load memory_copy instruction with invalid checking byte.
  //   7.  Load memory_init instruction with unexpected end of data index.
  //   8.  Load memory_copy instruction with unexpected end of source index with
  //       multi-memories proposal.
  //   9.  Load invalid memory index with multi-memories proposal.

  Vec = {
      0x0AU, // Code section
      0x04U, // Content size = 4
      0x01U, // Vector length = 1
      0x02U, // Code segment size = 2
      0x00U, // Local vec(0)
      0x28U  // OpCode I32__load.
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU, // Code section
      0x04U, // Content size = 4
      0x01U, // Vector length = 1
      0x02U, // Code segment size = 2
      0x00U, // Local vec(0)
      0x40U  // OpCode Memory__grow.
             // 0x00  // Missed checking byte
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU, // Code section
      0x05U, // Content size = 5
      0x01U, // Vector length = 1
      0x03U, // Code segment size = 3
      0x00U, // Local vec(0)
      0x40U, // OpCode Memory__grow.
      0xFFU  // Invalid checking byte.
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU,                             // Code section
      0x0FU,                             // Content size = 15
      0x01U,                             // Vector length = 1
      0x0DU,                             // Code segment size = 13
      0x00U,                             // Local vec(0)
      0x28U,                             // OpCode I32__load.
      0x8FU, 0x80U, 0x80U, 0x80U, 0x00U, // Align.
      0xFEU, 0xFFU, 0xFFU, 0xFFU, 0x0FU, // Offset.
      0x0BU                              // Expression End.
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU, // Code section
      0x06U, // Content size = 6
      0x01U, // Vector length = 1
      0x04U, // Code segment size = 4
      0x00U, // Local vec(0)
      0x40U, // OpCode Memory__grow.
      0x00U, // Valid checking byte.
      0x0BU  // Expression End.
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU,       // Code section
      0x07U,       // Content size = 7
      0x01U,       // Vector length = 1
      0x05U,       // Code segment size = 5
      0x00U,       // Local vec(0)
      0xFCU, 0x0A, // OpCode Memory__copy.
      0x44U,       // Invalid checking byte 1.
      0x00U        // Valid checking byte 2.
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0CU, // Datacount section
      0x01U, // Content size = 1
      0x01U, // Content
      0x0AU, // Code section
      0x05U, // Content size = 5
      0x01U, // Vector length = 1
      0x03U, // Code segment size = 3
      0x00U, // Local vec(0)
      0xFCU,
      0x08U // OpCode Memory__init.
            // 0x00  // Missed data index
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU, // Code section
      0x05U, // Content size = 5
      0x01U, // Vector length = 1
      0x03U, // Code segment size = 3
      0x00U, // Local vec(0)
      0xFCU,
      0x0AU // OpCode Memory__copy.
            // 0x01U, 0x02U  // Missed source and target index
  };
  EXPECT_FALSE(LdrMultiMem.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU, // Code section
      0x04U, // Content size = 4
      0x01U, // Vector length = 1
      0x02U, // Code segment size = 2
      0x00U, // Local vec(0)
      0x28U, // OpCode I32__load.
      0x40U  // Align specifies memory index.
             // 0x01U  // Missed memory index
  };
  EXPECT_FALSE(LdrMultiMem.parseModule(prefixedVec(Vec)));
}

TEST(InstructionTest, LoadConstInstruction) {
  std::vector<uint8_t> Vec;

  // 11. Test const numeric instructions.
  //
  //   1.  Load I32 const numeric instruction.
  //   2.  Load I64 const numeric instruction.
  //   3.  Load F32 const numeric instruction.
  //   4.  Load F64 const numeric instruction.
  //   5.  Load invalid unexpected end of F32 const numeric instruction.
  //   6.  Load invalid unexpected end of F64 const numeric instruction.

  Vec = {
      0x0AU,               // Code section
      0x08U,               // Content size = 8
      0x01U,               // Vector length = 1
      0x06U,               // Code segment size = 6
      0x00U,               // Local vec(0)
      0x41U,               // OpCode I32__const.
      0xC0U, 0xBBU, 0x78U, // I32 -123456.
      0x0BU                // Expression End.
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU,                                    // Code section
      0x0BU,                                    // Content size = 11
      0x01U,                                    // Vector length = 1
      0x09U,                                    // Code segment size = 9
      0x00U,                                    // Local vec(0)
      0x42U,                                    // OpCode I64__const.
      0xC2U, 0x8EU, 0xF6U, 0xF2U, 0xDDU, 0x7CU, // I64 -112233445566
      0x0BU                                     // Expression End.
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU,                      // Code section
      0x09U,                      // Content size = 9
      0x01U,                      // Vector length = 1
      0x07U,                      // Code segment size = 7
      0x00U,                      // Local vec(0)
      0x43U,                      // OpCode F32__const.
      0xDAU, 0x0FU, 0x49U, 0xC0U, // F32 -3.1415926
      0x0BU                       // Expression End.
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
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
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU, // Code section
      0x06U, // Content size = 6
      0x01U, // Vector length = 1
      0x04U, // Code segment size = 4
      0x00U, // Local vec(0)
      0x43U, // OpCode F32__const.
      0xDAU,
      0x0FU // F32 -3.1415926
            // 0x49U, 0xC0U  // Missed 2 bytes
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU, // Code section
      0x09U, // Content size = 9
      0x01U, // Vector length = 1
      0x07U, // Code segment size = 7
      0x00U, // Local vec(0)
      0x44U, // OpCode F64__const.
      0x18U, 0x2DU, 0x44U,
      0x54U, 0xFBU // F64 -3.1415926535897932
                   // 0x21U, 0x09U, 0xC0U  // Missed 3 bytes
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));
}

TEST(InstructionTest, Proposals) {
  std::vector<uint8_t> Vec;

  Conf.removeProposal(WasmEdge::Proposal::SIMD);
  WasmEdge::Loader::Loader LdrNoSIMD(Conf);
  Conf.addProposal(WasmEdge::Proposal::SIMD);

  Conf.removeProposal(WasmEdge::Proposal::BulkMemoryOperations);
  Conf.removeProposal(WasmEdge::Proposal::ReferenceTypes);
  WasmEdge::Loader::Loader LdrNoRefType(Conf);
  Conf.addProposal(WasmEdge::Proposal::BulkMemoryOperations);
  Conf.addProposal(WasmEdge::Proposal::ReferenceTypes);

  Conf.removeProposal(WasmEdge::Proposal::MultiValue);
  WasmEdge::Loader::Loader LdrNoMultiVal(Conf);
  Conf.addProposal(WasmEdge::Proposal::MultiValue);

  Conf.removeProposal(WasmEdge::Proposal::NonTrapFloatToIntConversions);
  WasmEdge::Loader::Loader LdrNoTrapConv(Conf);
  Conf.addProposal(WasmEdge::Proposal::NonTrapFloatToIntConversions);

  Conf.removeProposal(WasmEdge::Proposal::SignExtensionOperators);
  WasmEdge::Loader::Loader LdrNoSignExt(Conf);
  Conf.addProposal(WasmEdge::Proposal::SignExtensionOperators);

  Conf.addProposal(WasmEdge::Proposal::Threads);
  WasmEdge::Loader::Loader LdrThreads(Conf);
  Conf.removeProposal(WasmEdge::Proposal::Threads);

  Conf.addProposal(WasmEdge::Proposal::TailCall);
  WasmEdge::Loader::Loader LdrTailCall(Conf);
  Conf.removeProposal(WasmEdge::Proposal::TailCall);

  Conf.addProposal(WasmEdge::Proposal::FunctionReferences);
  WasmEdge::Loader::Loader LdrFuncRef(Conf);
  Conf.removeProposal(WasmEdge::Proposal::FunctionReferences);

  Conf.addProposal(WasmEdge::Proposal::TailCall);
  Conf.addProposal(WasmEdge::Proposal::FunctionReferences);
  WasmEdge::Loader::Loader LdrFuncRefAndTailCall(Conf);
  Conf.removeProposal(WasmEdge::Proposal::TailCall);
  Conf.removeProposal(WasmEdge::Proposal::FunctionReferences);

  // 12. Test ValTypes and instructions with disabled proposals
  //
  //   1.  Load if instruction with/without SIMD proposal.
  //   2.  Load if instruction with reference instructions with/without
  //       Ref-Types and Bulk-Mem proposals.
  //   3.  Load select_t instruction with/without SIMD proposal.
  //   4.  Load select_t instruction with reference instructions with/without
  //       Ref-Types and Bulk-Mem proposals.
  //   5.  Load if instruction with BlockType as result type with/without
  //       Multi-Value proposal.
  //   6.  Load saturating truncation instructions with/without NonTrap-Conv
  //       proposal.
  //   7.  Load sign extension instructions with/without Sign-Ext proposal.
  //   8.  Load atomic instructions with/without threads proposal.
  //   9.  Load return_call instructions with/without tail-call proposal.
  //   10. Load reference instructions with/without typed function reference
  //       proposal.
  //   11. Load Return_call_ref instruction with/without tail-call proposal.

  Vec = {
      0x0AU,                      // Code section
      0x2CU,                      // Content size = 44
      0x01U,                      // Vector length = 1
      0x2AU,                      // Code segment size = 42
      0x00U,                      // Local vec(0)
      0x04U,                      // OpCode If.
      0x7BU,                      // Block type V128.
      0xFDU, 0x0CU,               // OpCode V128__const.
      0x01U, 0x00U, 0x00U, 0x00U, // 1.
      0x00U, 0x00U, 0x00U, 0x00U, // 0.
      0x00U, 0x00U, 0x00U, 0x00U, // 0.
      0x00U, 0x00U, 0x00U, 0x00U, // 0.
      0x05U,                      // OpCode Else.
      0xFDU, 0x0CU,               // OpCode V128__const.
      0x02U, 0x00U, 0x00U, 0x00U, // 2.
      0x00U, 0x00U, 0x00U, 0x00U, // 0.
      0x00U, 0x00U, 0x00U, 0x00U, // 0.
      0x00U, 0x00U, 0x00U, 0x00U, // 0.
      0x0BU,                      // OpCode End.
      0x0BU                       // Expression End.
  };
  EXPECT_FALSE(LdrNoSIMD.parseModule(prefixedVec(Vec)));
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU,        // Code section
      0x0CU,        // Content size = 12
      0x01U,        // Vector length = 1
      0x0AU,        // Code segment size = 10
      0x00U,        // Local vec(0)
      0x04U,        // OpCode If.
      0x70U,        // Block type FuncRef.
      0xD0U, 0x70U, // OpCode Ref__null func.
      0x05U,        // OpCode Else.
      0xD0U, 0x70U, // OpCode Ref__null func.
      0x0BU,        // OpCode End.
      0x0BU         // Expression End.
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));
  EXPECT_FALSE(LdrNoRefType.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU,                      // Code section
      0x2DU,                      // Content size = 45
      0x01U,                      // Vector length = 1
      0x2BU,                      // Code segment size = 43
      0x00U,                      // Local vec(0)
      0xFDU, 0x0CU,               // OpCode V128__const.
      0x01U, 0x00U, 0x00U, 0x00U, // 1.
      0x00U, 0x00U, 0x00U, 0x00U, // 0.
      0x00U, 0x00U, 0x00U, 0x00U, // 0.
      0x00U, 0x00U, 0x00U, 0x00U, // 0.
      0xFDU, 0x0CU,               // OpCode V128__const.
      0x02U, 0x00U, 0x00U, 0x00U, // 2.
      0x00U, 0x00U, 0x00U, 0x00U, // 0.
      0x00U, 0x00U, 0x00U, 0x00U, // 0.
      0x00U, 0x00U, 0x00U, 0x00U, // 0.
      0x41U, 0x01U,               // OpCode I32__const 1.
      0x1CU,                      // OpCode Select_t.
      0x01U, 0x7BU,               // Select type V128.
      0x0BU                       // Expression End.
  };
  EXPECT_FALSE(LdrNoSIMD.parseModule(prefixedVec(Vec)));
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU,        // Code section
      0x0DU,        // Content size = 13
      0x01U,        // Vector length = 1
      0x0BU,        // Code segment size = 11
      0x00U,        // Local vec(0)
      0xD0U, 0x70U, // OpCode Ref__null func.
      0xD0U, 0x70U, // OpCode Ref__null func.
      0x41U, 0x01U, // OpCode I32__const 1.
      0x1CU,        // OpCode Select_t.
      0x01U, 0x70U, // Select type FuncRef.
      0x0BU,        // Expression End.
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));
  EXPECT_FALSE(LdrNoRefType.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU,        // Code section
      0x0CU,        // Content size = 12
      0x01U,        // Vector length = 1
      0x0AU,        // Code segment size = 10
      0x00U,        // Local vec(0)
      0x04U,        // OpCode If.
      0x01U,        // Block type function index 1.
      0xD0U, 0x70U, // OpCode Ref__null func.
      0x05U,        // OpCode Else.
      0xD0U, 0x70U, // OpCode Ref__null func.
      0x0BU,        // OpCode End.
      0x0BU,        // Expression End.
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));
  EXPECT_FALSE(LdrNoMultiVal.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU,        // Code section
      0x08U,        // Content size = 8
      0x01U,        // Vector length = 1
      0x06U,        // Code segment size = 6
      0x00U,        // Local vec(0)
      0xFCU, 0x00U, // OpCode I32__trunc_sat_f32_s.
      0xFCU, 0x01U, // OpCode I32__trunc_sat_f32_u.
      0x0BU         // Expression End.
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));
  EXPECT_FALSE(LdrNoTrapConv.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU, // Code section
      0x09U, // Content size = 9
      0x01U, // Vector length = 1
      0x07U, // Code segment size = 7
      0x00U, // Local vec(0)
      0xC0U, // OpCode I32__extend8_s.
      0xC1U, // OpCode I32__extend16_s.
      0xC2U, // OpCode I64__extend8_s.
      0xC3U, // OpCode I64__extend16_s.
      0xC4U, // OpCode I64__extend32_s.
      0x0BU  // Expression End.
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));
  EXPECT_FALSE(LdrNoSignExt.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU,                      // Code section
      0x10U,                      // Content size = 16
      0x01U,                      // Vector length = 1
      0x0EU,                      // Code segment size = 14
      0x00U,                      // Local vec(0)
      0xFEU, 0x00U, 0x00U, 0x00U, // OpCode Memory__atomic__notify.
      0xFEU, 0x10U, 0x00U, 0x00U, // OpCode I32__atomic__load.
      0xFEU, 0x4EU, 0x00U, 0x00U, // OpCode I64__atomic__rmw32__cmpxchg_u
      0x0BU                       // Expression End.
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));
  EXPECT_TRUE(LdrThreads.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU,        // Code section
      0x06U,        // Content size = 6
      0x01U,        // Vector length = 1
      0x04U,        // Code segment size = 4
      0x00U,        // Local vec(0)
      0x12U, 0x00U, // OpCode Return_call.
      0x0BU         // Expression End.
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));
  EXPECT_TRUE(LdrTailCall.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU,        // Code section
      0x06U,        // Content size = 6
      0x01U,        // Vector length = 1
      0x04U,        // Code segment size = 4
      0x00U,        // Local vec(0)
      0x14U, 0x00U, // OpCode Call_ref.
      0x0BU         // Expression End.
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));
  EXPECT_TRUE(LdrFuncRef.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU,        // Code section
      0x06U,        // Content size = 6
      0x01U,        // Vector length = 1
      0x04U,        // Code segment size = 4
      0x00U,        // Local vec(0)
      0x15U, 0x00U, // OpCode Return_call_ref.
      0x0BU         // Expression End.
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));
  EXPECT_FALSE(LdrFuncRef.parseModule(prefixedVec(Vec)));
  EXPECT_TRUE(LdrFuncRefAndTailCall.parseModule(prefixedVec(Vec)));
}

TEST(InstructionTest, LoadSIMDInstruction) {
  std::vector<uint8_t> Vec;

  // 13. Test SIMD instructions.
  //
  //   1.  Load invalid unexpected end memory align of V128__load.
  //   2.  Load invalid unexpected end memory offset of V128__load.
  //   3.  Load invalid unexpected end memory align of V128__load8_lane.
  //   4.  Load invalid unexpected end memory offset of V128__load8_lane.
  //   5.  Load invalid unexpected end lane index of V128__load8_lane.
  //   6.  Load invalid unexpected end value list of I8x16__shuffle.
  //   7.  Load invalid unexpected end lane index of I8x16__extract_lane_s.

  Vec = {
      0x0AU,       // Code section
      0x05U,       // Content size = 5
      0x01U,       // Vector length = 1
      0x03U,       // Code segment size = 3
      0x00U,       // Local vec(0)
      0xFDU, 0x00U // OpCode V128__load.
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU,        // Code section
      0x0AU,        // Content size = 10
      0x01U,        // Vector length = 1
      0x08U,        // Code segment size = 8
      0x00U,        // Local vec(0)
      0xFDU, 0x00U, // OpCode V128__load.
      0xFFU, 0xFFU, 0xFFU,
      0xFFU, 0x0FU // Align
                   // 0xFEU, 0xFFU, 0xFFU, 0xFFU, 0x0FU  // Missed Offset
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU,       // Code section
      0x05U,       // Content size = 5
      0x01U,       // Vector length = 1
      0x03U,       // Code segment size = 3
      0x00U,       // Local vec(0)
      0xFDU, 0x54U // OpCode V128__load8_lane.
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU,        // Code section
      0x0AU,        // Content size = 10
      0x01U,        // Vector length = 1
      0x08U,        // Code segment size = 8
      0x00U,        // Local vec(0)
      0xFDU, 0x54U, // OpCode V128__load8_lane.
      0xFFU, 0xFFU, 0xFFU,
      0xFFU, 0x0FU // Align
                   // 0xFEU, 0xFFU, 0xFFU, 0xFFU, 0x0FU  // Missed Offset
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU,        // Code section
      0x0FU,        // Content size = 15
      0x01U,        // Vector length = 1
      0x0DU,        // Code segment size = 13
      0x00U,        // Local vec(0)
      0xFDU, 0x54U, // OpCode V128__load8_lane.
      0xFFU, 0xFFU, 0xFFU,
      0xFFU, 0x0FU, // Align
      0xFEU, 0xFFU, 0xFFU,
      0xFFU, 0x0FU // Offset
                   // 0x22U  // Missed lane index
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU,        // Code section
      0x0EU,        // Content size = 14
      0x01U,        // Vector length = 1
      0x0CU,        // Code segment size = 12
      0x00U,        // Local vec(0)
      0xFDU, 0x0DU, // OpCode I8x16__shuffle.
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
      0xFFU, 0xFFU, 0xFFU, 0xFFU // Value list
      // 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU // Missed 7 bytes
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU,       // Code section
      0x05U,       // Content size = 5
      0x01U,       // Vector length = 1
      0x03U,       // Code segment size = 3
      0x00U,       // Local vec(0)
      0xFDU, 0x15U // OpCode I8x16__extract_lane_s.
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));
}
} // namespace
