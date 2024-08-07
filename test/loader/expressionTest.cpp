// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/test/loader/expressionTest.cpp - Load AST expression tests===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contents unit tests of loading AST expression node.
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

TEST(ExpressionTest, LoadExpression) {
  std::vector<uint8_t> Vec;

  Conf.removeProposal(WasmEdge::Proposal::BulkMemoryOperations);
  Conf.removeProposal(WasmEdge::Proposal::ReferenceTypes);
  WasmEdge::Loader::Loader LdrNoRefType(Conf);

  // 1. Test load limit.
  //
  //   1.  Load invalid empty expression.
  //   2.  Load expression with only end operation.
  //   3.  Load expression with invalid operations.
  //   4.  Load expression with instructions.
  //   5.  Load expression with instructions not in proposals.

  Vec = {
      0x0AU, // Code section
      0x03U, // Content size = 3
      0x01U, // Vector length = 1
      0x01U, // Code segment size = 1
      0x00U  // Local vec(0)
             // Expression: empty
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU, // Code section
      0x04U, // Content size = 4
      0x01U, // Vector length = 1
      0x02U, // Code segment size = 2
      0x00U, // Local vec(0)
      0x0BU  // Expression: OpCode End.
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU,               // Code section
      0x0AU,               // Content size = 10
      0x01U,               // Vector length = 1
      0x08U,               // Code segment size = 8
      0x00U,               // Local vec(0)
      0x45U, 0x46U, 0x47U, // Valid OpCodes.
      0xEDU, 0xEEU, 0xEFU, // Invalid OpCodes.
      0x0BU                // OpCode End.
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU,               // Code section
      0x07U,               // Content size = 7
      0x01U,               // Vector length = 1
      0x05U,               // Code segment size = 5
      0x00U,               // Local vec(0)
      0x45U, 0x46U, 0x47U, // Valid OpCodes.
      0x0BU                // OpCode End.
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU,        // Code section
      0x06U,        // Content size = 6
      0x01U,        // Vector length = 1
      0x04U,        // Code segment size = 4
      0x00U,        // Local vec(0)
      0x25U, 0x00U, // Table_get.
      0x0BU         // OpCode End.
  };
  EXPECT_FALSE(LdrNoRefType.parseModule(prefixedVec(Vec)));
}

} // namespace
