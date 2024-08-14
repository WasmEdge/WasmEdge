// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "loader/serialize.h"

#include <cstdint>
#include <gtest/gtest.h>
#include <vector>

namespace {

WasmEdge::Configure Conf;
WasmEdge::Loader::Serializer Ser(Conf);

WasmEdge::AST::ImportSection createImportSec(WasmEdge::AST::ImportDesc &Desc) {
  WasmEdge::AST::ImportSection ImportSec;
  ImportSec.getContent().push_back(Desc);
  return ImportSec;
}

WasmEdge::AST::ExportSection createExportSec(WasmEdge::AST::ExportDesc &Desc) {
  WasmEdge::AST::ExportSection ExportSec;
  ExportSec.getContent().push_back(Desc);
  return ExportSec;
}

TEST(SerializeDescriptionTest, SerializeImportDesc) {
  WasmEdge::AST::ImportDesc Desc;
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;

  WasmEdge::Configure ConfNoImpMutGlob;
  ConfNoImpMutGlob.removeProposal(WasmEdge::Proposal::ImportExportMutGlobals);
  WasmEdge::Loader::Serializer SerNoImpMutGlob(ConfNoImpMutGlob);

  // 1. Test serialize import description.
  //
  //   1.  Serialize import description with empty module and external name.
  //   2.  Serialize import description with module and external names.
  //   3.  Serialize import description of table type.
  //   4.  Serialize import description of memory type.
  //   5.  Serialize import description of global type.
  //   6.  Serialize invalid import description of global type without
  //       Mut-Globals proposal.

  Desc.setModuleName("");
  Desc.setExternalName("");
  Desc.setExternalType(WasmEdge::ExternalType::Function);
  Desc.setExternalFuncTypeIdx(0x00U);

  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createImportSec(Desc), Output));
  Expected = {
      0x02U,       // Import section
      0x05U,       // Content size = 5
      0x01U,       // Vector length = 1
      0x00U,       // Empty module name
      0x00U,       // Empty external name
      0x00U, 0x00U // function type and index
  };
  EXPECT_EQ(Output, Expected);

  Desc.setModuleName("test");
  Desc.setExternalName("Loader");
  Desc.setExternalType(WasmEdge::ExternalType::Function);
  Desc.setExternalFuncTypeIdx(0x00U);

  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createImportSec(Desc), Output));
  Expected = {
      0x02U,                                           // Import section
      0x0FU,                                           // Content size = 15
      0x01U,                                           // Vector length = 1
      0x04U, 0x74U, 0x65U, 0x73U, 0x74U,               // Module name: test
      0x06U, 0x4CU, 0x6FU, 0x61U, 0x64U, 0x65U, 0x72U, // External name: Loader
      0x00U, 0x00U // function type and index
  };
  EXPECT_EQ(Output, Expected);

  Desc.setExternalType(WasmEdge::ExternalType::Table);
  Desc.getExternalTableType().setRefType(WasmEdge::TypeCode::FuncRef);
  Desc.getExternalTableType().getLimit().setMin(4294967281);
  Desc.getExternalTableType().getLimit().setMax(4294967295);
  Desc.getExternalTableType().getLimit().setType(
      WasmEdge::AST::Limit::LimitType::HasMinMax);

  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createImportSec(Desc), Output));
  Expected = {
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
  EXPECT_EQ(Output, Expected);

  Desc.setExternalType(WasmEdge::ExternalType::Memory);
  Desc.getExternalMemoryType().getLimit().setMin(4294967281);
  Desc.getExternalMemoryType().getLimit().setMax(4294967295);
  Desc.getExternalMemoryType().getLimit().setType(
      WasmEdge::AST::Limit::LimitType::HasMinMax);

  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createImportSec(Desc), Output));
  Expected = {
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
  EXPECT_EQ(Output, Expected);

  Desc.setExternalType(WasmEdge::ExternalType::Global);
  Desc.getExternalGlobalType().setValType(WasmEdge::TypeCode::F64);
  Desc.getExternalGlobalType().setValMut(WasmEdge::ValMut::Const);

  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createImportSec(Desc), Output));
  Expected = {
      0x02U,                                           // Import section
      0x10U,                                           // Content size = 16
      0x01U,                                           // Vector length = 1
      0x04U, 0x74U, 0x65U, 0x73U, 0x74U,               // Module name: test
      0x06U, 0x4CU, 0x6FU, 0x61U, 0x64U, 0x65U, 0x72U, // External name: Loader
      0x03U,                                           // Global type
      0x7CU, 0x00U                                     // Const F64 number type
  };
  EXPECT_EQ(Output, Expected);

  Desc.getExternalGlobalType().setValMut(WasmEdge::ValMut::Var);
  EXPECT_FALSE(SerNoImpMutGlob.serializeSection(createImportSec(Desc), Output));
}

TEST(SerializeDescriptionTest, SerializeExportDesc) {
  WasmEdge::AST::ExportDesc Desc;
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;

  // 2. Test serialize export description.
  //
  //   1.  Serialize export description with empty module name.
  //   2.  Serialize export description with non-empty module name.
  //   3.  Serialize export description of table type.

  Desc.setExternalName("");
  Desc.setExternalType(WasmEdge::ExternalType::Function);
  Desc.setExternalIndex(0x00U);

  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createExportSec(Desc), Output));
  Expected = {
      0x07U,       // Export section
      0x04U,       // Content size = 4
      0x01U,       // Vector length = 1
      0x00U,       // Empty external name
      0x00U, 0x00U // function type and index
  };
  EXPECT_EQ(Output, Expected);

  Desc.setExternalName("Loader");
  Desc.setExternalType(WasmEdge::ExternalType::Function);
  Desc.setExternalIndex(0x00U);

  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createExportSec(Desc), Output));
  Expected = {
      0x07U,                                           // Export section
      0x0AU,                                           // Content size = 10
      0x01U,                                           // Vector length = 1
      0x06U, 0x4CU, 0x6FU, 0x61U, 0x64U, 0x65U, 0x72U, // External name: Loader
      0x00U, 0x00U // function type and index
  };
  EXPECT_EQ(Output, Expected);

  Desc.setExternalName("Loader");
  Desc.setExternalType(WasmEdge::ExternalType::Table);
  Desc.setExternalIndex(0xFFFFFFFFU);

  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createExportSec(Desc), Output));
  Expected = {
      0x07U,                                           // Export section
      0x0EU,                                           // Content size = 14
      0x01U,                                           // Vector length = 1
      0x06U, 0x4CU, 0x6FU, 0x61U, 0x64U, 0x65U, 0x72U, // External name: Loader
      0x01U, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU // Table type and table index
  };
  EXPECT_EQ(Output, Expected);
}
} // namespace
