// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "loader/serialize.h"

#include <cstdint>
#include <gtest/gtest.h>
#include <vector>

namespace {

WasmEdge::Configure Conf;
WasmEdge::Loader::Serializer Ser(Conf);

TEST(SerializeSectionTest, SerializeCustomSection) {
  WasmEdge::AST::CustomSection CustomSec;
  CustomSec.setName("name");
  CustomSec.getContent() = {0x01U, 0x02U, 0x03U, 0x04U, 0x05U};

  std::vector<uint8_t> Output;
  EXPECT_TRUE(Ser.serializeSection(CustomSec, Output));
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
      std::vector<WasmEdge::ValType>{WasmEdge::TypeCode::I32,
                                     WasmEdge::TypeCode::I64},
      {});
  WasmEdge::AST::FunctionType FT2(
      std::vector<WasmEdge::ValType>{WasmEdge::TypeCode::F32,
                                     WasmEdge::TypeCode::F64},
      std::vector<WasmEdge::ValType>{WasmEdge::TypeCode::FuncRef});
  WasmEdge::AST::FunctionType FT3(
      {}, std::vector<WasmEdge::ValType>{WasmEdge::TypeCode::V128});
  TypeSec.getContent().push_back(FT1);
  TypeSec.getContent().push_back(FT2);
  TypeSec.getContent().push_back(FT3);

  std::vector<uint8_t> Output;
  EXPECT_TRUE(Ser.serializeSection(TypeSec, Output));
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
  ID2.getExternalMemoryType().getLimit().setMin(0);
  ID2.getExternalMemoryType().getLimit().setMax(15);
  ID2.getExternalMemoryType().getLimit().setType(
      WasmEdge::AST::Limit::LimitType::HasMinMax);

  WasmEdge::AST::ImportDesc ID3;
  ID3.setModuleName("test");
  ID3.setExternalName("Loader3");
  ID3.setExternalType(WasmEdge::ExternalType::Global);
  ID3.getExternalGlobalType().setValType(WasmEdge::TypeCode::F64);
  ID3.getExternalGlobalType().setValMut(WasmEdge::ValMut::Const);

  WasmEdge::AST::ImportSection ImpSec;
  ImpSec.getContent().push_back(ID1);
  ImpSec.getContent().push_back(ID2);
  ImpSec.getContent().push_back(ID3);

  std::vector<uint8_t> Output;
  EXPECT_TRUE(Ser.serializeSection(ImpSec, Output));
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

  std::vector<uint8_t> Output;
  EXPECT_TRUE(Ser.serializeSection(FuncSec, Output));
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
  WasmEdge::AST::TableSegment TS1;
  WasmEdge::AST::TableType TT1(WasmEdge::TypeCode::ExternRef, 0, 15);
  TS1.getTableType() = TT1;
  WasmEdge::AST::TableSegment TS2;
  WasmEdge::AST::TableType TT2(WasmEdge::TypeCode::FuncRef, 0, 14);
  TS2.getTableType() = TT2;
  TableSec.getContent().push_back(TS1);
  TableSec.getContent().push_back(TS2);

  std::vector<uint8_t> Output;
  EXPECT_TRUE(Ser.serializeSection(TableSec, Output));
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

  std::vector<uint8_t> Output;
  EXPECT_TRUE(Ser.serializeSection(MemSec, Output));
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

TEST(SerializeSectionTest, SerializeGlobalSection) {
  WasmEdge::AST::GlobalSection GlobalSec;
  WasmEdge::AST::GlobalSegment GlobalSeg1;
  WasmEdge::AST::GlobalType GlobalType1(WasmEdge::TypeCode::F64,
                                        WasmEdge::ValMut::Const);
  GlobalSeg1.getGlobalType() = GlobalType1;
  GlobalSeg1.getExpr().getInstrs() = {
      WasmEdge::AST::Instruction(WasmEdge::OpCode::End)};

  WasmEdge::AST::GlobalSegment GlobalSeg2;
  WasmEdge::AST::GlobalType GlobalType2(WasmEdge::TypeCode::F32,
                                        WasmEdge::ValMut::Const);
  GlobalSeg2.getGlobalType() = GlobalType2;
  GlobalSeg2.getExpr().getInstrs() = {
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__eqz),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::End)};

  GlobalSec.getContent().push_back(GlobalSeg1);
  GlobalSec.getContent().push_back(GlobalSeg2);

  std::vector<uint8_t> Output;
  EXPECT_TRUE(Ser.serializeSection(GlobalSec, Output));
  std::vector<uint8_t> Expected = {
      0x06U,                      // Global section
      0x08U,                      // Content size = 8
      0x02U,                      // Vector length = 2
      0x7CU, 0x00U, 0x0BU,        // vec[0]
      0x7DU, 0x00U, 0x45U, 0x0BU, // vec[1]
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

  std::vector<uint8_t> Output;
  EXPECT_TRUE(Ser.serializeSection(ExpSec, Output));
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

  std::vector<uint8_t> Output;
  EXPECT_TRUE(Ser.serializeSection(StartSec, Output));
  std::vector<uint8_t> Expected = {
      0x08U,              // section ID
      0x03U,              // Content size = 3
      0xF0U, 0xE8U, 0x2BU // Content
  };
  EXPECT_EQ(Output, Expected);
}

TEST(SerializeSectionTest, SerializeElementSection) {
  WasmEdge::AST::ElementSection ElementSec;
  ElementSec.setContentSize(1);

  WasmEdge::AST::ElementSegment ElementSeg;
  ElementSeg.setMode(WasmEdge::AST::ElementSegment::ElemMode::Active);

  WasmEdge::AST::Expression Expr;
  Expr.getInstrs() = {WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__eqz),
                      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__eq),
                      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__ne),
                      WasmEdge::AST::Instruction(WasmEdge::OpCode::End)};
  ElementSeg.getExpr() = Expr;

  auto RefFunc1 = WasmEdge::AST::Instruction(WasmEdge::OpCode::Ref__func);
  RefFunc1.getTargetIndex() = 0x0A;
  auto RefFunc2 = WasmEdge::AST::Instruction(WasmEdge::OpCode::Ref__func);
  RefFunc2.getTargetIndex() = 0x0B;
  auto RefFunc3 = WasmEdge::AST::Instruction(WasmEdge::OpCode::Ref__func);
  RefFunc3.getTargetIndex() = 0x0C;
  auto End = WasmEdge::AST::Instruction(WasmEdge::OpCode::End);

  WasmEdge::AST::Expression InitExpr1;
  InitExpr1.getInstrs() = {RefFunc1, End};
  WasmEdge::AST::Expression InitExpr2;
  InitExpr2.getInstrs() = {RefFunc2, End};
  WasmEdge::AST::Expression InitExpr3;
  InitExpr3.getInstrs() = {RefFunc3, End};
  ElementSeg.getInitExprs() = {InitExpr1, InitExpr2, InitExpr3};

  ElementSec.getContent() = {ElementSeg};

  std::vector<uint8_t> Output;
  EXPECT_TRUE(Ser.serializeSection(ElementSec, Output));
  std::vector<uint8_t> Expected = {
      0x09U,                      // Element section
      0x0AU,                      // Content size = 10
      0x01U,                      // Vector length = 1
      0x00U,                      // Prefix 0x00
      0x45U, 0x46U, 0x47U, 0x0BU, // Expression
      0x03U, 0x0AU, 0x0BU, 0x0CU  // Vec(3)
  };
  EXPECT_EQ(Output, Expected);
}

TEST(SerializeSectionTest, SerializeCodeSection) {
  WasmEdge::AST::CodeSection CodeSec;
  WasmEdge::AST::CodeSegment CodeSeg;
  CodeSeg.setSegSize(8);
  CodeSeg.getLocals() = {{1, WasmEdge::TypeCode::F64},
                         {3, WasmEdge::TypeCode::F32}};
  WasmEdge::AST::Expression Expr;
  Expr.getInstrs() = {WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__eqz),
                      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__eq),
                      WasmEdge::AST::Instruction(WasmEdge::OpCode::End)};
  CodeSeg.getExpr() = Expr;
  CodeSec.getContent().push_back(CodeSeg);

  std::vector<uint8_t> Output;
  EXPECT_TRUE(Ser.serializeSection(CodeSec, Output));
  std::vector<uint8_t> Expected = {
      0x0AU,              // Code section
      0x0AU,              // Content size = 10
      0x01U,              // Vector length = 1
      0x08U,              // Code segment size = 8
      0x02U,              // Vector length = 2
      0x01U, 0x7CU,       // vec[0]
      0x03U, 0x7DU,       // vec[1]
      0x45U, 0x46U, 0x0BU // Expression
  };
  EXPECT_EQ(Output, Expected);
}

TEST(SerializeSectionTest, SerializeDataSection) {
  WasmEdge::AST::DataSection DataSec;
  WasmEdge::AST::DataSegment DataSeg;
  WasmEdge::AST::Expression Expr;
  Expr.getInstrs() = {WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__eqz),
                      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__eq),
                      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__ne),
                      WasmEdge::AST::Instruction(WasmEdge::OpCode::End)};
  DataSeg.setMode(WasmEdge::AST::DataSegment::DataMode::Active);
  DataSeg.getExpr() = Expr;
  DataSeg.getData() = {'t', 'e', 's', 't'};
  DataSec.getContent().push_back(DataSeg);

  std::vector<uint8_t> Output;
  EXPECT_TRUE(Ser.serializeSection(DataSec, Output));
  std::vector<uint8_t> Expected = {
      0x0BU,                            // Data section
      0x0BU,                            // Content size = 11
      0x01U,                            // Vector length = 1
      0x00U,                            // Prefix checking byte
      0x45U, 0x46U, 0x47U, 0x0BU,       // Expression
      0x04U, 0x74U, 0x65U, 0x73U, 0x74U // Vector length = 4, "test"
  };
  EXPECT_EQ(Output, Expected);
}

TEST(SerializeSectionTest, SerializeDataCountSection) {
  WasmEdge::AST::DataCountSection DataCntSec;
  DataCntSec.setContent(4279234575);

  std::vector<uint8_t> Output;
  EXPECT_TRUE(Ser.serializeSection(DataCntSec, Output));
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
