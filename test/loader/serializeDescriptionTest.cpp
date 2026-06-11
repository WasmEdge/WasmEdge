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
  ConfNoImpMutGlob.setWASMStandard(WasmEdge::Standard::WASM_1);
  ConfNoImpMutGlob.removeProposal(WasmEdge::Proposal::ImportExportMutGlobals);
  WasmEdge::Loader::Serializer SerNoImpMutGlob(ConfNoImpMutGlob);
  WasmEdge::Configure ConfWASM2;
  ConfWASM2.setWASMStandard(WasmEdge::Standard::WASM_2);
  WasmEdge::Loader::Serializer SerWASM2(ConfWASM2);

  Desc.setModuleName("");
  Desc.setExternalName("");
  Desc.setExternalType(WasmEdge::ExternalType::Function);
  Desc.setExternalFuncTypeIdx(0x00U);

  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createImportSec(Desc), Output));
  Expected = {
      0x02U,
      0x05U,
      0x01U,
      0x00U,
      0x00U,
      0x00U, 0x00U
  };
  EXPECT_EQ(Output, Expected);

  Desc.setModuleName("test");
  Desc.setExternalName("Loader");
  Desc.setExternalType(WasmEdge::ExternalType::Function);
  Desc.setExternalFuncTypeIdx(0x00U);

  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createImportSec(Desc), Output));
  Expected = {
      0x02U,
      0x0FU,
      0x01U,
      0x04U, 0x74U, 0x65U, 0x73U, 0x74U,
      0x06U, 0x4CU, 0x6FU, 0x61U, 0x64U, 0x65U, 0x72U,
      0x00U, 0x00U
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
      0x02U,
      0x1AU,
      0x01U,
      0x04U, 0x74U, 0x65U, 0x73U, 0x74U,
      0x06U, 0x4CU, 0x6FU, 0x61U, 0x64U, 0x65U, 0x72U,
      0x01U,
      0x70U,
      0x01U,
      0xF1U, 0xFFU, 0xFFU, 0xFFU, 0x0FU,
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU
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
      0x02U,
      0x19U,
      0x01U,
      0x04U, 0x74U, 0x65U, 0x73U, 0x74U,
      0x06U, 0x4CU, 0x6FU, 0x61U, 0x64U, 0x65U, 0x72U,
      0x02U,
      0x01U,
      0xF1U, 0xFFU, 0xFFU, 0xFFU, 0x0FU,
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU
  };
  EXPECT_EQ(Output, Expected);

  Desc.setExternalType(WasmEdge::ExternalType::Global);
  Desc.getExternalGlobalType().setValType(WasmEdge::TypeCode::F64);
  Desc.getExternalGlobalType().setValMut(WasmEdge::ValMut::Const);

  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createImportSec(Desc), Output));
  Expected = {
      0x02U,
      0x10U,
      0x01U,
      0x04U, 0x74U, 0x65U, 0x73U, 0x74U,
      0x06U, 0x4CU, 0x6FU, 0x61U, 0x64U, 0x65U, 0x72U,
      0x03U,
      0x7CU, 0x00U
  };
  EXPECT_EQ(Output, Expected);

  Desc.getExternalGlobalType().setValMut(WasmEdge::ValMut::Var);
  EXPECT_FALSE(SerNoImpMutGlob.serializeSection(createImportSec(Desc), Output));

  Desc.setExternalType(WasmEdge::ExternalType::Tag);
  Desc.getExternalTagType().setTypeIdx(0x02U);
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createImportSec(Desc), Output));
  Expected = {
      0x02U,
      0x10U,
      0x01U,
      0x04U, 0x74U, 0x65U, 0x73U, 0x74U,
      0x06U, 0x4CU, 0x6FU, 0x61U, 0x64U, 0x65U, 0x72U,
      0x04U,
      0x00U, 0x02U
  };
  EXPECT_EQ(Expected, Output);

  Output = {};
  EXPECT_FALSE(SerWASM2.serializeSection(createImportSec(Desc), Output));
}

TEST(SerializeDescriptionTest, SerializeExportDesc) {
  WasmEdge::AST::ExportDesc Desc;
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;

  WasmEdge::Configure ConfWASM2;
  ConfWASM2.setWASMStandard(WasmEdge::Standard::WASM_2);
  WasmEdge::Loader::Serializer SerWASM2(ConfWASM2);

  Desc.setExternalName("");
  Desc.setExternalType(WasmEdge::ExternalType::Function);
  Desc.setExternalIndex(0x00U);

  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createExportSec(Desc), Output));
  Expected = {
      0x07U,
      0x04U,
      0x01U,
      0x00U,
      0x00U, 0x00U
  };
  EXPECT_EQ(Output, Expected);

  Desc.setExternalName("Loader");
  Desc.setExternalType(WasmEdge::ExternalType::Function);
  Desc.setExternalIndex(0x00U);

  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createExportSec(Desc), Output));
  Expected = {
      0x07U,
      0x0AU,
      0x01U,
      0x06U, 0x4CU, 0x6FU, 0x61U, 0x64U, 0x65U, 0x72U,
      0x00U, 0x00U
  };
  EXPECT_EQ(Output, Expected);

  Desc.setExternalName("Loader");
  Desc.setExternalType(WasmEdge::ExternalType::Table);
  Desc.setExternalIndex(0xFFFFFFFFU);

  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createExportSec(Desc), Output));
  Expected = {
      0x07U,
      0x0EU,
      0x01U,
      0x06U, 0x4CU, 0x6FU, 0x61U, 0x64U, 0x65U, 0x72U,
      0x01U, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU
  };
  EXPECT_EQ(Output, Expected);

  Desc.setExternalType(WasmEdge::ExternalType::Tag);
  Desc.setExternalIndex(0x02U);
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createExportSec(Desc), Output));
  Expected = {
      0x07U,
      0x0AU,
      0x01U,
      0x06U, 0x4CU, 0x6FU, 0x61U, 0x64U, 0x65U, 0x72U,
      0x04U,
      0x02U
  };
  EXPECT_EQ(Expected, Output);

  Output = {};
  EXPECT_FALSE(SerWASM2.serializeSection(createExportSec(Desc), Output));
}
} // namespace

