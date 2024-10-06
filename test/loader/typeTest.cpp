// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/test/loader/typeTest.cpp - Load AST type unit tests ------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contents unit tests of loading AST type nodes.
///
//===----------------------------------------------------------------------===//

#include "loader/loader.h"

#include <cstdint>
#include <gtest/gtest.h>
#include <vector>

// AST::Limit test is contained in AST::MemoryType.

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

TEST(TypeTest, LoadFunctionType) {
  std::vector<uint8_t> Vec;

  Conf.removeProposal(WasmEdge::Proposal::BulkMemoryOperations);
  Conf.removeProposal(WasmEdge::Proposal::ReferenceTypes);
  WasmEdge::Loader::Loader LdrNoRefType(Conf);
  Conf.addProposal(WasmEdge::Proposal::BulkMemoryOperations);
  Conf.addProposal(WasmEdge::Proposal::ReferenceTypes);
  Conf.removeProposal(WasmEdge::Proposal::MultiValue);
  WasmEdge::Loader::Loader LdrNoMultiVal(Conf);
  Conf.addProposal(WasmEdge::Proposal::MultiValue);

  // 1. Test load function type.
  //
  //   1.  Load invalid empty function type.
  //   2.  Load invalid types of function type.
  //   3.  Load void parameter and result function type.
  //   4.  Load non-void parameter function type.
  //   5.  Load non-void result function type.
  //   6.  Load function type with parameters and result.
  //   7.  Load invalid parameters with unexpected end.
  //   8.  Load invalid results with unexpected end.
  //   9.  Load invalid parameters with invalid value types.
  //   10. Load invalid results with invalid value types.
  //   11. Load invalid parameters with ExternRef without Ref-Types proposal.
  //   12. Load invalid results with ExternRef without Ref-Types proposal.
  //   13. Load invalid parameters with invalid value types without Ref-Types
  //       proposal.
  //   14. Load invalid results with invalid value types without Ref-Types
  //       proposal.
  //   15. Load invalid function type with multi-value returns without
  //       Multi-Value proposal.

  Vec = {
      0x01U, // Type section
      0x01U, // Content size = 1
      0x01U  // Vector length = 1
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x01U, // Type section
      0x04U, // Content size = 4
      0x01U, // Vector length = 1
      0xFFU, // Invalid function type header
      0x00U, // Parameter length = 0
      0x00U  // Result length = 0
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x01U, // Type section
      0x04U, // Content size = 4
      0x01U, // Vector length = 1
      0x60U, // Function type header
      0x00U, // Parameter length = 0
      0x00U  // Result length = 0
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x01U,                      // Type section
      0x08U,                      // Content size = 8
      0x01U,                      // Vector length = 1
      0x60U,                      // Function type header
      0x04U,                      // Parameter length = 4
      0x7CU, 0x7DU, 0x7EU, 0x7FU, // Parameter list
      0x00U                       // Result length = 0
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x01U, // Type section
      0x05U, // Content size = 5
      0x01U, // Vector length = 1
      0x60U, // Function type header
      0x00U, // Parameter length = 0
      0x01U, // Result length = 1
      0x7CU  // Result list
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x01U,                      // Type section
      0x09U,                      // Content size = 9
      0x01U,                      // Vector length = 1
      0x60U,                      // Function type header
      0x04U,                      // Parameter length = 4
      0x7CU, 0x7DU, 0x7EU, 0x7FU, // Parameter list
      0x01U,                      // Result length = 1
      0x7CU                       // Result list
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x01U,       // Type section
      0x05U,       // Content size = 5
      0x01U,       // Vector length = 1
      0x60U,       // Function type header
      0x04U,       // Parameter length = 4
      0x7CU, 0x7DU // Parameter list only in 2
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x01U,                      // Type section
      0x08U,                      // Content size = 8
      0x01U,                      // Vector length = 1
      0x60U,                      // Function type header
      0x04U,                      // Parameter length = 4
      0x7CU, 0x7DU, 0x7EU, 0x7FU, // Parameter list
      0x02U,                      // Result length = 2
      0x7CU                       // Result list only in 1
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x01U,        // Type section
      0x07U,        // Content size = 7
      0x01U,        // Vector length = 1
      0x60U,        // Function type header
      0x02U,        // Parameter length = 2
      0x6DU, 0x6DU, // Parameter list with invalid value types
      0x01U,        // Result length = 2
      0x7CU, 0x7FU  // Result list
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x01U,        // Type section
      0x07U,        // Content size = 7
      0x01U,        // Vector length = 1
      0x60U,        // Function type header
      0x02U,        // Parameter length = 2
      0x7CU, 0x7FU, // Parameter list
      0x01U,        // Result length = 2
      0x6DU, 0x6DU  // Result list with invalid value types
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x01U,        // Type section
      0x07U,        // Content size = 7
      0x01U,        // Vector length = 1
      0x60U,        // Function type header
      0x02U,        // Parameter length = 2
      0x6FU, 0x6FU, // Parameter list with ExternRef
      0x01U,        // Result length = 2
      0x7CU, 0x7FU  // Result list
  };
  EXPECT_FALSE(LdrNoRefType.parseModule(prefixedVec(Vec)));

  Vec = {
      0x01U,        // Type section
      0x07U,        // Content size = 7
      0x01U,        // Vector length = 1
      0x60U,        // Function type header
      0x02U,        // Parameter length = 2
      0x7CU, 0x7FU, // Parameter list
      0x01U,        // Result length = 2
      0x6FU, 0x6FU  // Result list with ExternRef
  };
  EXPECT_FALSE(LdrNoRefType.parseModule(prefixedVec(Vec)));

  Vec = {
      0x01U,        // Type section
      0x07U,        // Content size = 7
      0x01U,        // Vector length = 1
      0x60U,        // Function type header
      0x02U,        // Parameter length = 2
      0x6DU, 0x6DU, // Parameter list with invalid value types
      0x01U,        // Result length = 2
      0x7CU, 0x7FU  // Result list
  };
  EXPECT_FALSE(LdrNoRefType.parseModule(prefixedVec(Vec)));

  Vec = {
      0x01U,        // Type section
      0x07U,        // Content size = 7
      0x01U,        // Vector length = 1
      0x60U,        // Function type header
      0x02U,        // Parameter length = 2
      0x7CU, 0x7FU, // Parameter list
      0x01U,        // Result length = 2
      0x6DU, 0x6DU  // Result list with invalid value types
  };
  EXPECT_FALSE(LdrNoRefType.parseModule(prefixedVec(Vec)));

  Vec = {
      0x01U,                      // Type section
      0x09U,                      // Content size = 9
      0x01U,                      // Vector length = 1
      0x60U,                      // Function type header
      0x04U,                      // Parameter length = 4
      0x7CU, 0x7DU, 0x7EU, 0x7FU, // Parameter list
      0x02U,                      // Result length = 2
      0x7CU, 0x7DU                // Result list
  };
  EXPECT_FALSE(LdrNoMultiVal.parseModule(prefixedVec(Vec)));
}

TEST(TypeTest, LoadTableType) {
  std::vector<uint8_t> Vec;

  Conf.removeProposal(WasmEdge::Proposal::BulkMemoryOperations);
  Conf.removeProposal(WasmEdge::Proposal::ReferenceTypes);
  WasmEdge::Loader::Loader LdrNoRefType(Conf);
  Conf.addProposal(WasmEdge::Proposal::BulkMemoryOperations);
  Conf.addProposal(WasmEdge::Proposal::ReferenceTypes);

  // 3. Test load table type, which is reference type and limit.
  //
  //   1.  Load invalid empty table type.
  //   2.  Load invalid reference type.
  //   3.  Load invalid types of limit in table type.
  //   4.  Load limit with only min.
  //   5.  Load invalid limit with fail of loading max.
  //   6.  Load limit with min and max.
  //   7.  Load invalid ExternRef without Ref-Types proposal.
  //   8.  Load invalid reference type without Ref-Types proposal.

  Vec = {
      0x04U, // Table section
      0x01U, // Content size = 1
      0x01U  // Vector length = 1
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x04U, // Table section
      0x04U, // Content size = 4
      0x01U, // Vector length = 1
      0xFFU, // Unknown reference type
      0x00U, // Limit with only has min
      0x00U  // Min = 0
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x04U, // Table section
      0x04U, // Content size = 4
      0x01U, // Vector length = 1
      0x70U, // Reference type
      0x02U, // Unknown limit type
      0x00U  // Min = 0
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x04U,                            // Table section
      0x08U,                            // Content size = 8
      0x01U,                            // Vector length = 1
      0x70U,                            // Reference type
      0x00U,                            // Only has min
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU // Min = 4294967295
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x04U,                            // Table section
      0x08U,                            // Content size = 8
      0x01U,                            // Vector length = 1
      0x70U,                            // Reference type
      0x01U,                            // Has min and max
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU // Min = 4294967295
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x04U,                             // Table section
      0x0DU,                             // Content size = 13
      0x01U,                             // Vector length = 1
      0x70U,                             // Reference type
      0x01U,                             // Has min and max
      0xF1U, 0xFFU, 0xFFU, 0xFFU, 0x0FU, // Min = 4294967281
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU  // Max = 4294967295
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x04U, // Table section
      0x04U, // Content size = 4
      0x01U, // Vector length = 1
      0xFFU, // Unknown reference type
      0x00U, // Limit with only has min
      0x00U  // Min = 0
  };
  EXPECT_FALSE(LdrNoRefType.parseModule(prefixedVec(Vec)));

  Vec = {
      0x04U, // Table section
      0x03U, // Content size = 3
      0x01U, // Vector length = 1
      0x6FU, // ExternRef without proposals
      0x00U, // Limit with only has min
      0x00U  // Min = 0
  };
  EXPECT_FALSE(LdrNoRefType.parseModule(prefixedVec(Vec)));
}

TEST(TypeTest, LoadMemoryType) {
  std::vector<uint8_t> Vec;

  // 2. Test load memory type, which is limit.
  //
  //   1.  Load invalid empty limit.
  //   2.  Load invalid types of limit.
  //   3.  Load limit with only min.
  //   4.  Load invalid limit with fail of loading max.
  //   5.  Load limit with min and max.

  Vec = {
      0x05U, // Memory section
      0x01U, // Content size = 1
      0x01U  // Vector length = 1
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x05U, // Memory section
      0x03U, // Content size = 3
      0x01U, // Vector length = 1
      0x02U, // Unknown limit type
      0x00U  // Min = 0
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x05U,                            // Memory section
      0x07U,                            // Content size = 7
      0x01U,                            // Vector length = 1
      0x00U,                            // Only has min
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU // Min = 4294967295
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x05U,                            // Memory section
      0x07U,                            // Content size = 7
      0x01U,                            // Vector length = 1
      0x01U,                            // Has min and max
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU // Min = 4294967295
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x05U,                             // Memory section
      0x0CU,                             // Content size = 12
      0x01U,                             // Vector length = 1
      0x01U,                             // Has min and max
      0xF1U, 0xFFU, 0xFFU, 0xFFU, 0x0FU, // Min = 4294967281
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU  // Max = 4294967295
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));
}

TEST(TypeTest, LoadGlobalType) {
  std::vector<uint8_t> Vec;

  Conf.removeProposal(WasmEdge::Proposal::BulkMemoryOperations);
  Conf.removeProposal(WasmEdge::Proposal::ReferenceTypes);
  WasmEdge::Loader::Loader LdrNoRefType(Conf);
  Conf.addProposal(WasmEdge::Proposal::BulkMemoryOperations);
  Conf.addProposal(WasmEdge::Proposal::ReferenceTypes);

  // 4. Test load global type.
  //
  //   1.  Load invalid empty global type.
  //   2.  Load invalid global type without mutation.
  //   3.  Load invalid value type of global type.
  //   4.  Load invalid mutation of global type.
  //   5.  Load valid global type.
  //   6.  Load invalid global type with ExternRef without Ref-Types proposal.

  Vec = {
      0x06U, // Global section
      0x01U, // Content size = 1
      0x01U  // Vector length = 1
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x06U, // Global section
      0x02U, // Content size = 2
      0x01U, // Vector length = 1
      0x7CU  // F64 number type
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x06U, // Global section
      0x04U, // Content size = 4
      0x01U, // Vector length = 1
      0x6DU, // Unknown value type
      0x00U, // Const mutation
      0x0BU  // Expression
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x06U, // Global section
      0x04U, // Content size = 4
      0x01U, // Vector length = 1
      0x7CU, // F64 number type
      0xFFU, // Invalid mutation type
      0x0BU  // Expression
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x06U, // Global section
      0x04U, // Content size = 4
      0x01U, // Vector length = 1
      0x7CU, // F64 number type
      0x00U, // Const mutation
      0x0BU  // Expression
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x06U, // Global section
      0x04U, // Content size = 4
      0x01U, // Vector length = 1
      0x6FU, // ExternRef
      0x00U, // Const mutation
      0x0BU  // Expression
  };
  EXPECT_FALSE(LdrNoRefType.parseModule(prefixedVec(Vec)));
}

TEST(TypeTest, LoadHeapType) {
  std::vector<uint8_t> Vec;

  Conf.addProposal(WasmEdge::Proposal::FunctionReferences);
  WasmEdge::Loader::Loader LdrFuncRef(Conf);
  Conf.removeProposal(WasmEdge::Proposal::FunctionReferences);

  // 5. Test load heap type.
  //
  //   1.  Load invalid empty heap type.
  //   2.  Load invalid heap type with unknown heap type code.
  //   3.  Load invalid heap type with type index with/without typed function
  //       references proposal.

  Vec = {
      0x06U,        // Global section
      0x04U,        // Content size = 4
      0x01U,        // Vector length = 1
      0x7FU, 0x00U, // Global type
      0xD0U         // OpCode Ref__null
                    // Missed heap type
  };
  EXPECT_FALSE(LdrFuncRef.parseModule(prefixedVec(Vec)));

  Vec = {
      0x06U,        // Global section
      0x06U,        // Content size = 6
      0x01U,        // Vector length = 1
      0x7FU, 0x00U, // Global type
      0xD0U,        // OpCode Ref__null
      0x5CU,        // Invalid heap type code
      0x0BU         // Expression End
  };
  EXPECT_FALSE(LdrFuncRef.parseModule(prefixedVec(Vec)));

  Vec = {
      0x06U,        // Global section
      0x06U,        // Content size = 6
      0x01U,        // Vector length = 1
      0x7FU, 0x00U, // Global type
      0xD0U,        // OpCode Ref__null
      0x28U,        // Type index 40
      0x0BU         // Expression End
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));
  EXPECT_TRUE(LdrFuncRef.parseModule(prefixedVec(Vec)));
}
} // namespace
