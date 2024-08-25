// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/test/loader/sectionTest.cpp - Load AST section unit tests ===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contents unit tests of loading AST section nodes.
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

TEST(SectionTest, LoadCustomSection) {
  std::vector<uint8_t> Vec;

  // 1. Test load custom section.
  //
  //   1.  Load invalid empty section.
  //   2.  Load invalid custom section without contents.
  //   3.  Load custom section with 0-length name.
  //   4.  Load custom section with contents.

  Vec = {0x00U};
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x00U, // Custom section
      0x00U  // Content size = 0
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x00U, // Custom section
      0x01U, // Content size = 1
      0x00U  // Name length = 0
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x00U,                                   // Custom section
      0x07U,                                   // Content size = 7
      0x00U,                                   // Name length = 0
      0xFFU, 0xEEU, 0xDDU, 0xCCU, 0xBBU, 0xAAU // Content
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));
}

TEST(SectionTest, LoadTypeSection) {
  std::vector<uint8_t> Vec;

  // 2. Test load type section.
  //
  //   1.  Load invalid empty section.
  //   2.  Load type section without contents.
  //   3.  Load type section with zero vector length.
  //   4.  Load type section with contents.

  Vec = {0x01U};
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x01U, // Type section
      0x00U  // Content size = 0
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x01U, // Type section
      0x01U, // Content size = 1
      0x00U  // Vector length = 0
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x01U,                                    // Type section
      0x13U,                                    // Content size = 19
      0x03U,                                    // Vector length = 3
      0x60U, 0x02U, 0x7CU, 0x7DU, 0x01U, 0x7CU, // vec[0]
      0x60U, 0x02U, 0x7DU, 0x7EU, 0x01U, 0x7DU, // vec[1]
      0x60U, 0x02U, 0x7EU, 0x7FU, 0x01U, 0x7EU  // vec[2]
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));
}

TEST(SectionTest, LoadImportSection) {
  std::vector<uint8_t> Vec;

  // 3. Test load import section.
  //
  //   1.  Load invalid empty section.
  //   2.  Load import section without contents.
  //   3.  Load import section with zero vector length.
  //   4.  Load import section with contents.

  Vec = {0x02U};
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x02U, // Import section
      0x00U  // Content size = 0
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x02U, // Import section
      0x01U, // Content size = 1
      0x00U  // Vector length = 0
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x02U, // Import section
      0x2EU, // Content size = 46
      0x03U, // Vector length = 3
      // vec[0]
      0x04U, 0x74U, 0x65U, 0x73U, 0x74U,               // ModName: "test"
      0x06U, 0x4CU, 0x6FU, 0x61U, 0x64U, 0x65U, 0x72U, // ExtName: "Loader"
      0x00U, 0x00U,                                    // function type
      // vec[1]
      0x04U, 0x74U, 0x65U, 0x73U, 0x74U,               // ModName: "test"
      0x06U, 0x4CU, 0x6FU, 0x61U, 0x64U, 0x65U, 0x72U, // ExtName: "Loader"
      0x02U, 0x01U, 0x00U, 0x0FU,                      // Memory type
      // vec[2]
      0x04U, 0x74U, 0x65U, 0x73U, 0x74U,               // ModName: "test"
      0x06U, 0x4CU, 0x6FU, 0x61U, 0x64U, 0x65U, 0x72U, // ExtName: "Loader"
      0x03U, 0x7CU, 0x00U                              // Global type
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));
}

TEST(SectionTest, LoadFunctionSection) {
  std::vector<uint8_t> Vec;

  // 4. Test load function section.
  //
  //   1.  Load invalid empty section.
  //   2.  Load function section without contents.
  //   3.  Load function section with zero vector length.
  //   4.  Load function section with contents.
  //   5.  Load function section with contents not match section size.

  Vec = {0x03U};
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x03U, // Function section
      0x00U  // Content size = 0
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x03U, // Function section
      0x01U, // Content size = 1
      0x00U  // Vector length = 0
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x03U,                             // Function section
      0x09U,                             // Content size = 9
      0x03U,                             // Vector length = 3
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU, // vec[0]
      0x00U,                             // vec[1]
      0xB9U, 0x60U,                      // vec[2]
      0x0AU,                             // Code section
      0x1FU,                             // Content size = 31
      0x03U,                             // Vector length = 3
      0x09U,                             // Code segment size = 9
      0x02U, 0x01U, 0x7CU, 0x02U, 0x7DU, // Local vec(2)
      0x45U, 0x46U, 0x47U, 0x0BU,        // Expression
      0x09U,                             // Code segment size = 9
      0x02U, 0x03U, 0x7CU, 0x04U, 0x7DU, // Local vec(2)
      0x45U, 0x46U, 0x47U, 0x0BU,        // Expression
      0x09U,                             // Code segment size = 9
      0x02U, 0x05U, 0x7CU, 0x06U, 0x7DU, // Local vec(2)
      0x45U, 0x46U, 0x47U, 0x0BU         // Expression
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x03U,                             // Function section
      0x09U,                             // Content size = 9
      0x02U,                             // Vector length = 2
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU, // vec[0]
      0x00U,                             // vec[1]
      0xB9U, 0x60U                       // redundant vec[2]
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));
}

TEST(SectionTest, LoadTableSection) {
  std::vector<uint8_t> Vec;

  // 5. Test load table section.
  //
  //   1.  Load invalid empty section.
  //   2.  Load table section without contents.
  //   3.  Load table section with zero vector length.
  //   4.  Load table section with contents.

  Vec = {0x04U};
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x04U, // Table section
      0x00U  // Content size = 0
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x04U, // Table section
      0x01U, // Content size = 1
      0x00U  // Vector length = 0
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x04U,                      // Table section
      0x0DU,                      // Content size = 13
      0x03U,                      // Vector length = 3
      0x70U, 0x01U, 0x00U, 0x0FU, // vec[0]
      0x70U, 0x01U, 0x00U, 0x0EU, // vec[1]
      0x70U, 0x01U, 0x00U, 0x0DU  // vec[2]
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));
}

TEST(SectionTest, LoadMemorySection) {
  std::vector<uint8_t> Vec;

  // 6. Test load memory section.
  //
  //   1.  Load invalid empty section.
  //   2.  Load memory section without contents.
  //   3.  Load memory section with zero vector length.
  //   4.  Load memory section with contents.

  Vec = {0x05U};
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x05U, // Memory section
      0x00U  // Content size = 0
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x05U, // Memory section
      0x01U, // Content size = 1
      0x00U  // Vector length = 0
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x05U,               // Memory section
      0x0AU,               // Content size = 10
      0x03U,               // Vector length = 3
      0x01U, 0x00U, 0x0FU, // vec[0]
      0x01U, 0x00U, 0x0EU, // vec[1]
      0x01U, 0x00U, 0x0DU  // vec[2]
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));
}

TEST(SectionTest, LoadGlobalSection) {
  std::vector<uint8_t> Vec;

  // 7. Test load global section.
  //
  //   1.  Load invalid empty section.
  //   2.  Load global section without contents.
  //   3.  Load global section with zero vector length.
  //   4.  Load global section with contents.

  Vec = {0x06U};
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x06U, // Global section
      0x00U  // Content size = 0
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x06U, // Global section
      0x01U, // Content size = 1
      0x00U  // Vector length = 0
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x06U,                            // Global section
      0x0DU,                            // Content size = 13
      0x03U,                            // Vector length = 3
      0x7CU, 0x00U, 0x0BU,              // vec[0]
      0x7DU, 0x00U, 0x45U, 0x0BU,       // vec[1]
      0x7EU, 0x01U, 0x46U, 0x47U, 0x0BU // vec[2]
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));
}

TEST(SectionTest, LoadExportSection) {
  std::vector<uint8_t> Vec;

  // 8. Test load export section.
  //
  //   1.  Load invalid empty section.
  //   2.  Load export section without contents.
  //   3.  Load export section with zero vector length.
  //   4.  Load export section with contents.

  Vec = {0x07U};
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x07U, // Export section
      0x00U  // Content size = 0
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x07U, // Export section
      0x01U, // Content size = 1
      0x00U  // Vector length = 0
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x07U, // Export section
      0x28U, // Content size = 40
      0x03U, // Vector length = 3
      // vec[0]
      0x06U, 0x4CU, 0x6FU, 0x61U, 0x64U, 0x65U, 0x72U, // ExtName: Loader
      0x00U, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU,        // function type and idx
      // vec[1]
      0x06U, 0x4CU, 0x6FU, 0x61U, 0x64U, 0x65U, 0x72U, // ExtName: Loader
      0x01U, 0xFDU, 0xFFU, 0xFFU, 0xFFU, 0x0FU,        // Table type and idx
      // vec[2]
      0x06U, 0x4CU, 0x6FU, 0x61U, 0x64U, 0x65U, 0x72U, // ExtName: Loader
      0x02U, 0xFBU, 0xFFU, 0xFFU, 0xFFU, 0x0FU         // Memory type and idx
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));
}

TEST(SectionTest, LoadStartSection) {
  std::vector<uint8_t> Vec;

  // 9. Test load start section.
  //
  //   1.  Load invalid empty section.
  //   2.  Load start section without contents.
  //   3.  Load start section with contents.
  //   4.  Load start section with contents not match section size.

  Vec = {0x08U};
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x08U, // Start section
      0x00U  // Content size = 0
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x08U,                            // Start section
      0x05U,                            // Content size = 5
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU // Content
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x08U,              // Start section
      0x05U,              // Content size = 5
      0xFFU, 0xFFU, 0x0FU // Content
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));
}

TEST(SectionTest, LoadElementSection) {
  std::vector<uint8_t> Vec;

  // 10. Test load element section.
  //
  //   1.  Load invalid empty section.
  //   2.  Load element section without contents.
  //   3.  Load element section with zero vector length.
  //   4.  Load element section with contents.

  Vec = {0x09U};
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x09U, // Element section
      0x00U  // Content size = 0
  };

  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x09U, // Element section
      0x01U, // Content size = 1
      0x00U  // Vector length = 0
  };

  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x09U, // Element section
      0x1CU, // Content size = 28
      0x03U, // Vector length = 3
      // vec[0]
      0x00U,                      // Prefix 0x00
      0x45U, 0x46U, 0x47U, 0x0BU, // Expression
      0x03U, 0x00U, 0x0AU, 0x0FU, // Vec(3)
      // vec[1]
      0x00U,                      // Prefix 0x00
      0x45U, 0x46U, 0x47U, 0x0BU, // Expression
      0x03U, 0x0AU, 0x0BU, 0x0CU, // Vec(3)
      // vec[2]
      0x00U,                      // Prefix 0x00
      0x45U, 0x46U, 0x47U, 0x0BU, // Expression
      0x03U, 0x03U, 0x06U, 0x09U  // Vec(3)
  };

  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));
}

TEST(SectionTest, LoadCodeSection) {
  std::vector<uint8_t> Vec;

  // 11. Test load code section.
  //
  //   1.  Load invalid empty section.
  //   2.  Load code section without contents.
  //   3.  Load code section with zero vector length.
  //   4.  Load code section with contents.

  Vec = {0x0AU};
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU, // Code section
      0x00U  // Content size = 0
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0AU, // Code section
      0x01U, // Content size = 1
      0x00U  // Vector length = 0
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x03U,                             // Function section
      0x09U,                             // Content size = 9
      0x03U,                             // Vector length = 3
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU, // vec[0]
      0x00U,                             // vec[1]
      0xB9U, 0x60U,                      // vec[2]
      0x0AU,                             // Code section
      0x1FU,                             // Content size = 31
      0x03U,                             // Vector length = 3
      // vec[0]
      0x09U,                             // Code segment size = 9
      0x02U, 0x01U, 0x7CU, 0x02U, 0x7DU, // Local vec(2)
      0x45U, 0x46U, 0x47U, 0x0BU,        // Expression
      // vec[1]
      0x09U,                             // Code segment size = 9
      0x02U, 0x03U, 0x7CU, 0x04U, 0x7DU, // Local vec(2)
      0x45U, 0x46U, 0x47U, 0x0BU,        // Expression
      // vec[2]
      0x09U,                             // Code segment size = 9
      0x02U, 0x05U, 0x7CU, 0x06U, 0x7DU, // Local vec(2)
      0x45U, 0x46U, 0x47U, 0x0BU         // Expression
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));
}

TEST(SectionTest, LoadDataSection) {
  std::vector<uint8_t> Vec;

  // 12. Test load data section.
  //
  //   1.  Load invalid empty section.
  //   2.  Load data section without contents.
  //   3.  Load data section with zero vector length.
  //   4.  Load data section with contents.

  Vec = {0x0BU};
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0BU, // Data section
      0x00U  // Content size = 0
  };

  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0BU, // Data section
      0x01U, // Content size = 1
      0x00U  // Vector length = 0
  };

  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0BU, // Data section
      0x20U, // Content size = 32
      0x03U, // Vector length = 3
      // vec[0]
      0x00U,                             // Prefix 0x00
      0x45U, 0x46U, 0x47U, 0x0BU,        // Expression
      0x04U, 0x74U, 0x65U, 0x73U, 0x74U, // Vector length = 4, "test"
      // vec[1]
      0x01U,                             // Prefix 0x01
      0x04U, 0x74U, 0x65U, 0x73U, 0x74U, // Vector length = 4, "test"
      // vec[2]
      0x02U,                             // Prefix 0x02
      0xF0U, 0xFFU, 0xFFU, 0xFFU, 0x0FU, // Memory index
      0x45U, 0x46U, 0x47U, 0x0BU,        // Expression
      0x04U, 0x74U, 0x65U, 0x73U, 0x74U  // Vector length = 4, "test"
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));
}

TEST(SectionTest, LoadDataCountSection) {
  std::vector<uint8_t> Vec;

  Conf.removeProposal(WasmEdge::Proposal::BulkMemoryOperations);
  Conf.removeProposal(WasmEdge::Proposal::ReferenceTypes);
  WasmEdge::Loader::Loader LdrNoRefType(Conf);

  // 13. Test load datacount section.
  //
  //   1.  Load invalid empty section.
  //   2.  Load datacount section without contents.
  //   3.  Load datacount section with contents.
  //   4.  Load datacount section with contents not match section size.
  //   5.  Load datacount section without Ref-Types proposal.

  Vec = {0x0CU};
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0CU, // Datacount section
      0x00U  // Content size = 0
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0BU,                             // Data section
      0x0BU,                             // Content size = 11
      0x01U,                             // Vector length = 1
      0x00U,                             // Prefix 0x00
      0x45U, 0x46U, 0x47U, 0x0BU,        // Expression
      0x04U, 0x74U, 0x65U, 0x73U, 0x74U, // Vector length = 4, "test"
      0x0CU,                             // Datacount section
      0x01U,                             // Content size = 1
      0x01U                              // Content
  };
  EXPECT_TRUE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0CU,              // Datacount section
      0x05U,              // Content size = 5
      0xFFU, 0xFFU, 0x0FU // Content
  };
  EXPECT_FALSE(Ldr.parseModule(prefixedVec(Vec)));

  Vec = {
      0x0CU, // Datacount section
      0x00U, // Content size = 0
  };
  EXPECT_FALSE(LdrNoRefType.parseModule(prefixedVec(Vec)));
}
} // namespace
