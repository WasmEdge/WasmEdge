// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/test/loader/descriptionTest.cpp - Load AST description ---===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contents unit tests of loading AST description nodes, which are
/// ImportDesc and ExportDesc.
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
      0x01U, 0x00U, 0x00U, 0x00U  // Version
  };
  PrefixVec.reserve(PrefixVec.size() + Vec.size());
  PrefixVec.insert(PrefixVec.end(), Vec.begin(), Vec.end());
  return PrefixVec;
}

TEST(DescriptionTest, LoadImportDesc) {
  std::vector<uint8_t> Vec;

  Conf.removeProposal(WasmEdge::Proposal::ImportExportMutGlobals);
  WasmEdge::Loader::Loader LdrNoImpMutGlob(Conf);

  // 1. Test load import description.
  //
  //   1.  Load invalid empty import description.
  //   2.  Load import description with empty module and external name.
  //   3.  Load import description with module and external names.
  //   4.  Load import description with invalid external type.
  //   5.  Load import description of table type.
  //   6.  Load import description of memory type.
  //   7.  Load import description of global type.
  //   8.  Load invalid import description of global type without Mut-Globals
  //       proposal.

  Vec = {
      0x02U, // Import section
      0x01U, // Content size = 1
      0x01U  // Vector length = 1
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x02U,       // Import section
      0x05U,       // Content size = 5
      0x01U,       // Vector length = 1
      0x00U,       // Empty module name
      0x00U,       // Empty external name
      0x00U, 0x00U // function type and index
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x02U,                                           // Import section
      0x0FU,                                           // Content size = 15
      0x01U,                                           // Vector length = 1
      0x04U, 0x74U, 0x65U, 0x73U, 0x74U,               // Module name: test
      0x06U, 0x4CU, 0x6FU, 0x61U, 0x64U, 0x65U, 0x72U, // External name: Loader
      0x00U, 0x00U // function type and index
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x02U,                                           // Import section
      0x0EU,                                           // Content size = 14
      0x01U,                                           // Vector length = 1
      0x04U, 0x74U, 0x65U, 0x73U, 0x74U,               // Module name: test
      0x06U, 0x4CU, 0x6FU, 0x61U, 0x64U, 0x65U, 0x72U, // External name: Loader
      0x04U, 0x00U                                     // Invalid external type
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x02U,                                           // Import section
      0x1AU,                                           // Content size = 26
      0x01U,                                           // Vector length = 1
      0x04U, 0x74U, 0x65U, 0x73U, 0x74U,               // Module name: test
      0x06U, 0x4CU, 0x6FU, 0x61U, 0x64U, 0x65U, 0x72U, // External name: Loader
      0x01U,                                           // Table type
      0x70U,                                           // Reference type
      0x01U,                                           // Has min and max
      0xF1U, 0xFFU, 0xFFU, 0xFFU, 0x0FU,               // Min = 4294967281
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU                // Max = 4294967295
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x02U,                                           // Import section
      0x19U,                                           // Content size = 25
      0x01U,                                           // Vector length = 1
      0x04U, 0x74U, 0x65U, 0x73U, 0x74U,               // Module name: test
      0x06U, 0x4CU, 0x6FU, 0x61U, 0x64U, 0x65U, 0x72U, // External name: Loader
      0x02U,                                           // Memory type
      0x01U,                                           // Has min and max
      0xF1U, 0xFFU, 0xFFU, 0xFFU, 0x0FU,               // Min = 4294967281
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU                // Max = 4294967295
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x02U,                                           // Import section
      0x10U,                                           // Content size = 16
      0x01U,                                           // Vector length = 1
      0x04U, 0x74U, 0x65U, 0x73U, 0x74U,               // Module name: test
      0x06U, 0x4CU, 0x6FU, 0x61U, 0x64U, 0x65U, 0x72U, // External name: Loader
      0x03U,                                           // Global type
      0x7CU, 0x00U                                     // Const F64 number type
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x02U,                                           // Import section
      0x10U,                                           // Content size = 16
      0x01U,                                           // Vector length = 1
      0x04U, 0x74U, 0x65U, 0x73U, 0x74U,               // Module name: test
      0x06U, 0x4CU, 0x6FU, 0x61U, 0x64U, 0x65U, 0x72U, // External name: Loader
      0x03U,                                           // Global type
      0x7CU, 0x01U                                     // Mut F64 number type
  };
  EXPECT_FALSE(LdrNoImpMutGlob.parseModule(prefixedVec(Vec)));
}

TEST(DescriptionTest, LoadExportDesc) {
  std::vector<uint8_t> Vec;

  // 2. Test load export description.
  //
  //   1.  Load invalid empty export description.
  //   2.  Load export description with empty module name.
  //   3.  Load export description with non-empty module name.
  //   4.  Load export description with invalid external type.
  //   5.  Load export description of table type.

  Vec = {
      0x07U, // Export section
      0x01U, // Content size = 1
      0x01U  // Vector length = 1
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x07U,       // Export section
      0x04U,       // Content size = 4
      0x01U,       // Vector length = 1
      0x00U,       // Empty external name
      0x00U, 0x00U // function type and index
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x07U,                                           // Export section
      0x0AU,                                           // Content size = 10
      0x01U,                                           // Vector length = 1
      0x06U, 0x4CU, 0x6FU, 0x61U, 0x64U, 0x65U, 0x72U, // External name: Loader
      0x00U, 0x00U // function type and index
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x07U,                                           // Export section
      0x0AU,                                           // Content size = 10
      0x01U,                                           // Vector length = 1
      0x06U, 0x4CU, 0x6FU, 0x61U, 0x64U, 0x65U, 0x72U, // External name: Loader
      0x05U, 0x00U                                     // Invalid external type
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x07U,                                           // Export section
      0x0EU,                                           // Content size = 14
      0x01U,                                           // Vector length = 1
      0x06U, 0x4CU, 0x6FU, 0x61U, 0x64U, 0x65U, 0x72U, // External name: Loader
      0x01U, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU // Table type and table index
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));
}
} // namespace
