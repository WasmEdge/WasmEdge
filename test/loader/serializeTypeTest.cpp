/// Unit tests of serialize types

#include "loader/serialize.h"

#include <cstdint>
#include <gtest/gtest.h>
#include <vector>

namespace {

WasmEdge::Loader::Serializer Ser;

WasmEdge::AST::TypeSection createTypeSec(WasmEdge::AST::FunctionType FuncType) {
  WasmEdge::AST::TypeSection TypeSec;
  TypeSec.getContent() = {
    FuncType
  };
  return TypeSec;
}

WasmEdge::AST::TableSection createTableSec(WasmEdge::AST::TableType TableType) {
  WasmEdge::AST::TableSection TableSec;
  TableSec.getContent() = {
    TableType
  };
  return TableSec;
}

WasmEdge::AST::MemorySection createMemorySec(WasmEdge::AST::MemoryType MemoryType) {
  WasmEdge::AST::MemorySection MemorySec;
  MemorySec.getContent() = {
      MemoryType
  };
  return MemorySec;
}

WasmEdge::AST::GlobalSection createGlobalSec(WasmEdge::AST::GlobalType GlobalType) {
  WasmEdge::AST::GlobalSection GlobalSec;
  WasmEdge::AST::GlobalSegment GlobalSeg;
  GlobalSeg.getGlobalType() = GlobalType;
  GlobalSeg.getExpr().getInstrs() = {
    WasmEdge::AST::Instruction(WasmEdge::OpCode::End)
  };
  GlobalSec.getContent() = {GlobalSeg};
  return GlobalSec;
}

TEST(serializeTypeTest, SerializeFunctionType) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;

  // 1. Test serialize function type.
  //
  //   1.  Serialize void parameter and result function type.
  //   2.  Serialize non-void parameter function type.
  //   3.  Serialize non-void result function type.
  //   4.  Serialize function type with parameters and result.

  WasmEdge::AST::FunctionType FuncType;

  Output = Ser.serializeSection(createTypeSec(FuncType));
  Expected = {
      0x01U, // Type section
      0x04U, // Content size = 4
      0x01U, // Vector length = 1
      0x60U, // Function type header
      0x00U, // Parameter length = 0
      0x00U  // Result length = 0
  };
  EXPECT_EQ(Output, Expected);

  FuncType.getParamTypes() = {
    WasmEdge::ValType::F64,
    WasmEdge::ValType::F32,
    WasmEdge::ValType::I64,
    WasmEdge::ValType::I32
  };
  Output = Ser.serializeSection(createTypeSec(FuncType));
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
  FuncType.getReturnTypes() = {
    WasmEdge::ValType::F64
  };
  Output = Ser.serializeSection(createTypeSec(FuncType));
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

  FuncType.getParamTypes() = {
      WasmEdge::ValType::F64,
      WasmEdge::ValType::F32,
      WasmEdge::ValType::I64,
      WasmEdge::ValType::I32
  };
  Output = Ser.serializeSection(createTypeSec(FuncType));
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
}

TEST(serializeTypeTest, SerializeTableType) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;

  // 2. Test serialize table type, which is reference type and limit.
  //
  //   1.  Serialize limit with only min.
  //   2.  Serialize limit with min and max.

  WasmEdge::AST::TableType TableType;

  TableType.setRefType(WasmEdge::RefType::FuncRef);
  TableType.getLimit().setMin(4294967295);
  TableType.getLimit().setType(WasmEdge::AST::Limit::LimitType::HasMin);

  Output = Ser.serializeSection(createTableSec(TableType));
  Expected = {
      0x04U,                            // Table section
      0x08U,                            // Content size = 8
      0x01U,                            // Vector length = 1
      0x70U,                            // Reference type
      0x00U,                            // Only has min
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU // Min = 4294967295
  };
  EXPECT_EQ(Output, Expected);

  TableType.setRefType(WasmEdge::RefType::FuncRef);
  TableType.getLimit().setMin(4294967281);
  TableType.getLimit().setMax(4294967295);
  TableType.getLimit().setType(WasmEdge::AST::Limit::LimitType::HasMinMax);

  Output = Ser.serializeSection(createTableSec(TableType));
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

  Output = Ser.serializeSection(createMemorySec(MemoryType));
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

  Output = Ser.serializeSection(createMemorySec(MemoryType));
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
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;

  // 4. Test serialize global type.
  //
  //   1.  Serialize valid global type.

  WasmEdge::AST::GlobalType GlobalType;

  GlobalType.setValType(WasmEdge::ValType::F64);
  GlobalType.setValMut(WasmEdge::ValMut::Const);
  Output = Ser.serializeSection(createGlobalSec(GlobalType));
  Expected = {
      0x06U, // Global section
      0x04U, // Content size = 4
      0x01U, // Vector length = 1
      0x7CU, // F64 number type
      0x00U, // Const mutation
      0x0BU  // Expression
  };
  EXPECT_EQ(Output, Expected);
}
}