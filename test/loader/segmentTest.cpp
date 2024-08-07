// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/test/loader/segmentTest.cpp - Load AST segment unit tests ===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contents unit tests of loading AST segment nodes, which are
/// element segment, code segment, data segment, and global segment.
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

TEST(SegmentTest, LoadTableSegment) {
  std::vector<uint8_t> Vec;

  Conf.addProposal(WasmEdge::Proposal::FunctionReferences);
  WasmEdge::Loader::Loader LdrFuncRef(Conf);
  Conf.removeProposal(WasmEdge::Proposal::FunctionReferences);

  // 1. Test load table segment.
  //
  //   1.  Load invalid empty table segment.
  //   2.  Load table segment contains only table type with typed function
  //       reference proposal.
  //   3.  Load table segment contains initialization expression without
  //       typed function reference proposal.
  //   4.  Load table segment contains initialization expression with
  //       typed function reference proposal.
  //   5.  Load table segment in unexpected end of checking byte with
  //       typed function reference proposal.
  //   6.  Load table segment in wrong checking byte with typed function
  //       reference proposal.
  //   7.  Load table segment in unexpected end of table type with
  //       typed function reference proposal.
  //   8.  Load table segment in unexpected end of initialization expression
  //       with typed function reference proposal.

  Vec = {
      0x04U, // Table section
      0x01U, // Content size = 1
      0x01U, // Vector length = 1
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
  EXPECT_TRUE(LdrFuncRef.parseModule(prefixedVec(Vec)));

  Vec = {
      0x04U,                             // Table section
      0x13U,                             // Content size = 19
      0x01U,                             // Vector length = 1
      0x40U, 0x00U,                      // Table segment with init
      0x70U,                             // Reference type
      0x01U,                             // Has min and max
      0xF1U, 0xFFU, 0xFFU, 0xFFU, 0x0FU, // Min = 4294967281
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU, // Max = 4294967295
      0x45U, 0x46U, 0x47U, 0x0BU         // Expression
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  EXPECT_TRUE(LdrFuncRef.parseModule(prefixedVec(Vec)));

  Vec = {
      0x04U, // Table section
      0x02U, // Content size = 2
      0x01U, // Vector length = 1
      0x40U  // Table segment with init
             // 0x00U     Missed checking byte
             // Missed table type and initialization expression
  };
  EXPECT_FALSE(LdrFuncRef.parseModule(prefixedVec(Vec)));

  Vec = {
      0x04U,       // Table section
      0x03U,       // Content size = 3
      0x01U,       // Vector length = 1
      0x40U, 0x01U // Wrong checking byte
                   // Missed table type and initialization expression
  };
  EXPECT_FALSE(LdrFuncRef.parseModule(prefixedVec(Vec)));

  Vec = {
      0x04U,        // Table section
      0x04U,        // Content size = 4
      0x01U,        // Vector length = 1
      0x40U, 0x00U, // Table segment with init
      0x70U         // Reference type
                    // Missed limit and initialization expression
  };
  EXPECT_FALSE(LdrFuncRef.parseModule(prefixedVec(Vec)));

  Vec = {
      0x04U,                             // Table section
      0x12U,                             // Content size = 18
      0x01U,                             // Vector length = 1
      0x40U, 0x00U,                      // Table segment with init
      0x70U,                             // Reference type
      0x01U,                             // Has min and max
      0xF1U, 0xFFU, 0xFFU, 0xFFU, 0x0FU, // Min = 4294967281
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU, // Max = 4294967295
      0x45U, 0x46U, 0x47U                // Expression
                                         // 0x0BU     Missed end of expression
  };
  EXPECT_FALSE(LdrFuncRef.parseModule(prefixedVec(Vec)));
}

TEST(SegmentTest, LoadGlobalSegment) {
  std::vector<uint8_t> Vec;

  // 2. Test load global segment.
  //
  //   1.  Load invalid empty global segment.
  //   2.  Load global segment with expression of only End operation.
  //   3.  Load global segment with non-empty expression.

  Vec = {
      0x06U, // Global section
      0x01U, // Content size = 1
      0x01U, // Vector length = 1
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x06U,       // Global section
      0x04U,       // Content size = 4
      0x01U,       // Vector length = 1
      0x7FU, 0x00, // Global type
      0x0BU        // Expression
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x06U,                     // Global section
      0x07U,                     // Content size = 7
      0x01U,                     // Vector length = 1
      0x7FU, 0x00U,              // Global type
      0x45U, 0x46U, 0x47U, 0x0BU // Expression
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));
}

TEST(SegmentTest, LoadElementSegment) {
  std::vector<uint8_t> Vec;

  Conf.removeProposal(WasmEdge::Proposal::BulkMemoryOperations);
  Conf.removeProposal(WasmEdge::Proposal::ReferenceTypes);
  WasmEdge::Loader::Loader LdrNoRefType(Conf);
  Conf.addProposal(WasmEdge::Proposal::BulkMemoryOperations);
  Conf.addProposal(WasmEdge::Proposal::ReferenceTypes);

  // 3. Test load element segment.
  //
  //   1.  Load invalid empty element segment.
  //   2.  Load element segment with expression of only End operation and empty
  //       function indices list.
  //   3.  Load element segment with expression and function indices list.
  //   4.  Load element segment with invalid checking byte.
  //   5.  Load element segment with invalid checking byte without Ref-Types
  //       proposal.
  //   6.  Load element segment with unexpected end of table index.
  //   7.  Load element segment with unexpected end of offset expression.
  //   8.  Load element segment with invalid element kind.
  //   9.  Load element segment with unexpected end of element kind.
  //   10. Load element segment with unexpected end of initialization vector
  //       count.
  //   11. Load element segment with unexpected end of initialization function
  //       index.
  //   12. Load element segment with unexpected end of reference type.
  //   13. Load element segment with unexpected end of initialization
  //       expression vector count.

  Vec = {
      0x09U, // Element section
      0x01U, // Content size = 1
      0x01U  // Vector length = 1
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x09U, // Element section
      0x04U, // Content size = 4
      0x01U, // Vector length = 1
      0x00U, // Prefix checking byte
      0x0BU, // Offset expression
      0x00U  // Function indices list
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x09U,                             // Element section
      0x0FU,                             // Content size = 15
      0x01U,                             // Vector length = 1
      0x00U,                             // Prefix checking byte
      0x45U, 0x46U, 0x47U, 0x0BU,        // Offset expression
      0x03U,                             // Vector length = 3
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU, // vec[0]
      0x00U,                             // vec[1]
      0xB9U, 0x60U                       // vec[2]
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x09U,                             // Element section
      0x0FU,                             // Content size = 15
      0x01U,                             // Vector length = 1
      0x09U,                             // Prefix invalid checking byte
      0x45U, 0x46U, 0x47U, 0x0BU,        // Offset expression
      0x03U,                             // Vector length = 3
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU, // vec[0]
      0x00U,                             // vec[1]
      0xB9U, 0x60U                       // vec[2]
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x09U,       // Element section
      0x08U,       // Content size = 8
      0x01U,       // Vector length = 1
      0x01U,       // Prefix invalid checking byte without Ref-Types proposal
      0x00U,       // Element kind
      0x03U,       // Vector length = 3
      0x0AU,       // vec[0]
      0x00U,       // vec[1]
      0xB9U, 0x60U // vec[2]
  };
  EXPECT_FALSE(LdrNoRefType.parseModule(prefixedVec(Vec)));

  Vec = {
      0x09U, // Element section
      0x02U, // Content size = 2
      0x01U, // Vector length = 1
      0x02U  // Prefix checking byte
             // Missed table index
             // Missed offset expression
             // Missed element kind
             // Missed initialization vector
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x09U, // Element section
      0x05U, // Content size = 5
      0x01U, // Vector length = 1
      0x00U, // Prefix checking byte
      0x45U, 0x46U,
      0x47U // Offset expression
            // 0x0BU     Missed end of offset expression
            // Missed initialization vector
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x09U,       // Element section
      0x08U,       // Content size = 8
      0x01U,       // Vector length = 1
      0x01U,       // Prefix checking byte
      0x08U,       // Invalid element kind
      0x03U,       // Vector length = 3
      0x0AU,       // vec[0]
      0x00U,       // vec[1]
      0xB9U, 0x60U // vec[2]
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x09U, // Element section
      0x02U, // Content size = 2
      0x01U, // Vector length = 1
      0x01U  // Prefix checking byte
             // Missed element kind
             // Missed initialization vector
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x09U, // Element section
      0x03U, // Content size = 3
      0x01U, // Vector length = 1
      0x01U, // Prefix checking byte
      0x00U  // Element kind
             // Missed initialization vector
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x09U, // Element section
      0x05U, // Content size = 5
      0x01U, // Vector length = 1
      0x01U, // Prefix checking byte
      0x00U, // Element kind
      0x03U, // Vector length = 3
      0x0AU  // vec[0]
             // Missed vec[1] and vec[2]
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x09U, // Element section
      0x02U, // Content size = 2
      0x01U, // Vector length = 1
      0x05U  // Prefix checking byte
             // Missed reference type
             // Missed initialization expressions
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x09U, // Element section
      0x06U, // Content size = 6
      0x01U, // Vector length = 1
      0x04U, // Prefix checking byte of 0x04
      0x45U, 0x46U, 0x47U,
      0x0BU // Offset expression
            // Missed initialization expressions
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));
}

TEST(SegmentTest, LoadCodeSegment) {
  std::vector<uint8_t> Vec;

  Conf.removeProposal(WasmEdge::Proposal::BulkMemoryOperations);
  Conf.removeProposal(WasmEdge::Proposal::ReferenceTypes);
  WasmEdge::Loader::Loader LdrNoRefType(Conf);
  Conf.addProposal(WasmEdge::Proposal::BulkMemoryOperations);
  Conf.addProposal(WasmEdge::Proposal::ReferenceTypes);

  // 4. Test load code segment.
  //
  //   1.  Load invalid empty code segment.
  //   2.  Load invalid code segment of zero content size.
  //   3.  Load code segment of empty locals and expression with only End
  //       operation.
  //   4.  Load code segment with expression and local lists.
  //   5.  Load code segment with unexpected end of local number type.
  //   6.  Load code segment with invalid local number type without Ref-Types
  //       proposal.

  Vec = {
      0x03U, // Function section
      0x02U, // Content size = 2
      0x01U, // Vector length = 1
      0x00U, // Function index vector
      0x0AU, // Code section
      0x01U, // Content size = 1
      0x00U  // Vector length = 0
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x03U, // Function section
      0x02U, // Content size = 2
      0x01U, // Vector length = 1
      0x00U, // Function index vector
      0x0AU, // Code section
      0x02U, // Content size = 2
      0x01U, // Vector length = 1
      0x00U  // Code segment size = 0
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x03U, // Function section
      0x02U, // Content size = 2
      0x01U, // Vector length = 1
      0x00U, // Function index vector
      0x0AU, // Code section
      0x04U, // Content size = 4
      0x01U, // Vector length = 1
      0x02U, // Code segment size = 2
      0x00U, // Vector length = 0
      0x0BU  // Expression
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x03U,                             // Function section
      0x02U,                             // Content size = 2
      0x01U,                             // Vector length = 1
      0x00U,                             // Function index vector
      0x0AU,                             // Code section
      0x15U,                             // Content size = 21
      0x01U,                             // Vector length = 1
      0x13U,                             // Code segment size = 19
      0x04U,                             // Vector length = 4
      0x01U, 0x7CU,                      // vec[0]
      0x03U, 0x7DU,                      // vec[1]
      0xFFU, 0xFFU, 0xFFU, 0x0FU, 0x7EU, // vec[2]
      0xF3U, 0xFFU, 0xFFU, 0x0FU, 0x7FU, // vec[3]
      0x45U, 0x46U, 0x47U, 0x0BU         // Expression
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x03U,        // Function section
      0x02U,        // Content size = 2
      0x01U,        // Vector length = 1
      0x00U,        // Function index vector
      0x0AU,        // Code section
      0x06U,        // Content size = 6
      0x01U,        // Vector length = 1
      0x04U,        // Code segment size = 4
      0x02U,        // Vector length = 2
      0x01U, 0x7CU, // vec[0]
      0x03U         // 0x7DU                    // vec[1], missed value type
                    // 0x45U, 0x46U, 0x0BU      // Missed Expression
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x03U,                             // Function section
      0x02U,                             // Content size = 2
      0x01U,                             // Vector length = 1
      0x00U,                             // Function index vector
      0x0AU,                             // Code section
      0x15U,                             // Content size = 21
      0x01U,                             // Vector length = 1
      0x13U,                             // Code segment size = 19
      0x04U,                             // Vector length = 4
      0x01U, 0x7CU,                      // vec[0]
      0x03U, 0x6FU,                      // vec[1], ExternRef
      0xFFU, 0xFFU, 0xFFU, 0x0FU, 0x7EU, // vec[2]
      0xF3U, 0xFFU, 0xFFU, 0x0FU, 0x7FU, // vec[3]
      0x45U, 0x46U, 0x47U, 0x0BU         // Expression
  };
  EXPECT_FALSE(LdrNoRefType.parseModule(prefixedVec(Vec)));
}

TEST(SegmentTest, LoadDataSegment) {
  std::vector<uint8_t> Vec;

  Conf.removeProposal(WasmEdge::Proposal::BulkMemoryOperations);
  Conf.removeProposal(WasmEdge::Proposal::ReferenceTypes);
  WasmEdge::Loader::Loader LdrNoRefType(Conf);
  Conf.addProposal(WasmEdge::Proposal::BulkMemoryOperations);
  Conf.addProposal(WasmEdge::Proposal::ReferenceTypes);

  // 5. Test load data segment.
  //
  //   1.  Load invalid empty data segment.
  //   2.  Load data segment of expression with only End operation and empty
  //       initialization data.
  //   3.  Load data segment with expression and initialization data.
  //   4.  Load data segment with invalid checking byte.
  //   5.  Load data segment with invalid checking byte without Bulk-Mem
  //       proposal.
  //   6.  Load data segment with unexpected end of memory index.
  //   7.  Load data segment with unexpected end of expression.
  //   8.  Load data segment with unexpected end of initialization data vector.

  Vec = {
      0x0BU, // Data section
      0x01U, // Content size = 1
      0x01U  // Vector length = 1
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0BU, // Data section
      0x04U, // Content size = 4
      0x01U, // Vector length = 1
      0x00U, // Prefix checking byte
      0x0BU, // Expression
      0x00U  // Vector length = 0
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0BU,                            // Data section
      0x0BU,                            // Content size = 11
      0x01U,                            // Vector length = 1
      0x00U,                            // Prefix checking byte
      0x45U, 0x46U, 0x47U, 0x0BU,       // Expression
      0x04U, 0x74U, 0x65U, 0x73U, 0x74U // Vector length = 4, "test"
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0BU,                            // Data section
      0x0BU,                            // Content size = 11
      0x01U,                            // Vector length = 1
      0x06U,                            // Prefix invalid checking byte
      0x45U, 0x46U, 0x47U, 0x0BU,       // Expression
      0x04U, 0x74U, 0x65U, 0x73U, 0x74U // Vector length = 4, "test"
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0BU, // Data section
      0x07U, // Content size = 7
      0x01U, // Vector length = 1
      0x01U, // Prefix invalid checking byte without Bulk-Mem proposal
      0x04U, 0x74U, 0x65U, 0x73U, 0x74U // Vector length = 4, "test"
  };
  EXPECT_FALSE(LdrNoRefType.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0BU, // Data section
      0x02U, // Content size = 2
      0x01U, // Vector length = 1
      0x02U  // Prefix checking byte
             // Missed memory index, offset expression, and initialization data.
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0BU, // Data section
      0x04U, // Content size = 4
      0x01U, // Vector length = 1
      0x02U, // Prefix checking byte
      0x45U,
      0x46U // Missed end of expression
            // Missed initialization data
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0BU, // Data section
      0x05U, // Content size = 5
      0x01U, // Vector length = 1
      0x02U, // Prefix checking byte
      0x45U, 0x46U,
      0x0BU // Expression
            // Missed initialization data
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));
}
} // namespace
