// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/test/loader/InstructionTest.cpp - Instruction unit tests -===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains unit tests for loading instruction nodes.
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

  Vec = {
      0x0AU,        // Code section
      0x09U,        // Content size = 9
      0x01U,        // Vector length = 1
      0x07U,        // Code segment size = 7
      0x00U,        // Local vec(0)
      0x03U,        // OpCode Loop.
      0xC0U, 0x40U, // Non-canonical SLEB128 blocktype.
      0x0BU,        // OpCode End.
      0x0BU         // Expression End.
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));
}

TEST(InstructionTest, LoadIfElseControlInstruction) {
  std::vector<uint8_t> Vec;

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
             // Missing vec[2] and label index
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));
}

TEST(InstructionTest, LoadCallControlInstruction) {
  std::vector<uint8_t> Vec;

  Conf.setWASMStandard(WasmEdge::Standard::WASM_1);
  WasmEdge::Loader::Loader LdrNoRefType(Conf);
  Conf.setWASMStandard(WasmEdge::Standard::WASM_3);

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
                   // 0x00U  // Missing table index.
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

  Conf.setWASMStandard(WasmEdge::Standard::WASM_1);
  WasmEdge::Loader::Loader LdrNoRefType(Conf);
  Conf.setWASMStandard(WasmEdge::Standard::WASM_3);

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

  Conf.setWASMStandard(WasmEdge::Standard::WASM_1);
  WasmEdge::Loader::Loader LdrNoSIMD(Conf);
  Conf.setWASMStandard(WasmEdge::Standard::WASM_3);

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

  Conf.setWASMStandard(WasmEdge::Standard::WASM_2);
  WasmEdge::Loader::Loader LdrMultiMem(Conf);
  Conf.setWASMStandard(WasmEdge::Standard::WASM_3);

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
             // 0x00  // Missing checking byte
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
            // 0x00  // Missing data index
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
            // 0x01U, 0x02U  // Missing source and target index
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
             // 0x01U  // Missing memory index
  };
  EXPECT_FALSE(LdrMultiMem.parseModule(prefixedVec(Vec)));
}

TEST(InstructionTest, LoadConstInstruction) {
  std::vector<uint8_t> Vec;

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
            // 0x49U, 0xC0U  // Missing 2 bytes
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
                   // 0x21U, 0x09U, 0xC0U  // Missing 3 bytes
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));
}

TEST(InstructionTest, LoadMiscInstruction) {
  std::vector<uint8_t> Vec;

  Vec = {
      0x0AU,        // Code section
      0x06U,        // Content size = 6
      0x01U,        // Vector length = 1
      0x04U,        // Code segment size = 4
      0x00U,        // Local vec(0)
      0xFCU, 0x00U, // OpCode I32__trunc_sat_f32_s.
      0x0BU         // Expression End.
  };
  {
    auto Result = Ldr.parseModule(prefixedVec(Vec));
    ASSERT_TRUE(Result);
    const auto &Codes = (*Result)->getCodeSection().getContent();
    ASSERT_EQ(Codes.size(), 1U);
    const auto Instrs = Codes[0].getExpr().getInstrs();
    ASSERT_EQ(Instrs.size(), 2U);
    EXPECT_EQ(Instrs[0].getOpCode(), WasmEdge::OpCode::I32__trunc_sat_f32_s);
    EXPECT_EQ(Instrs[1].getOpCode(), WasmEdge::OpCode::End);
  }

  Vec = {
      0x0AU,               // Code section
      0x07U,               // Content size = 7
      0x01U,               // Vector length = 1
      0x05U,               // Code segment size = 5
      0x00U,               // Local vec(0)
      0xFCU, 0x80U, 0x00U, // OpCode I32__trunc_sat_f32_s.
      0x0BU                // Expression End.
  };
  {
    auto Result = Ldr.parseModule(prefixedVec(Vec));
    ASSERT_TRUE(Result);
    const auto &Codes = (*Result)->getCodeSection().getContent();
    ASSERT_EQ(Codes.size(), 1U);
    const auto Instrs = Codes[0].getExpr().getInstrs();
    ASSERT_EQ(Instrs.size(), 2U);
    EXPECT_EQ(Instrs[0].getOpCode(), WasmEdge::OpCode::I32__trunc_sat_f32_s);
    EXPECT_EQ(Instrs[1].getOpCode(), WasmEdge::OpCode::End);
  }

  Vec = {
      0x0AU,                      // Code section
      0x08U,                      // Content size = 8
      0x01U,                      // Vector length = 1
      0x06U,                      // Code segment size = 6
      0x00U,                      // Local vec(0)
      0xFCU, 0x80U, 0x80U, 0x00U, // OpCode I32__trunc_sat_f32_s.
      0x0BU                       // Expression End.
  };
  {
    auto Result = Ldr.parseModule(prefixedVec(Vec));
    ASSERT_TRUE(Result);
    const auto &Codes = (*Result)->getCodeSection().getContent();
    ASSERT_EQ(Codes.size(), 1U);
    const auto Instrs = Codes[0].getExpr().getInstrs();
    ASSERT_EQ(Instrs.size(), 2U);
    EXPECT_EQ(Instrs[0].getOpCode(), WasmEdge::OpCode::I32__trunc_sat_f32_s);
    EXPECT_EQ(Instrs[1].getOpCode(), WasmEdge::OpCode::End);
  }

  Vec = {
      0x0AU,                             // Code section
      0x09U,                             // Content size = 9
      0x01U,                             // Vector length = 1
      0x07U,                             // Code segment size = 7
      0x00U,                             // Local vec(0)
      0xFCU, 0x80U, 0x80U, 0x80U, 0x00U, // OpCode I32__trunc_sat_f32_s.
      0x0BU                              // Expression End.
  };
  {
    auto Result = Ldr.parseModule(prefixedVec(Vec));
    ASSERT_TRUE(Result);
    const auto &Codes = (*Result)->getCodeSection().getContent();
    ASSERT_EQ(Codes.size(), 1U);
    const auto Instrs = Codes[0].getExpr().getInstrs();
    ASSERT_EQ(Instrs.size(), 2U);
    EXPECT_EQ(Instrs[0].getOpCode(), WasmEdge::OpCode::I32__trunc_sat_f32_s);
    EXPECT_EQ(Instrs[1].getOpCode(), WasmEdge::OpCode::End);
  }

  Vec = {
      0x0AU,                                    // Code section
      0x0AU,                                    // Content size = 10
      0x01U,                                    // Vector length = 1
      0x08U,                                    // Code segment size = 8
      0x00U,                                    // Local vec(0)
      0xFCU, 0x80U, 0x80U, 0x80U, 0x80U, 0x00U, // OpCode I32__trunc_sat_f32_s.
      0x0BU                                     // Expression End.
  };
  {
    auto Result = Ldr.parseModule(prefixedVec(Vec));
    ASSERT_TRUE(Result);
    const auto &Codes = (*Result)->getCodeSection().getContent();
    ASSERT_EQ(Codes.size(), 1U);
    const auto Instrs = Codes[0].getExpr().getInstrs();
    ASSERT_EQ(Instrs.size(), 2U);
    EXPECT_EQ(Instrs[0].getOpCode(), WasmEdge::OpCode::I32__trunc_sat_f32_s);
    EXPECT_EQ(Instrs[1].getOpCode(), WasmEdge::OpCode::End);
  }

  Vec = {
      0x0AU, // Code section
      0x04U, // Content size = 4
      0x01U, // Vector length = 1
      0x02U, // Code segment size = 2
      0x00U, // Local vec(0)
      0xFCU  // Miscellaneous prefix without an opcode.
  };
  auto Result = Ldr.parseModule(prefixedVec(Vec));
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::UnexpectedEnd);

  Vec = {
      0x0AU,       // Code section
      0x05U,       // Content size = 5
      0x01U,       // Vector length = 1
      0x03U,       // Code segment size = 3
      0x00U,       // Local vec(0)
      0xFCU, 0x80U // Incomplete ULEB32 opcode.
  };
  Result = Ldr.parseModule(prefixedVec(Vec));
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::UnexpectedEnd);

  Vec = {
      0x0AU,                                   // Code section
      0x09U,                                   // Content size = 9
      0x01U,                                   // Vector length = 1
      0x07U,                                   // Code segment size = 7
      0x00U,                                   // Local vec(0)
      0xFCU, 0x80U, 0x80U, 0x80U, 0x80U, 0x10U // Opcode value exceeds ULEB32.
  };
  Result = Ldr.parseModule(prefixedVec(Vec));
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::IntegerTooLarge);

  Vec = {
      0x0AU, // Code section
      0x0AU, // Content size = 10
      0x01U, // Vector length = 1
      0x08U, // Code segment size = 8
      0x00U, // Local vec(0)
      0xFCU, 0x80U, 0x80U, 0x80U,
      0x80U, 0x80U, 0x00U // ULEB32 opcode encoding exceeds five bytes.
  };
  Result = Ldr.parseModule(prefixedVec(Vec));
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::IntegerTooLong);

  Vec = {
      0x0AU,              // Code section
      0x06U,              // Content size = 6
      0x01U,              // Vector length = 1
      0x04U,              // Code segment size = 4
      0x00U,              // Local vec(0)
      0xFCU, 0x80U, 0x02U // Unknown miscellaneous opcode 0x100.
  };
  Result = Ldr.parseModule(prefixedVec(Vec));
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::IllegalOpCode);
  Vec[6] = 0x93U; // Unknown miscellaneous opcode 0x113.
  Result = Ldr.parseModule(prefixedVec(Vec));
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::IllegalOpCode);

  Vec = {
      0x0AU,        // Code section
      0x06U,        // Content size = 6
      0x01U,        // Vector length = 1
      0x04U,        // Code segment size = 4
      0x00U,        // Local vec(0)
      0xFCU, 0x13U, // Wide arithmetic opcode 0x13.
      0x0BU         // Expression End.
  };
  Result = Ldr.parseModule(prefixedVec(Vec));
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::IllegalOpCode);
  Vec[6] = 0x14U; // Wide arithmetic opcode 0x14.
  Result = Ldr.parseModule(prefixedVec(Vec));
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::IllegalOpCode);
  Vec[6] = 0x15U; // Wide arithmetic opcode 0x15.
  Result = Ldr.parseModule(prefixedVec(Vec));
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::IllegalOpCode);
  Vec[6] = 0x16U; // Wide arithmetic opcode 0x16.
  Result = Ldr.parseModule(prefixedVec(Vec));
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::IllegalOpCode);

  Vec = {
      0x0AU,               // Code section
      0x07U,               // Content size = 7
      0x01U,               // Vector length = 1
      0x05U,               // Code segment size = 5
      0x00U,               // Local vec(0)
      0xFCU, 0x93U, 0x00U, // Wide arithmetic opcode 0x13.
      0x0BU                // Expression End.
  };
  Result = Ldr.parseModule(prefixedVec(Vec));
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::IllegalOpCode);
  Vec[6] = 0x94U; // Wide arithmetic opcode 0x14.
  Result = Ldr.parseModule(prefixedVec(Vec));
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::IllegalOpCode);
  Vec[6] = 0x95U; // Wide arithmetic opcode 0x15.
  Result = Ldr.parseModule(prefixedVec(Vec));
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::IllegalOpCode);
  Vec[6] = 0x96U; // Wide arithmetic opcode 0x16.
  Result = Ldr.parseModule(prefixedVec(Vec));
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::IllegalOpCode);

  Vec = {
      0x0AU,                      // Code section
      0x08U,                      // Content size = 8
      0x01U,                      // Vector length = 1
      0x06U,                      // Code segment size = 6
      0x00U,                      // Local vec(0)
      0xFCU, 0x93U, 0x80U, 0x00U, // Wide arithmetic opcode 0x13.
      0x0BU                       // Expression End.
  };
  Result = Ldr.parseModule(prefixedVec(Vec));
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::IllegalOpCode);
  Vec[6] = 0x94U; // Wide arithmetic opcode 0x14.
  Result = Ldr.parseModule(prefixedVec(Vec));
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::IllegalOpCode);
  Vec[6] = 0x95U; // Wide arithmetic opcode 0x15.
  Result = Ldr.parseModule(prefixedVec(Vec));
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::IllegalOpCode);
  Vec[6] = 0x96U; // Wide arithmetic opcode 0x16.
  Result = Ldr.parseModule(prefixedVec(Vec));
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::IllegalOpCode);

  Vec = {
      0x0AU,                             // Code section
      0x09U,                             // Content size = 9
      0x01U,                             // Vector length = 1
      0x07U,                             // Code segment size = 7
      0x00U,                             // Local vec(0)
      0xFCU, 0x93U, 0x80U, 0x80U, 0x00U, // Wide arithmetic opcode 0x13.
      0x0BU                              // Expression End.
  };
  Result = Ldr.parseModule(prefixedVec(Vec));
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::IllegalOpCode);
  Vec[6] = 0x94U; // Wide arithmetic opcode 0x14.
  Result = Ldr.parseModule(prefixedVec(Vec));
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::IllegalOpCode);
  Vec[6] = 0x95U; // Wide arithmetic opcode 0x15.
  Result = Ldr.parseModule(prefixedVec(Vec));
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::IllegalOpCode);
  Vec[6] = 0x96U; // Wide arithmetic opcode 0x16.
  Result = Ldr.parseModule(prefixedVec(Vec));
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::IllegalOpCode);

  Vec = {
      0x0AU,                                    // Code section
      0x0AU,                                    // Content size = 10
      0x01U,                                    // Vector length = 1
      0x08U,                                    // Code segment size = 8
      0x00U,                                    // Local vec(0)
      0xFCU, 0x93U, 0x80U, 0x80U, 0x80U, 0x00U, // Wide arithmetic opcode 0x13.
      0x0BU                                     // Expression End.
  };
  Result = Ldr.parseModule(prefixedVec(Vec));
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::IllegalOpCode);
  Vec[6] = 0x94U; // Wide arithmetic opcode 0x14.
  Result = Ldr.parseModule(prefixedVec(Vec));
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::IllegalOpCode);
  Vec[6] = 0x95U; // Wide arithmetic opcode 0x15.
  Result = Ldr.parseModule(prefixedVec(Vec));
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::IllegalOpCode);
  Vec[6] = 0x96U; // Wide arithmetic opcode 0x16.
  Result = Ldr.parseModule(prefixedVec(Vec));
  ASSERT_FALSE(Result);
  EXPECT_EQ(Result.error(), WasmEdge::ErrCode::Value::IllegalOpCode);
}

TEST(InstructionTest, Proposals) {
  std::vector<uint8_t> Vec;

  Conf.setWASMStandard(WasmEdge::Standard::WASM_1);
  WasmEdge::Loader::Loader LdrWASM1(Conf);

  Conf.setWASMStandard(WasmEdge::Standard::WASM_2);
  WasmEdge::Loader::Loader LdrWASM2(Conf);

  Conf.setWASMStandard(WasmEdge::Standard::WASM_3);
  Conf.addProposal(WasmEdge::Proposal::Threads);
  WasmEdge::Loader::Loader LdrThreads(Conf);
  Conf.removeProposal(WasmEdge::Proposal::Threads);

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
  EXPECT_FALSE(LdrWASM1.parseModule(prefixedVec(Vec)));
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
  EXPECT_FALSE(LdrWASM1.parseModule(prefixedVec(Vec)));
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

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
  EXPECT_FALSE(LdrWASM1.parseModule(prefixedVec(Vec)));
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
  EXPECT_FALSE(LdrWASM1.parseModule(prefixedVec(Vec)));
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

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
  EXPECT_FALSE(LdrWASM1.parseModule(prefixedVec(Vec)));
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

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
  EXPECT_FALSE(LdrWASM1.parseModule(prefixedVec(Vec)));
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

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
  EXPECT_FALSE(LdrWASM1.parseModule(prefixedVec(Vec)));
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

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
  EXPECT_FALSE(LdrWASM2.parseModule(prefixedVec(Vec)));
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU,        // Code section
      0x06U,        // Content size = 6
      0x01U,        // Vector length = 1
      0x04U,        // Code segment size = 4
      0x00U,        // Local vec(0)
      0x14U, 0x00U, // OpCode Call_ref.
      0x0BU         // Expression End.
  };
  EXPECT_FALSE(LdrWASM2.parseModule(prefixedVec(Vec)));
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU,        // Code section
      0x06U,        // Content size = 6
      0x01U,        // Vector length = 1
      0x04U,        // Code segment size = 4
      0x00U,        // Local vec(0)
      0x15U, 0x00U, // OpCode Return_call_ref.
      0x0BU         // Expression End.
  };
  EXPECT_FALSE(LdrWASM2.parseModule(prefixedVec(Vec)));
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));
}

TEST(InstructionTest, LoadSIMDInstruction) {
  std::vector<uint8_t> Vec;

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
                   // 0xFEU, 0xFFU, 0xFFU, 0xFFU, 0x0FU  // Missing offset
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
                   // 0xFEU, 0xFFU, 0xFFU, 0xFFU, 0x0FU  // Missing offset
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
                   // 0x22U  // Missing lane index
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
      // 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU // Missing 7 bytes
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

TEST(InstructionTest, LoadTryTable) {
  std::vector<uint8_t> Vec;

  Vec = {
      0x00U, 0x61U, 0x73U, 0x6DU, // Magic
      0x01U, 0x00U, 0x00U, 0x00U, // Version

      // --- Type section ---
      0x01U,               // Section ID: Type
      0x04U,               // Content size = 4
      0x01U,               // Vector length = 1
      0x60U, 0x00U, 0x00U, // FuncType: () -> ()

      // --- Function section ---
      0x03U, // Section ID: Function
      0x02U, // Content size = 2
      0x01U, // Vector length = 1
      0x00U, // Type index = 0

      // --- Tag section ---
      0x0DU,        // Section ID: Tag
      0x03U,        // Content size = 3
      0x01U,        // Vector length = 1
      0x00U, 0x00U, // Exception kind = 0, type index = 0

      // --- Code section ---
      0x0AU,               // Section ID: Code
      0x12U,               // Content size = 18
      0x01U,               // Vector length = 1
      0x10U,               // Code segment size = 16
      0x00U,               // Local vec(0)
      0x1FU,               // OpCode Try_table
      0x40U,               // Block type: void
      0x04U,               // 4 catch clauses
      0x00U, 0x00U, 0x00U, // catch (flag=0x00): tag_idx=0, label_idx=0
      0x01U, 0x00U, 0x00U, // catch_ref (flag=0x01): tag_idx=0, label_idx=0
      0x02U, 0x00U,        // catch_all (flag=0x02): label_idx=0
      0x03U, 0x00U,        // catch_all_ref (flag=0x03): label_idx=0
      0x0BU,               // OpCode End (try_table block)
      0x0BU                // Expression End
  };
  EXPECT_TRUE(Ldr.parseModule(Vec));

  Vec = {
      0x00U, 0x61U, 0x73U, 0x6DU, // Magic
      0x01U, 0x00U, 0x00U, 0x00U, // Version

      // --- Type section ---
      0x01U,               // Section ID: Type
      0x04U,               // Content size = 4
      0x01U,               // Vector length = 1
      0x60U, 0x00U, 0x00U, // FuncType: () -> ()

      // --- Function section ---
      0x03U, // Section ID: Function
      0x02U, // Content size = 2
      0x01U, // Vector length = 1
      0x00U, // Type index = 0

      // --- Tag section ---
      0x0DU,        // Section ID: Tag
      0x03U,        // Content size = 3
      0x01U,        // Vector length = 1
      0x00U, 0x00U, // Exception kind = 0, type index = 0

      // --- Code section ---
      0x0AU,               // Section ID: Code
      0x0BU,               // Content size = 11
      0x01U,               // Vector length = 1
      0x09U,               // Code segment size = 9
      0x00U,               // Local vec(0)
      0x1FU,               // OpCode Try_table
      0x40U,               // Block type: void
      0x01U,               // 1 catch clause
      0x04U, 0x00U, 0x00U, // INVALID flag=0x04, tag_idx=0, label_idx=0
      0x0BU,               // OpCode End (try_table block)
      0x0BU                // Expression End
  };
  EXPECT_FALSE(Ldr.parseModule(Vec));

  Vec = {
      0x00U, 0x61U, 0x73U, 0x6DU, // Magic
      0x01U, 0x00U, 0x00U, 0x00U, // Version

      // --- Type section ---
      0x01U,               // Section ID: Type
      0x04U,               // Content size = 4
      0x01U,               // Vector length = 1
      0x60U, 0x00U, 0x00U, // FuncType: () -> ()

      // --- Function section ---
      0x03U, // Section ID: Function
      0x02U, // Content size = 2
      0x01U, // Vector length = 1
      0x00U, // Type index = 0

      // --- Tag section ---
      0x0DU,        // Section ID: Tag
      0x03U,        // Content size = 3
      0x01U,        // Vector length = 1
      0x00U, 0x00U, // Exception kind = 0, type index = 0

      // --- Code section ---
      0x0AU,               // Section ID: Code
      0x0BU,               // Content size = 11
      0x01U,               // Vector length = 1
      0x09U,               // Code segment size = 9
      0x00U,               // Local vec(0)
      0x1FU,               // OpCode Try_table
      0x40U,               // Block type: void
      0x01U,               // 1 catch clause
      0xFFU, 0x00U, 0x00U, // INVALID flag=0xFF, tag_idx=0, label_idx=0
      0x0BU,               // OpCode End (try_table block)
      0x0BU                // Expression End
  };
  EXPECT_FALSE(Ldr.parseModule(Vec));
}

TEST(InstructionTest, LoadBrOnCastFlags) {
  std::vector<uint8_t> Vec;

  Vec = {
      0x0AU, // Code section
      0x34U, // Content size = 52
      0x01U, // Vector length = 1
      0x32U, // Code segment size = 50
      0x00U, // Local vec(0)

      0xFBU, 0x18U, // OpCode Br_on_cast.
      0x00U,        // Cast flags.
      0x00U,        // Label index.
      0x6DU, 0x6DU, // Source and destination heap types.
      0xFBU, 0x18U, // OpCode Br_on_cast.
      0x01U,        // Cast flags.
      0x00U,        // Label index.
      0x6DU, 0x6DU, // Source and destination heap types.
      0xFBU, 0x18U, // OpCode Br_on_cast.
      0x02U,        // Cast flags.
      0x00U,        // Label index.
      0x6DU, 0x6DU, // Source and destination heap types.
      0xFBU, 0x18U, // OpCode Br_on_cast.
      0x03U,        // Cast flags.
      0x00U,        // Label index.
      0x6DU, 0x6DU, // Source and destination heap types.
      0xFBU, 0x19U, // OpCode Br_on_cast_fail.
      0x00U,        // Cast flags.
      0x00U,        // Label index.
      0x6DU, 0x6DU, // Source and destination heap types.
      0xFBU, 0x19U, // OpCode Br_on_cast_fail.
      0x01U,        // Cast flags.
      0x00U,        // Label index.
      0x6DU, 0x6DU, // Source and destination heap types.
      0xFBU, 0x19U, // OpCode Br_on_cast_fail.
      0x02U,        // Cast flags.
      0x00U,        // Label index.
      0x6DU, 0x6DU, // Source and destination heap types.
      0xFBU, 0x19U, // OpCode Br_on_cast_fail.
      0x03U,        // Cast flags.
      0x00U,        // Label index.
      0x6DU, 0x6DU, // Source and destination heap types.
      0x0BU         // Expression End.
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU,        // Code section
      0x0AU,        // Content size = 10
      0x01U,        // Vector length = 1
      0x08U,        // Code segment size = 8
      0x00U,        // Local vec(0)
      0xFBU, 0x18U, // OpCode Br_on_cast.
      0x04U,        // Invalid cast flags.
      0x00U,        // Label index.
      0x6DU, 0x6DU, // Source and destination heap types.
      0x0BU         // Expression End.
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU,        // Code section
      0x0AU,        // Content size = 10
      0x01U,        // Vector length = 1
      0x08U,        // Code segment size = 8
      0x00U,        // Local vec(0)
      0xFBU, 0x18U, // OpCode Br_on_cast.
      0xFFU,        // Invalid cast flags.
      0x00U,        // Label index.
      0x6DU, 0x6DU, // Source and destination heap types.
      0x0BU         // Expression End.
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU,        // Code section
      0x0AU,        // Content size = 10
      0x01U,        // Vector length = 1
      0x08U,        // Code segment size = 8
      0x00U,        // Local vec(0)
      0xFBU, 0x19U, // OpCode Br_on_cast_fail.
      0x04U,        // Invalid cast flags.
      0x00U,        // Label index.
      0x6DU, 0x6DU, // Source and destination heap types.
      0x0BU         // Expression End.
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU,        // Code section
      0x0AU,        // Content size = 10
      0x01U,        // Vector length = 1
      0x08U,        // Code segment size = 8
      0x00U,        // Local vec(0)
      0xFBU, 0x19U, // OpCode Br_on_cast_fail.
      0xFFU,        // Invalid cast flags.
      0x00U,        // Label index.
      0x6DU, 0x6DU, // Source and destination heap types.
      0x0BU         // Expression End.
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));
}

} // namespace
