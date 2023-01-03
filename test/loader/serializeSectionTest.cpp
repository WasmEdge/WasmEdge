/// Unit tests of serialize sections

#include "loader/serialize.h"

#include <cstdint>
#include <gtest/gtest.h>
#include <vector>

namespace {

WasmEdge::Loader::Serializer Ser;

TEST(SerializeSectionTest, SerializeCustomSection) {
  WasmEdge::AST::CustomSection CustomSec;
  CustomSec.setName("name");
  CustomSec.getContent() = {0x01U, 0x02U, 0x03U, 0x04U, 0x05U};

  std::vector<uint8_t> Output = Ser.serializeSection(CustomSec);
  std::vector<uint8_t> Expected = {
      0x00U,                            // Section ID
      0x0AU,                            // Content size = 10
      0x04U,                            // Name length = 4
      0x6EU, 0x61U, 0x6DU, 0x65U,       // Name
      0x01U, 0x02U, 0x03U, 0x04U, 0x05U // Content
  };
  EXPECT_EQ(Output, Expected);
}

TEST(SerializeSectionTest, SerializeTypeSection) {
  WasmEdge::AST::TypeSection TypeSec;
  WasmEdge::AST::FunctionType FT1(
      {WasmEdge::ValType::I32, WasmEdge::ValType::I64}, {});
  WasmEdge::AST::FunctionType FT2(
      {WasmEdge::ValType::F32, WasmEdge::ValType::F64},
      {WasmEdge::ValType::FuncRef});
  WasmEdge::AST::FunctionType FT3({}, {WasmEdge::ValType::V128});
  TypeSec.getContent().push_back(FT1);
  TypeSec.getContent().push_back(FT2);
  TypeSec.getContent().push_back(FT3);

  std::vector<uint8_t> Output = Ser.serializeSection(TypeSec);
  std::vector<uint8_t> Expected = {
      0x01U,                                    // section ID
      0x10U,                                    // Content size = 16
      0x03U,                                    // Vector length = 3
      0x60U, 0x02U, 0x7FU, 0x7EU, 0x00U,        // vec[0]
      0x60U, 0x02U, 0x7DU, 0x7CU, 0x01U, 0x70U, // vec[1]
      0x60U, 0x00U, 0x01U, 0x7BU                // vec[2]
  };
  EXPECT_EQ(Output, Expected);
}

TEST(SerializeSectionTest, SerializeImportSection) {
  WasmEdge::AST::ImportDesc ID1;
  ID1.setModuleName("test");
  ID1.setExternalName("Loader1");
  ID1.setExternalType(WasmEdge::ExternalType::Function);
  ID1.setExternalFuncTypeIdx(0x01U);

  WasmEdge::AST::ImportDesc ID2;
  ID2.setModuleName("test");
  ID2.setExternalName("Loader2");
  ID2.setExternalType(WasmEdge::ExternalType::Memory);
  ID2.getExternalMemoryType().getLimit() = WasmEdge::AST::Limit(0, 15);

  WasmEdge::AST::ImportDesc ID3;
  ID3.setModuleName("test");
  ID3.setExternalName("Loader3");
  ID3.setExternalType(WasmEdge::ExternalType::Global);
  ID3.getExternalGlobalType().setValType(WasmEdge::ValType::F64);
  ID3.getExternalGlobalType().setValMut(WasmEdge::ValMut::Const);

  WasmEdge::AST::ImportSection ImpSec;
  ImpSec.getContent().push_back(ID1);
  ImpSec.getContent().push_back(ID2);
  ImpSec.getContent().push_back(ID3);

  std::vector<uint8_t> Output = Ser.serializeSection(ImpSec);
  std::vector<uint8_t> Expected = {
      0x02U, // section ID
      0x31U, // Content size = 49
      0x03U, // Vector length = 3
      // vec[0]
      0x04U, 0x74U, 0x65U, 0x73U, 0x74U, // ModName: "test"
      0x07U, 0x4CU, 0x6FU, 0x61U, 0x64U, 0x65U, 0x72U,
      0x31U,        // ExtName: "Loader1"
      0x00U, 0x01U, // function index
      // vec[1]
      0x04U, 0x74U, 0x65U, 0x73U, 0x74U, // ModName: "test"
      0x07U, 0x4CU, 0x6FU, 0x61U, 0x64U, 0x65U, 0x72U,
      0x32U,                      // ExtName: "Loader2"
      0x02U, 0x01U, 0x00U, 0x0FU, // Memory type
      // vec[2]
      0x04U, 0x74U, 0x65U, 0x73U, 0x74U, // ModName: "test"
      0x07U, 0x4CU, 0x6FU, 0x61U, 0x64U, 0x65U, 0x72U,
      0x33U,              // ExtName: "Loader3"
      0x03U, 0x7CU, 0x00U // Global type
  };
  EXPECT_EQ(Output, Expected);
}

TEST(SerializeSectionTest, SerializeFunctionSection) {
  WasmEdge::AST::FunctionSection FuncSec;
  FuncSec.getContent() = {1, 2, 1, 1};

  std::vector<uint8_t> Output = Ser.serializeSection(FuncSec);
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
  WasmEdge::AST::TableType TT1(WasmEdge::RefType::ExternRef, 0, 15);
  WasmEdge::AST::TableType TT2(WasmEdge::RefType::FuncRef, 0, 14);
  TableSec.getContent().push_back(TT1);
  TableSec.getContent().push_back(TT2);

  std::vector<uint8_t> Output = Ser.serializeSection(TableSec);
  std::vector<uint8_t> Expected = {
      0x04U,                      // section ID
      0x09U,                      // Content size = 9
      0x02U,                      // Vector length = 2
      0x6FU, 0x01U, 0x00U, 0x0FU, // vec[0]
      0x70U, 0x01U, 0x00U, 0x0EU, // vec[1]
  };
  EXPECT_EQ(Output, Expected);
}

TEST(SerializeSectionTest, SerializeMemorySection) {
  WasmEdge::AST::MemorySection MemSec;
  WasmEdge::AST::MemoryType MT1(2);
  WasmEdge::AST::MemoryType MT2(0, 14);
  WasmEdge::AST::MemoryType MT3(0, 13, true);
  MemSec.getContent().push_back(MT1);
  MemSec.getContent().push_back(MT2);
  MemSec.getContent().push_back(MT3);

  std::vector<uint8_t> Output = Ser.serializeSection(MemSec);
  std::vector<uint8_t> Expected = {
      0x05U,               // section ID
      0x09U,               // Content size = 9
      0x03U,               // Vector length = 3
      0x00U, 0x02U,        // vec[0]
      0x01U, 0x00U, 0x0EU, // vec[1]
      0x03U, 0x00U, 0x0DU  // vec[2]
  };
  EXPECT_EQ(Output, Expected);
}

TEST(SerializeSectionTest, SerializeExportSection) {
  WasmEdge::AST::ExportSection ExpSec;
  WasmEdge::AST::ExportDesc ED1;
  ED1.setExternalType(WasmEdge::ExternalType::Function);
  ED1.setExternalName("Loader");
  ED1.setExternalIndex(145);

  WasmEdge::AST::ExportDesc ED2;
  ED2.setExternalType(WasmEdge::ExternalType::Table);
  ED2.setExternalName("Loader");
  ED2.setExternalIndex(30);

  ExpSec.getContent().push_back(ED1);
  ExpSec.getContent().push_back(ED2);

  std::vector<uint8_t> Output = Ser.serializeSection(ExpSec);
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

  std::vector<uint8_t> Output = Ser.serializeSection(StartSec);
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

  std::vector<uint8_t> Output = Ser.serializeSection(DataSec);
  std::vector<uint8_t> Expected = {
      0x0CU,                            // section ID
      0x05U,                            // Content size = 5
      0x8FU, 0xE0U, 0xBFU, 0xF8U, 0x0FU // Content
  };

  EXPECT_EQ(Output, Expected);
}
} // namespace

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
