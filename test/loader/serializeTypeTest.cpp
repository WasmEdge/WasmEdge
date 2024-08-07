// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "loader/serialize.h"

#include <cstdint>
#include <gtest/gtest.h>
#include <vector>

namespace {

WasmEdge::Configure Conf;
WasmEdge::Loader::Serializer Ser(Conf);

WasmEdge::AST::TypeSection
createTypeSec(const WasmEdge::AST::FunctionType &FuncType) {
  WasmEdge::AST::TypeSection TypeSec;
  TypeSec.getContent() = {FuncType};
  return TypeSec;
}

WasmEdge::AST::TableSection
createTableSec(const WasmEdge::AST::TableType &TableType) {
  WasmEdge::AST::TableSection TableSec;
  WasmEdge::AST::TableSegment TableSeg;
  TableSeg.getTableType() = TableType;
  TableSec.getContent() = {TableSeg};
  return TableSec;
}

WasmEdge::AST::MemorySection
createMemorySec(WasmEdge::AST::MemoryType MemoryType) {
  WasmEdge::AST::MemorySection MemorySec;
  MemorySec.getContent() = {MemoryType};
  return MemorySec;
}

WasmEdge::AST::GlobalSection
createGlobalSec(WasmEdge::AST::GlobalType GlobalType) {
  WasmEdge::AST::GlobalSection GlobalSec;
  WasmEdge::AST::GlobalSegment GlobalSeg;
  GlobalSeg.getGlobalType() = GlobalType;
  GlobalSeg.getExpr().getInstrs() = {
      WasmEdge::AST::Instruction(WasmEdge::OpCode::End)};
  GlobalSec.getContent() = {GlobalSeg};
  return GlobalSec;
}

TEST(serializeTypeTest, SerializeFunctionType) {
  WasmEdge::Configure ConfNoRefType;
  ConfNoRefType.removeProposal(WasmEdge::Proposal::BulkMemoryOperations);
  ConfNoRefType.removeProposal(WasmEdge::Proposal::ReferenceTypes);
  WasmEdge::Loader::Serializer SerNoRefType(ConfNoRefType);
  WasmEdge::Configure ConfNoMultiVal;
  ConfNoMultiVal.removeProposal(WasmEdge::Proposal::MultiValue);
  WasmEdge::Loader::Serializer SerNoMultiVal(ConfNoMultiVal);

  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;

  // 1. Test serialize function type.
  //
  //   1.  Serialize void parameter and result function type.
  //   2.  Serialize non-void parameter function type.
  //   3.  Serialize non-void result function type.
  //   4.  Serialize function type with parameters and result.
  //   5.  Serialize invalid parameters with ExternRef without Ref-Types
  //       proposal.
  //   6.  Serialize invalid results with ExternRef without Ref-Types proposal.
  //   7.  Serialize invalid function type with multi-value returns without
  //       Multi-Value proposal.

  WasmEdge::AST::FunctionType FuncType;

  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createTypeSec(FuncType), Output));
  Expected = {
      0x01U, // Type section
      0x04U, // Content size = 4
      0x01U, // Vector length = 1
      0x60U, // Function type header
      0x00U, // Parameter length = 0
      0x00U  // Result length = 0
  };
  EXPECT_EQ(Output, Expected);

  FuncType.getParamTypes() = {WasmEdge::TypeCode::F64, WasmEdge::TypeCode::F32,
                              WasmEdge::TypeCode::I64, WasmEdge::TypeCode::I32};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createTypeSec(FuncType), Output));
  Expected = {
      0x01U,                      // Type section
      0x08U,                      // Content size = 8
      0x01U,                      // Vector length = 1
      0x60U,                      // Function type header
      0x04U,                      // Parameter length = 4
      0x7CU, 0x7DU, 0x7EU, 0x7FU, // Parameter list
      0x00U                       // Result length = 0
  };
  EXPECT_EQ(Output, Expected);

  FuncType.getParamTypes() = {};
  FuncType.getReturnTypes() = {WasmEdge::TypeCode::F64};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createTypeSec(FuncType), Output));
  Expected = {
      0x01U, // Type section
      0x05U, // Content size = 5
      0x01U, // Vector length = 1
      0x60U, // Function type header
      0x00U, // Parameter length = 0
      0x01U, // Result length = 1
      0x7CU  // Result list
  };
  EXPECT_EQ(Output, Expected);

  FuncType.getParamTypes() = {WasmEdge::TypeCode::F64, WasmEdge::TypeCode::F32,
                              WasmEdge::TypeCode::I64, WasmEdge::TypeCode::I32};
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createTypeSec(FuncType), Output));
  Expected = {
      0x01U,                      // Type section
      0x09U,                      // Content size = 9
      0x01U,                      // Vector length = 1
      0x60U,                      // Function type header
      0x04U,                      // Parameter length = 4
      0x7CU, 0x7DU, 0x7EU, 0x7FU, // Parameter list
      0x01U,                      // Result length = 1
      0x7CU                       // Result list
  };
  EXPECT_EQ(Output, Expected);

  FuncType.getParamTypes() = {WasmEdge::TypeCode::ExternRef};
  FuncType.getReturnTypes() = {};
  EXPECT_FALSE(SerNoRefType.serializeSection(createTypeSec(FuncType), Output));

  FuncType.getParamTypes() = {};
  FuncType.getReturnTypes() = {WasmEdge::TypeCode::ExternRef};
  EXPECT_FALSE(SerNoRefType.serializeSection(createTypeSec(FuncType), Output));

  FuncType.getReturnTypes() = {WasmEdge::TypeCode::I32,
                               WasmEdge::TypeCode::I32};
  EXPECT_FALSE(SerNoMultiVal.serializeSection(createTypeSec(FuncType), Output));
}

TEST(serializeTypeTest, SerializeTableType) {
  WasmEdge::Configure ConfNoRefType;
  ConfNoRefType.removeProposal(WasmEdge::Proposal::BulkMemoryOperations);
  ConfNoRefType.removeProposal(WasmEdge::Proposal::ReferenceTypes);
  WasmEdge::Loader::Serializer SerNoRefType(ConfNoRefType);

  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;

  // 2. Test serialize table type, which is reference type and limit.
  //
  //   1.  Serialize limit with only min.
  //   2.  Serialize limit with min and max.
  //   3.  Serialize invalid ExternRef without Ref-Types proposal.

  WasmEdge::AST::TableType TableType;

  TableType.setRefType(WasmEdge::TypeCode::FuncRef);
  TableType.getLimit().setMin(4294967295);
  TableType.getLimit().setType(WasmEdge::AST::Limit::LimitType::HasMin);

  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createTableSec(TableType), Output));
  Expected = {
      0x04U,                            // Table section
      0x08U,                            // Content size = 8
      0x01U,                            // Vector length = 1
      0x70U,                            // Reference type
      0x00U,                            // Only has min
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU // Min = 4294967295
  };
  EXPECT_EQ(Output, Expected);

  TableType.setRefType(WasmEdge::TypeCode::FuncRef);
  TableType.getLimit().setMin(4294967281);
  TableType.getLimit().setMax(4294967295);
  TableType.getLimit().setType(WasmEdge::AST::Limit::LimitType::HasMinMax);

  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createTableSec(TableType), Output));
  Expected = {
      0x04U,                             // Table section
      0x0DU,                             // Content size = 13
      0x01U,                             // Vector length = 1
      0x70U,                             // Reference type
      0x01U,                             // Has min and max
      0xF1U, 0xFFU, 0xFFU, 0xFFU, 0x0FU, // Min = 4294967281
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU  // Max = 4294967295
  };
  EXPECT_EQ(Output, Expected);

  TableType.setRefType(WasmEdge::TypeCode::ExternRef);
  EXPECT_FALSE(
      SerNoRefType.serializeSection(createTableSec(TableType), Output));
}

TEST(serializeTypeTest, SerializeMemoryType) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;

  // 3. Test serialize memory type, which is limit.
  //
  //   1.  Serialize limit with only min.
  //   2.  Serialize limit with min and max.

  WasmEdge::AST::MemoryType MemoryType;

  MemoryType.getLimit().setMin(4294967295);
  MemoryType.getLimit().setType(WasmEdge::AST::Limit::LimitType::HasMin);

  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createMemorySec(MemoryType), Output));
  Expected = {
      0x05U,                            // Memory section
      0x07U,                            // Content size = 7
      0x01U,                            // Vector length = 1
      0x00U,                            // Only has min
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU // Min = 4294967295
  };
  EXPECT_EQ(Output, Expected);

  MemoryType.getLimit().setMin(4294967281);
  MemoryType.getLimit().setMax(4294967295);
  MemoryType.getLimit().setType(WasmEdge::AST::Limit::LimitType::HasMinMax);

  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createMemorySec(MemoryType), Output));
  Expected = {
      0x05U,                             // Memory section
      0x0CU,                             // Content size = 12
      0x01U,                             // Vector length = 1
      0x01U,                             // Has min and max
      0xF1U, 0xFFU, 0xFFU, 0xFFU, 0x0FU, // Min = 4294967281
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU  // Max = 4294967295
  };
  EXPECT_EQ(Output, Expected);
}

TEST(serializeTypeTest, SerializeGlobalType) {
  WasmEdge::Configure ConfNoRefType;
  ConfNoRefType.removeProposal(WasmEdge::Proposal::BulkMemoryOperations);
  ConfNoRefType.removeProposal(WasmEdge::Proposal::ReferenceTypes);
  WasmEdge::Loader::Serializer SerNoRefType(ConfNoRefType);

  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;

  // 4. Test serialize global type.
  //
  //   1.  Serialize valid global type.
  //   2.  Load invalid global type with ExternRef without Ref-Types proposal.

  WasmEdge::AST::GlobalType GlobalType;

  GlobalType.setValType(WasmEdge::TypeCode::F64);
  GlobalType.setValMut(WasmEdge::ValMut::Const);
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createGlobalSec(GlobalType), Output));
  Expected = {
      0x06U, // Global section
      0x04U, // Content size = 4
      0x01U, // Vector length = 1
      0x7CU, // F64 number type
      0x00U, // Const mutation
      0x0BU  // Expression
  };
  EXPECT_EQ(Output, Expected);

  GlobalType.setValType(WasmEdge::TypeCode::ExternRef);
  EXPECT_FALSE(
      SerNoRefType.serializeSection(createGlobalSec(GlobalType), Output));
}
} // namespace
