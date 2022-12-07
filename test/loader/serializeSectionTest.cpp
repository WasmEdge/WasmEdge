/// Unit tests of serialize sections

#include "loader/loader.h"
#include "loader/serialize.h"

#include <cstdint>
#include <gtest/gtest.h>
#include <vector>

namespace {

WasmEdge::Serialize::Serialize Ser;

TEST(SerializeSectionTest, SerializeCustomSection) {

  WasmEdge::AST::CustomSection CustomSec;
  CustomSec.setName("name");
  CustomSec.getContent() = {0x01U, 0x02U, 0x03U, 0x04U, 0x05U};

  std::vector<uint8_t> Output = Ser.serializeCustomSection(CustomSec);
  std::vector<uint8_t> Expected = {
      0x00U,                            // Section ID
      0x0BU,                            // Content size = 11
      0x04U,                            // Name length = 4
      0x6EU, 0x61U, 0x6D,  0x65,        // Name
      0x05U,                            // Vec size = 5
      0x01U, 0x02U, 0x03U, 0x04U, 0x05U // Vec content
  };
  EXPECT_EQ(Output, Expected);
}

TEST(SerializeSectionTest, SerializeTypeSection) {

  WasmEdge::AST::TypeSection TypeSec;
  WasmEdge::AST::FunctionType FT1;
  WasmEdge::ValType Type = static_cast<WasmEdge::ValType>(0x07);
  FT1.getParamTypes() = {Type, Type};
  FT1.getReturnTypes() = {};

  WasmEdge::AST::FunctionType FT2;
  FT2.getParamTypes() = {Type, Type};
  FT2.getReturnTypes() = {Type};

  WasmEdge::AST::FunctionType FT3;
  FT3.getParamTypes() = {};
  FT3.getReturnTypes() = {Type};

  TypeSec.getContent().push_back(FT1);
  TypeSec.getContent().push_back(FT2);
  TypeSec.getContent().push_back(FT3);

  std::vector<uint8_t> Output = Ser.serializeTypeSection(TypeSec);
  std::vector<uint8_t> Expected = {
      0x01U,                                    // section ID
      0x10U,                                    // Content size = 16
      0x03U,                                    // Vector length = 3
      0x60U, 0x02U, 0x07U, 0x07U, 0x00U,        // vec[0]
      0x60U, 0x02U, 0x07U, 0x07U, 0x01U, 0x07U, // vec[1]
      0x60U, 0x00U, 0x01U, 0x07U                // vec[2]
  };

  EXPECT_EQ(Output, Expected);
}

TEST(SerializeSectionTest, SerializeImportSection) {

  WasmEdge::AST::ImportDesc ID1;
  ID1.setModuleName("test");
  ID1.setExternalName("Loader");
  WasmEdge::ExternalType ExtType1 = static_cast<WasmEdge::ExternalType>(0x00U);
  ID1.setExternalType(ExtType1);
  ID1.setExternalFuncTypeIdx(0x01U);

  WasmEdge::AST::ImportDesc ID2;
  ID2.setModuleName("test");
  ID2.setExternalName("Loader");
  WasmEdge::ExternalType ExtType2 = static_cast<WasmEdge::ExternalType>(0x02U);
  ID2.setExternalType(ExtType2);
  ID2.getExternalMemoryType().getLimit().setMin(0);
  ID2.getExternalMemoryType().getLimit().setMax(15);

  WasmEdge::AST::ImportDesc ID3;
  ID3.setModuleName("test");
  ID3.setExternalName("Loader");
  WasmEdge::ExternalType ExtType3 = static_cast<WasmEdge::ExternalType>(0x03U);
  ID3.setExternalType(ExtType3);
  WasmEdge::ValType valtype = static_cast<WasmEdge::ValType>(0x7CU);
  WasmEdge::ValMut valmut = static_cast<WasmEdge::ValMut>(0x00);
  ID3.getExternalGlobalType().setValType(valtype);
  ID3.getExternalGlobalType().setValMut(valmut);

  WasmEdge::AST::ImportSection ImpSec;
  ImpSec.getContent().push_back(ID1);
  ImpSec.getContent().push_back(ID2);
  ImpSec.getContent().push_back(ID3);

  std::vector<uint8_t> Output = Ser.serializeImportSection(ImpSec);
  std::vector<uint8_t> Expected = {
      0x02U, // section ID
      0x2EU, // Content size = 46
      0x03U, // Vector length = 3
      // vec[0]
      0x04U, 0x74U, 0x65U, 0x73U, 0x74U,               // ModName: "test"
      0x06U, 0x4CU, 0x6FU, 0x61U, 0x64U, 0x65U, 0x72U, // ExtName: "Loader"
      0x00U, 0x01U,                                    // function type
      // vec[1]
      0x04U, 0x74U, 0x65U, 0x73U, 0x74U,               // ModName: "test"
      0x06U, 0x4CU, 0x6FU, 0x61U, 0x64U, 0x65U, 0x72U, // ExtName: "Loader"
      0x02U, 0x01U, 0x00U, 0x0FU,                      // Memory type
      // vec[2]
      0x04U, 0x74U, 0x65U, 0x73U, 0x74U,               // ModName: "test"
      0x06U, 0x4CU, 0x6FU, 0x61U, 0x64U, 0x65U, 0x72U, // ExtName: "Loader"
      0x03U, 0x7CU, 0x00U                              // Global type
  };
  EXPECT_EQ(Output, Expected);
}

TEST(SerializeSectionTest, SerializeFunctionSection) {

  WasmEdge::AST::FunctionSection FuncSec;
  FuncSec.getContent() = {1, 2, 1, 1};

  std::vector<uint8_t> Output = Ser.serializeFunctionSection(FuncSec);

  std::vector<uint8_t> Expected = {
      0x03U,                     // section ID
      0x05U,                     // Content size = 5
      0x04U,                     // Vector length = 4
      0x01U, 0x02U, 0x01U, 0x01U // vec[0]
  };
  EXPECT_EQ(Output, Expected);
}

TEST(SerializeSectionTest, SerializeTableSection) {

  WasmEdge::AST::TableSection TableSec;
  WasmEdge::RefType RefType1 = static_cast<WasmEdge::RefType>(0x70U);
  WasmEdge::AST::TableType TT1;
  TT1.setRefType(RefType1);
  TT1.getLimit().setMax(0);
  TT1.getLimit().setMax(15);

  WasmEdge::AST::TableType TT2;
  TT2.setRefType(RefType1);
  TT2.getLimit().setMax(0);
  TT2.getLimit().setMax(14);

  TableSec.getContent().push_back(TT1);
  TableSec.getContent().push_back(TT2);

  std::vector<uint8_t> Output = Ser.serializeTableSection(TableSec);

  std::vector<uint8_t> Expected = {
      0x04U,                      // section ID
      0x09U,                      // Content size = 9
      0x02U,                      // Vector length = 2
      0x70U, 0x01U, 0x00U, 0x0FU, // vec[0]
      0x70U, 0x01U, 0x00U, 0x0EU, // vec[1]
  };
  EXPECT_EQ(Output, Expected);
}

TEST(SerializeSectionTest, SerializeMemorySection) {
  WasmEdge::AST::MemorySection MemSec;
  WasmEdge::AST::MemoryType MT1;
  MT1.getLimit().setMin(0x00U);
  MT1.getLimit().setMax(0x0FU);

  WasmEdge::AST::MemoryType MT2;
  MT2.getLimit().setMin(0x00U);
  MT2.getLimit().setMax(0x0E);

  WasmEdge::AST::MemoryType MT3;
  MT3.getLimit().setMin(0x00U);
  MT3.getLimit().setMax(0x0D);

  MemSec.getContent().push_back(MT1);
  MemSec.getContent().push_back(MT2);
  MemSec.getContent().push_back(MT3);

  std::vector<uint8_t> Output = Ser.serializeMemorySection(MemSec);

  std::vector<uint8_t> Expected = {
      0x05U,               // section ID
      0x0AU,               // Content size = 10
      0x03U,               // Vector length = 3
      0x01U, 0x00U, 0x0FU, // vec[0]
      0x01U, 0x00U, 0x0EU, // vec[1]
      0x01U, 0x00U, 0x0DU  // vec[2]
  };
  EXPECT_EQ(Output, Expected);
}

TEST(SerializeSectionTest, SerializeExportSection) {

  WasmEdge::AST::ExportSection ExpSec;
  WasmEdge::AST::ExportDesc ED1;
  WasmEdge::ExternalType ExtType1 = static_cast<WasmEdge::ExternalType>(0x00U);
  ED1.setExternalType(ExtType1);
  ED1.setExternalName("Loader");
  ED1.setExternalIndex(145);

  WasmEdge::ExternalType ExtType2 = static_cast<WasmEdge::ExternalType>(0x01U);
  WasmEdge::AST::ExportDesc ED2;
  ED2.setExternalType(ExtType2);
  ED2.setExternalName("Loader");
  ED2.setExternalIndex(30);

  ExpSec.getContent().push_back(ED1);
  ExpSec.getContent().push_back(ED2);

  std::vector<uint8_t> Output = Ser.serializeExportSection(ExpSec);
  std::vector<uint8_t> Expected = {
      0x07U, // section ID
      0x14U, // Content size = 20
      0x02U, // Vector length = 2
      // vec[0]
      0x06U, 0x4CU, 0x6FU, 0x61U, 0x64U, 0x65U, 0x72U, // ExtName: Loader
      0x00U, 0x91U, 0x01U,                             // function type
      // vec[1]
      0x06U, 0x4CU, 0x6FU, 0x61U, 0x64U, 0x65U, 0x72U, // ExtName: Loader
      0x01U, 0x1EU                                     // Table type
  };
  EXPECT_EQ(Output, Expected);
}

TEST(SerializeSectionTest, SerializeStartSection) {

  WasmEdge::AST::StartSection StartSec;
  StartSec.setContent(717936);

  std::vector<uint8_t> Output = Ser.serializeStartSection(StartSec);
  std::vector<uint8_t> Expected = {
      0x08U,              // section ID
      0x03U,              // Content size = 3
      0xF0U, 0xE8U, 0x2BU // Content
  };
  EXPECT_EQ(Output, Expected);
}

TEST(SerializeSectionTest, SerializeDataCountSection) {

  WasmEdge::AST::DataCountSection DataSec;
  DataSec.setContent(4279234575);

  std::vector<uint8_t> Output = Ser.serializeDataCountSection(DataSec);

  std::vector<uint8_t> Expected = {
      0x12U,                            // section ID
      0x05U,                            // Content size = 5
      0x8FU, 0xE0U, 0xBFU, 0xF8U, 0x0FU // Content
  };

  EXPECT_EQ(Output, Expected);
}
} // namespace
