// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "loader/serialize.h"

#include <cstdint>
#include <gtest/gtest.h>
#include <vector>

namespace {

WasmEdge::Loader::Serializer Ser;

WasmEdge::AST::TypeSection
createTypeSec(const WasmEdge::AST::FunctionType &FuncType) {
  WasmEdge::AST::TypeSection TypeSec;
  TypeSec.getContent() = {FuncType};
  return TypeSec;
}

WasmEdge::AST::TypeSection
createTypeSec(const WasmEdge::AST::SubType &SubType) {
  WasmEdge::AST::TypeSection TypeSec;
  TypeSec.getContent() = {SubType};
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

TEST(SerializeTypeTest, SerializeValType) {
  std::vector<uint8_t> Output;
  std::vector<uint8_t> Expected;

  // 1. Test serialize Function References heap types.
  //
  //   1.  Test FuncRef heap type.
  //   2.  Test ExternRef heap type.
  //   3.  Test Ref heap type.
  //   4.  Test RefNull heap type.
  //   5.  Test TypeIndex 5 heap type.
  //   6.  Test NullFuncRef type.
  //   7.  Test NullExternRef type.
  //   8.  Test NullRef type.
  //   9.  Test AnyRef type.
  //   10.  Test EqRef type.
  //   11.  Test I31Ref type.
  //   12.  Test StructRef type.
  //   13.  Test ArrayRef type.
  //   14.  Test them as RefTypes
  //   15.  Test I8 storage type.
  //   16.  Test I16 storage type.
  //   17.  Test ExnRef type.

  WasmEdge::AST::GlobalType GlobalType;
  GlobalType.setValType(WasmEdge::TypeCode::FuncRef);
  GlobalType.setValMut(WasmEdge::ValMut::Const);
  Output = {};

  Ser.serializeSection(createGlobalSec(GlobalType), Output);
  Expected = {
      0x06U, // Global section
      0x04U, // Content size = 4
      0x01U, // Vector length = 1
      0x70U, // FuncRef type
      0x00U, // Const mutation
      0x0BU  // Expression
  };
  EXPECT_EQ(Output, Expected);

  GlobalType.setValType(WasmEdge::TypeCode::ExternRef);
  Output = {};
  Ser.serializeSection(createGlobalSec(GlobalType), Output);
  Expected = {
      0x06U, // Global section
      0x04U, // Content size = 4
      0x01U, // Vector length = 1
      0x6FU, // ExternRef type
      0x00U, // Const mutation
      0x0BU  // Expression
  };
  EXPECT_EQ(Output, Expected);

  GlobalType.setValType(WasmEdge::ValType(WasmEdge::TypeCode::Ref,
                                          WasmEdge::TypeCode::ExternRef));
  Output = {};
  Ser.serializeSection(createGlobalSec(GlobalType), Output);
  Expected = {
      0x06U, // Global section
      0x05U, // Content size = 5
      0x01U, // Vector length = 1
      0x64U, // Ref type
      0x6FU, // ExternRef heap type
      0x00U, // Const mutation
      0x0BU  // Expression End
  };
  EXPECT_EQ(Output, Expected);

  GlobalType.setValType(WasmEdge::ValType(WasmEdge::TypeCode::RefNull,
                                          WasmEdge::TypeCode::ExternRef));
  Output = {};
  Ser.serializeSection(createGlobalSec(GlobalType), Output);
  Expected = {
      0x06U, // Global section
      0x04U, // Content size = 4
      0x01U, // Vector length = 1
      0x6FU, // ExternRef heap type
      0x00U, // Const mutation
      0x0BU  // Expression End
  };
  EXPECT_EQ(Output, Expected);

  GlobalType.setValType(WasmEdge::ValType(WasmEdge::TypeCode::Ref, 5));
  Output = {};
  Ser.serializeSection(createGlobalSec(GlobalType), Output);
  Expected = {
      0x06U, // Global section
      0x05U, // Content size = 5
      0x01U, // Vector length = 1
      0x64U, // Ref heap type
      0x05U, // Type index 5
      0x00U, // Second byte reserved for future extensions
      0x0BU  // Expression End
  };
  EXPECT_EQ(Output, Expected);

  GlobalType.setValType(WasmEdge::TypeCode::NullFuncRef);
  Output = {};
  Ser.serializeSection(createGlobalSec(GlobalType), Output);
  Expected = {
      0x06U, // Global section
      0x04U, // Content size = 4
      0x01U, // Vector length = 1
      0x73U, // NullFuncRef type
      0x00U, // Const mutation
      0x0BU  // Expression
  };
  EXPECT_EQ(Output, Expected);

  GlobalType.setValType(WasmEdge::TypeCode::NullExternRef);
  Output = {};
  Ser.serializeSection(createGlobalSec(GlobalType), Output);
  Expected[3] = 0x72U; // Opcode NullExternRef
  EXPECT_EQ(Output, Expected);

  GlobalType.setValType(WasmEdge::TypeCode::NullRef);
  Output = {};
  Ser.serializeSection(createGlobalSec(GlobalType), Output);
  Expected[3] = 0x71U; // Opcode NullRef
  EXPECT_EQ(Output, Expected);

  GlobalType.setValType(WasmEdge::TypeCode::AnyRef);
  Output = {};
  Ser.serializeSection(createGlobalSec(GlobalType), Output);
  Expected[3] = 0x6EU; // Opcode AnyRef
  EXPECT_EQ(Output, Expected);

  GlobalType.setValType(WasmEdge::TypeCode::EqRef);
  Output = {};
  Ser.serializeSection(createGlobalSec(GlobalType), Output);
  Expected[3] = 0x6DU; // Opcode EqRef
  EXPECT_EQ(Output, Expected);

  GlobalType.setValType(WasmEdge::TypeCode::I31Ref);
  Output = {};
  Ser.serializeSection(createGlobalSec(GlobalType), Output);
  Expected[3] = 0x6CU; // Opcode I31Ref
  EXPECT_EQ(Output, Expected);

  GlobalType.setValType(WasmEdge::TypeCode::StructRef);
  Output = {};
  Ser.serializeSection(createGlobalSec(GlobalType), Output);
  Expected[3] = 0x6BU; // Opcode StructRef
  EXPECT_EQ(Output, Expected);

  GlobalType.setValType(WasmEdge::TypeCode::ArrayRef);
  Output = {};
  Ser.serializeSection(createGlobalSec(GlobalType), Output);
  Expected[3] = 0x6AU; // Opcode ArrayRef
  EXPECT_EQ(Output, Expected);

  Output = {};

  // Tests for the previous types as RefTypes
  WasmEdge::AST::TableType TableType;
  TableType.setRefType(WasmEdge::TypeCode::NullFuncRef);
  TableType.getLimit().setMin(4294967295);
  TableType.getLimit().setType(WasmEdge::AST::Limit::LimitType::HasMin);

  Output = {};
  Ser.serializeSection(createTableSec(TableType), Output);
  Expected = {
      0x04U,                            // Table section
      0x08U,                            // Content size = 8
      0x01U,                            // Vector length = 1
      0x73U,                            // NullFuncRef type
      0x00U,                            // Only has min
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU // Min = 4294967295
  };
  EXPECT_EQ(Output, Expected);

  Output = {};
  TableType.setRefType(WasmEdge::TypeCode::NullExternRef);
  Ser.serializeSection(createTableSec(TableType), Output);
  Expected[3] = 0x72U; // NullExternRef type

  Output = {};
  TableType.setRefType(WasmEdge::TypeCode::NullRef);
  Ser.serializeSection(createTableSec(TableType), Output);
  Expected[3] = 0x71U; // NullRef type

  Output = {};
  TableType.setRefType(WasmEdge::TypeCode::FuncRef);
  Ser.serializeSection(createTableSec(TableType), Output);
  Expected[3] = 0x70U; // FuncRef type

  Output = {};
  TableType.setRefType(WasmEdge::TypeCode::ExternRef);
  Ser.serializeSection(createTableSec(TableType), Output);
  Expected[3] = 0x6FU; // ExternRef type

  Output = {};
  TableType.setRefType(WasmEdge::TypeCode::AnyRef);
  Ser.serializeSection(createTableSec(TableType), Output);
  Expected[3] = 0x6EU; // AnyRef type

  Output = {};
  TableType.setRefType(WasmEdge::TypeCode::EqRef);
  Ser.serializeSection(createTableSec(TableType), Output);
  Expected[3] = 0x6DU; // EqRef type

  Output = {};
  TableType.setRefType(WasmEdge::TypeCode::I31Ref);
  Ser.serializeSection(createTableSec(TableType), Output);
  Expected[3] = 0x6CU; // I31Ref type

  Output = {};
  TableType.setRefType(WasmEdge::TypeCode::StructRef);
  Ser.serializeSection(createTableSec(TableType), Output);
  Expected[3] = 0x6BU; // StructRef type

  Output = {};
  TableType.setRefType(WasmEdge::TypeCode::ArrayRef);
  Ser.serializeSection(createTableSec(TableType), Output);
  Expected[3] = 0x6AU; // ArrayRef type

  Output = {};

  // Test I8 and I16 types
  GlobalType.setValType(WasmEdge::TypeCode::I8);
  Output = {};
  Ser.serializeSection(createGlobalSec(GlobalType), Output);
  Expected = {
      0x06U, // Global section
      0x04U, // Content size = 4
      0x01U, // Vector length = 1
      0x78U, // I8 type
      0x00U, // Const mutation
      0x0BU  // Expression
  };
  EXPECT_EQ(Output, Expected);

  GlobalType.setValType(WasmEdge::TypeCode::I16);
  Output = {};
  Ser.serializeSection(createGlobalSec(GlobalType), Output);
  Expected[3] = 0x77U; // Opcode I16
  EXPECT_EQ(Output, Expected);

  Output = {};

  GlobalType.setValType(WasmEdge::ValType(WasmEdge::TypeCode::Ref,
                                          WasmEdge::TypeCode::StructRef));
  Expected = {
      0x06U, // Global section
      0x05U, // Content size = 5
      0x01U, // Vector length = 1
      0x64U, // Ref type
      0x6BU, // StructRef type
      0x00U, // Const mutation
      0x0BU  // Expression
  };
  Output = {};
  Ser.serializeSection(createGlobalSec(GlobalType), Output);
  EXPECT_EQ(Output, Expected);

  GlobalType.setValType(WasmEdge::ValType(WasmEdge::TypeCode::RefNull,
                                          WasmEdge::TypeCode::StructRef));
  Output = {};
  Expected = {
      0x06U, // Global section
      0x04U, // Content size = 4
      0x01U, // Vector length = 1
      0x6BU, // StructRef type
      0x00U, // Const mutation
      0x0BU  // Expression
  };
  Ser.serializeSection(createGlobalSec(GlobalType), Output);
  EXPECT_EQ(Output, Expected);

  GlobalType.setValType(WasmEdge::TypeCode::ExnRef);
  Output = {};
  Ser.serializeSection(createGlobalSec(GlobalType), Output);
  Expected = {
      0x06U, // Global section
      0x04U, // Content size = 4
      0x01U, // Vector length = 1
      0x69U, // ExnRef type
      0x00U, // Const mutation
      0x0BU  // Expression
  };
  EXPECT_EQ(Output, Expected);
}

TEST(SerializeTypeTest, SerializeFunctionType) {

  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;

  // 2. Test serialize function type.
  //
  //   1.  Serialize void parameter and result function type.
  //   2.  Serialize non-void parameter function type.
  //   3.  Serialize non-void result function type.
  //   4.  Serialize function type with parameters and result.

  WasmEdge::AST::FunctionType FuncType;

  Output = {};
  Ser.serializeSection(createTypeSec(FuncType), Output);
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
  Ser.serializeSection(createTypeSec(FuncType), Output);
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
  Ser.serializeSection(createTypeSec(FuncType), Output);
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
  Ser.serializeSection(createTypeSec(FuncType), Output);
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

  FuncType.getParamTypes() = {};
  FuncType.getReturnTypes() = {WasmEdge::TypeCode::ExternRef};

  FuncType.getReturnTypes() = {WasmEdge::TypeCode::I32,
                               WasmEdge::TypeCode::I32};
}

TEST(SerializeTypeTest, SerializeCompositeType) {
  std::vector<uint8_t> Output;
  std::vector<uint8_t> Expected;

  // 3. Test composite types.
  //
  //   1.  Test CompositeType (Array).
  //   2.  Test CompositeType (Struct).

  WasmEdge::AST::SubType SubType;
  WasmEdge::AST::FieldType FType;
  WasmEdge::AST::CompositeType CompType;

  FType.setStorageType(WasmEdge::TypeCode::I8);
  FType.setValMut(WasmEdge::ValMut::Const);
  CompType.setArrayType(std::move(FType));
  Output = {};
  SubType.getCompositeType() = CompType;
  Ser.serializeSection(createTypeSec(SubType), Output);
  Expected = {
      0x01U, // Type section
      0x04U, // Content size
      0x01U, // Vector length
      0x5EU, // Array type
      0x78U, // I8 type
      0x00U  // Const mutation
  };
  EXPECT_EQ(Output, Expected);

  Output = {};

  CompType.setStructType({FType, FType, FType});
  SubType.getCompositeType() = CompType;
  Output = {};
  Ser.serializeSection(createTypeSec(SubType), Output);
  Expected = {
      0x01U,        // Type section
      0x09U,        // Content size
      0x01U,        // Vector length
      0x5FU,        // Struct type
      0x03U,        // Vector length
      0x78U, 0x00U, // First Field Type (I8 Const mutation)
      0x78U, 0x00U, // Second Field Type (I8 Const mutation)
      0x78U, 0x00U  // Third Field Type (I8 Const mutation)
  };
  EXPECT_EQ(Output, Expected);
}

TEST(SerializeTypeTest, SerializeSubType) {
  std::vector<uint8_t> Output;
  std::vector<uint8_t> Expected;

  // 4. Test serialize SubType, CompositeType, RecuresiveType, and FieldTypes.
  //
  //   1.  Test SubType (and CompositeType too) with final flag.
  //   2.  Test SubType (CompositeType, and FieldType too) with final flag.
  //   3.  Test SubType (CompositeType, and FieldType too) without final flag.
  //   4.  Test non-final SubType with zero supertypes (emits 0x50 0x00).
  //   5.  Test RecType (RecType ::= 0x4E vector(subtype)).

  WasmEdge::AST::SubType SubType;
  SubType.getCompositeType() =
      WasmEdge::AST::CompositeType(WasmEdge::AST::FunctionType());
  SubType.getSuperTypeIndices() = {0x01U, 0x02U, 0x03U};
  SubType.setFinal(true);
  Output = {};
  Ser.serializeSection(createTypeSec(SubType), Output);
  Expected = {
      0x01U,               // Type section
      0x09U,               // Content size
      0x01U,               // Vector length
      0x4FU,               // SubFinal type
      0x03U,               // TypeIdx Vector size
      0x01U, 0x02U, 0x03U, // TypeIdx vector
      0x60U,               // FuncType header
      0x00U,               // Param length
      0x00U                // Result length
  };
  EXPECT_EQ(Output, Expected);

  WasmEdge::AST::FieldType FType;
  FType.setStorageType(WasmEdge::TypeCode::I8);
  FType.setValMut(WasmEdge::ValMut::Const);
  WasmEdge::AST::CompositeType CompType;
  CompType.setArrayType(std::move(FType));
  Output = {};
  WasmEdge::AST::SubType SubType1;
  SubType1.getCompositeType() = CompType;
  Ser.serializeSection(createTypeSec(SubType1), Output);
  Expected = {
      0x01U, // Type section
      0x04U, // Content size
      0x01U, // Vector length
      0x5EU, // Array type
      0x78U, // I8 type
      0x00U  // Const mutation
  };
  EXPECT_EQ(Output, Expected);

  WasmEdge::AST::SubType SubType2;
  SubType2.getCompositeType() = CompType;
  SubType2.getSuperTypeIndices() = {0x01U, 0x02U, 0x03U};
  SubType2.setFinal(false);
  Output = {};
  Ser.serializeSection(createTypeSec(SubType2), Output);
  Expected = {
      0x01U,               // Type section
      0x09U,               // Content size
      0x01U,               // Vector length
      0x50U,               // Sub type
      0x03U,               // TypeIdx Vector size
      0x01U, 0x02U, 0x03U, // TypeIdx vector
      0x5EU,               // Array type
      0x78U,               // I8 type
      0x00U                // Const mutation
  };
  EXPECT_EQ(Output, Expected);

  WasmEdge::AST::SubType SubType3;
  SubType3.getCompositeType() = CompType;
  SubType3.setFinal(false);
  Output = {};
  Ser.serializeSection(createTypeSec(SubType3), Output);
  Expected = {
      0x01U, // Type section
      0x06U, // Content size
      0x01U, // Vector length
      0x50U, // Sub type (non-final)
      0x00U, // TypeIdx vector size (empty)
      0x5EU, // Array type
      0x78U, // I8 type
      0x00U  // Const mutation
  };
  EXPECT_EQ(Output, Expected);

  WasmEdge::AST::TypeSection TypeSec;
  SubType1.setRecursiveInfo(0x00U, 0x01U);
  SubType2.setRecursiveInfo(0x00U, 0x01U);
  TypeSec.getContent() = {SubType1, SubType2};
  Output = {};
  Ser.serializeSection(TypeSec, Output);
  Expected = {
      0x01U,               // Type section
      0x10U,               // Content size = 16
      0x02U,               // Vector length
      0x4EU,               // Rec type
      0x01U,               // Vector length
      0x5EU,               // Array type
      0x78U,               // I8 type
      0x00U,               // Const mutation
      0x4EU,               // Rec type
      0x01U,               // Vector length
      0x50U,               // Sub type
      0x03U,               // TypeIdx Vector size
      0x01U, 0x02U, 0x03U, // TypeIdx vector
      0x5EU,               // Array type
      0x78U,               // I8 type
      0x00U                // Const mutation
  };
  EXPECT_EQ(Output, Expected);
}

TEST(SerializeTypeTest, SerializeTableType) {

  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;

  // 5. Test serialize table type, which is reference type and limit.
  //
  //   1.  Serialize limit with only min.
  //   2.  Serialize limit with min and max.

  WasmEdge::AST::TableType TableType;

  TableType.setRefType(WasmEdge::TypeCode::FuncRef);
  TableType.getLimit().setMin(4294967295);
  TableType.getLimit().setType(WasmEdge::AST::Limit::LimitType::HasMin);

  Output = {};
  Ser.serializeSection(createTableSec(TableType), Output);
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
  Ser.serializeSection(createTableSec(TableType), Output);
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
}

TEST(SerializeTypeTest, SerializeMemoryType) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;

  // 6. Test serialize memory type, which is limit.
  //
  //   1.  Serialize limit with only min.
  //   2.  Serialize limit with min and max.

  WasmEdge::AST::MemoryType MemoryType;

  MemoryType.getLimit().setMin(4294967295);
  MemoryType.getLimit().setType(WasmEdge::AST::Limit::LimitType::HasMin);

  Output = {};
  Ser.serializeSection(createMemorySec(MemoryType), Output);
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
  Ser.serializeSection(createMemorySec(MemoryType), Output);
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

TEST(SerializeTypeTest, SerializeLimitAllFlags) {
  // 7. Pin all eight limit flag encodings. The flag is derived from the limit
  // alone: bit 0 is a maximum, bit 1 is shared, bit 2 is a 64-bit index.
  using LimitType = WasmEdge::AST::Limit::LimitType;

  auto serialize = [](LimitType Type, uint64_t Min, uint64_t Max) {
    WasmEdge::AST::MemoryType MemType;
    MemType.getLimit().setType(Type);
    MemType.getLimit().setMin(Min);
    MemType.getLimit().setMax(Max);
    WasmEdge::AST::MemorySection Sec;
    Sec.getContent() = {MemType};
    std::vector<uint8_t> Output;
    Ser.serializeSection(Sec, Output);
    // Drop the section id, section size and vector length.
    return std::vector<uint8_t>(Output.begin() + 3, Output.end());
  };

  EXPECT_EQ(serialize(LimitType::HasMin, 1, 0),
            std::vector<uint8_t>({0x00U, 0x01U}));
  EXPECT_EQ(serialize(LimitType::HasMinMax, 1, 2),
            std::vector<uint8_t>({0x01U, 0x01U, 0x02U}));
  EXPECT_EQ(serialize(LimitType::SharedNoMax, 1, 0),
            std::vector<uint8_t>({0x02U, 0x01U}));
  EXPECT_EQ(serialize(LimitType::Shared, 1, 2),
            std::vector<uint8_t>({0x03U, 0x01U, 0x02U}));
  EXPECT_EQ(serialize(LimitType::I64HasMin, 1, 0),
            std::vector<uint8_t>({0x04U, 0x01U}));
  EXPECT_EQ(serialize(LimitType::I64HasMinMax, 1, 2),
            std::vector<uint8_t>({0x05U, 0x01U, 0x02U}));
  EXPECT_EQ(serialize(LimitType::I64SharedNoMax, 1, 0),
            std::vector<uint8_t>({0x06U, 0x01U}));
  EXPECT_EQ(serialize(LimitType::I64Shared, 1, 2),
            std::vector<uint8_t>({0x07U, 0x01U, 0x02U}));
}

TEST(SerializeTypeTest, SerializeMemory64AndSharedLimit) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;

  // 8. Test serialize memory64 (i64) and shared limits.
  //
  //   1.  Serialize i64 memory limit with only min (flag 0x04).
  //   2.  Serialize i64 memory limit with min and max (flag 0x05).
  //   3.  Serialize i64 table limit with min and max (flag 0x05).
  //   4.  Serialize shared limit with min and max (flag 0x03).

  WasmEdge::AST::MemoryType MemoryType;

  MemoryType.getLimit().setType(WasmEdge::AST::Limit::LimitType::I64HasMin);
  MemoryType.getLimit().setMin(4294967296);
  Output = {};
  Ser.serializeSection(createMemorySec(MemoryType), Output);
  Expected = {
      0x05U,                            // Memory section
      0x07U,                            // Content size = 7
      0x01U,                            // Vector length = 1
      0x04U,                            // I64 only has min
      0x80U, 0x80U, 0x80U, 0x80U, 0x10U // Min = 4294967296
  };
  EXPECT_EQ(Output, Expected);

  MemoryType.getLimit().setType(WasmEdge::AST::Limit::LimitType::I64HasMinMax);
  MemoryType.getLimit().setMin(4294967296);
  MemoryType.getLimit().setMax(8589934592);
  Output = {};
  Ser.serializeSection(createMemorySec(MemoryType), Output);
  Expected = {
      0x05U,                             // Memory section
      0x0CU,                             // Content size = 12
      0x01U,                             // Vector length = 1
      0x05U,                             // I64 has min and max
      0x80U, 0x80U, 0x80U, 0x80U, 0x10U, // Min = 4294967296
      0x80U, 0x80U, 0x80U, 0x80U, 0x20U  // Max = 8589934592
  };
  EXPECT_EQ(Output, Expected);

  // The same limit path is used for table64; check the ref type and framing.
  WasmEdge::AST::TableType TableType;
  TableType.setRefType(WasmEdge::TypeCode::FuncRef);
  TableType.getLimit().setType(WasmEdge::AST::Limit::LimitType::I64HasMinMax);
  TableType.getLimit().setMin(4294967296);
  TableType.getLimit().setMax(8589934592);
  Output = {};
  Ser.serializeSection(createTableSec(TableType), Output);
  Expected = {
      0x04U,                             // Table section
      0x0DU,                             // Content size = 13
      0x01U,                             // Vector length = 1
      0x70U,                             // FuncRef type
      0x05U,                             // I64 has min and max
      0x80U, 0x80U, 0x80U, 0x80U, 0x10U, // Min = 4294967296
      0x80U, 0x80U, 0x80U, 0x80U, 0x20U  // Max = 8589934592
  };
  EXPECT_EQ(Output, Expected);

  MemoryType.getLimit().setType(WasmEdge::AST::Limit::LimitType::Shared);
  MemoryType.getLimit().setMin(4294967281);
  MemoryType.getLimit().setMax(4294967295);
  Output = {};
  Ser.serializeSection(createMemorySec(MemoryType), Output);
  Expected = {
      0x05U,                             // Memory section
      0x0CU,                             // Content size = 12
      0x01U,                             // Vector length = 1
      0x03U,                             // Shared with min and max
      0xF1U, 0xFFU, 0xFFU, 0xFFU, 0x0FU, // Min = 4294967281
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU  // Max = 4294967295
  };
  EXPECT_EQ(Output, Expected);

  Output = {};
}

TEST(SerializeTypeTest, SerializeMemoryTypeSharedLimits) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;

  // 9. Test serialize memory type with shared limits.
  //
  //   1.  Serialize shared limit (0x03).

  WasmEdge::AST::MemoryType MemType;
  MemType.getLimit().setMin(0);
  MemType.getLimit().setMax(65536);
  MemType.getLimit().setType(WasmEdge::AST::Limit::LimitType::Shared);

  Output = {};
  Ser.serializeSection(createMemorySec(MemType), Output);
  Expected = {
      0x05U,              // Memory section
      0x06U,              // Content size = 6
      0x01U,              // Vector length = 1
      0x03U,              // Shared flag (0x03)
      0x00U,              // Min = 0
      0x80U, 0x80U, 0x04U // Max = 65536
  };
  EXPECT_EQ(Output, Expected);

  Output = {};

  WasmEdge::AST::MemoryType MemTypeSharedNoMax;
  MemTypeSharedNoMax.getLimit().setMin(10);
  MemTypeSharedNoMax.getLimit().setType(
      WasmEdge::AST::Limit::LimitType::SharedNoMax);
}

TEST(SerializeTypeTest, SerializeGlobalType) {

  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;

  // 10. Test serialize global type.
  //
  //   1.  Serialize valid global type.

  WasmEdge::AST::GlobalType GlobalType;

  GlobalType.setValType(WasmEdge::TypeCode::F64);
  GlobalType.setValMut(WasmEdge::ValMut::Const);
  Output = {};
  Ser.serializeSection(createGlobalSec(GlobalType), Output);
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
}

} // namespace
