// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

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

TEST(serializeTypeTest, SerializeFunctionType) {
  WasmEdge::Configure ConfWASM1;
  ConfWASM1.setWASMStandard(WasmEdge::Standard::WASM_1);
  WasmEdge::Loader::Serializer SerWASM1(ConfWASM1);

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
  EXPECT_FALSE(SerWASM1.serializeSection(createTypeSec(FuncType), Output));

  FuncType.getParamTypes() = {};
  FuncType.getReturnTypes() = {WasmEdge::TypeCode::ExternRef};
  EXPECT_FALSE(SerWASM1.serializeSection(createTypeSec(FuncType), Output));

  FuncType.getReturnTypes() = {WasmEdge::TypeCode::I32,
                               WasmEdge::TypeCode::I32};
  EXPECT_FALSE(SerWASM1.serializeSection(createTypeSec(FuncType), Output));
}

TEST(serializeTypeTest, SerializeTableType) {
  WasmEdge::Configure ConfWASM1;
  ConfWASM1.setWASMStandard(WasmEdge::Standard::WASM_1);
  WasmEdge::Loader::Serializer SerWASM1(ConfWASM1);

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
  EXPECT_FALSE(SerWASM1.serializeSection(createTableSec(TableType), Output));
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

TEST(serializeTypeTest, SerializeMemory64AndSharedLimit) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;

  // Test serialize memory64 (i64) and shared limits.
  //
  //   1.  Serialize i64 memory limit with only min (flag 0x04).
  //   2.  Serialize i64 memory limit with min and max (flag 0x05).
  //   3.  Serialize invalid i64 memory limit without the Memory64 proposal.
  //   4.  Serialize i64 table limit with min and max (flag 0x05).
  //   5.  Serialize shared limit with min and max (flag 0x03).
  //   6.  Serialize invalid shared limit without the Threads proposal.
  //   7.  Serialize invalid shared limit without max (flag 0x02).

  WasmEdge::Configure ConfMem64;
  ConfMem64.removeProposal(WasmEdge::Proposal::Threads);
  ConfMem64.addProposal(WasmEdge::Proposal::Memory64);
  WasmEdge::Loader::Serializer SerMem64(ConfMem64);

  WasmEdge::AST::MemoryType MemoryType;

  MemoryType.getLimit().setType(WasmEdge::AST::Limit::LimitType::I64HasMin);
  MemoryType.getLimit().setMin(4294967296);
  Output = {};
  EXPECT_TRUE(SerMem64.serializeSection(createMemorySec(MemoryType), Output));
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
  EXPECT_TRUE(SerMem64.serializeSection(createMemorySec(MemoryType), Output));
  Expected = {
      0x05U,                             // Memory section
      0x0CU,                             // Content size = 12
      0x01U,                             // Vector length = 1
      0x05U,                             // I64 has min and max
      0x80U, 0x80U, 0x80U, 0x80U, 0x10U, // Min = 4294967296
      0x80U, 0x80U, 0x80U, 0x80U, 0x20U  // Max = 8589934592
  };
  EXPECT_EQ(Output, Expected);

  // I64 limit requires the Memory64 proposal.
  WasmEdge::Configure ConfNoMem64;
  ConfNoMem64.removeProposal(WasmEdge::Proposal::Memory64);
  WasmEdge::Loader::Serializer SerNoMem64(ConfNoMem64);
  Output = {};
  EXPECT_FALSE(
      SerNoMem64.serializeSection(createMemorySec(MemoryType), Output));

  // The same limit path is used for table64; check the ref type and framing.
  WasmEdge::AST::TableType TableType;
  TableType.setRefType(WasmEdge::TypeCode::FuncRef);
  TableType.getLimit().setType(WasmEdge::AST::Limit::LimitType::I64HasMinMax);
  TableType.getLimit().setMin(4294967296);
  TableType.getLimit().setMax(8589934592);
  Output = {};
  EXPECT_TRUE(SerMem64.serializeSection(createTableSec(TableType), Output));
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

  // Shared limit requires the Threads proposal.
  WasmEdge::Configure ConfThreads;
  ConfThreads.addProposal(WasmEdge::Proposal::Threads);
  WasmEdge::Loader::Serializer SerThreads(ConfThreads);

  MemoryType.getLimit().setType(WasmEdge::AST::Limit::LimitType::Shared);
  MemoryType.getLimit().setMin(4294967281);
  MemoryType.getLimit().setMax(4294967295);
  Output = {};
  EXPECT_TRUE(SerThreads.serializeSection(createMemorySec(MemoryType), Output));
  Expected = {
      0x05U,                             // Memory section
      0x0CU,                             // Content size = 12
      0x01U,                             // Vector length = 1
      0x03U,                             // Shared with min and max
      0xF1U, 0xFFU, 0xFFU, 0xFFU, 0x0FU, // Min = 4294967281
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU  // Max = 4294967295
  };
  EXPECT_EQ(Output, Expected);

  WasmEdge::Configure ConfNoThreads;
  ConfNoThreads.removeProposal(WasmEdge::Proposal::Threads);
  WasmEdge::Loader::Serializer SerNoThreads(ConfNoThreads);
  Output = {};
  EXPECT_FALSE(
      SerNoThreads.serializeSection(createMemorySec(MemoryType), Output));

  // Shared limit without max is invalid even with the Threads proposal.
  MemoryType.getLimit().setType(WasmEdge::AST::Limit::LimitType::SharedNoMax);
  Output = {};
  EXPECT_FALSE(SerThreads.serializeSection(createMemorySec(MemoryType), Output));
}

TEST(serializeTypeTest, SerializeGlobalType) {
  WasmEdge::Configure ConfWASM1;
  ConfWASM1.setWASMStandard(WasmEdge::Standard::WASM_1);
  WasmEdge::Loader::Serializer SerWASM1(ConfWASM1);

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
  EXPECT_FALSE(SerWASM1.serializeSection(createGlobalSec(GlobalType), Output));
}

TEST(serializeTypeTest, SerializeValType) {
  std::vector<uint8_t> Output;
  std::vector<uint8_t> Expected;

  // 5. Test serialize Function References heap types.
  //
  //   1. Test FuncRef heap type.
  //   2. Test ExternRef heap type.
  //   3. Test Ref heap type.
  //   4. Test RefNull heap type.
  //   5. Test TypeIndex 5 heap type.
  //   6. Test NullFuncRef type.
  //   7. Test NullExternRef type.
  //   8. Test NullRef type.
  //   9. Test AnyRef type.
  //  10. Test EqRef type.
  //  11. Test I31Ref type.
  //  12. Test StructRef type.
  //  13. Test ArrayRef type.
  //  14. Test them as RefTypes
  //  15. Test I8 storage type.
  //  16. Test I16 storage type.
  //  17. Test I16 storage type without the GC proposal.
  //  18. Test ExnRef type.
  //  19. Test ExnRef type without the exception handling proposal.

  WasmEdge::AST::GlobalType GlobalType;
  GlobalType.setValType(WasmEdge::TypeCode::FuncRef);
  GlobalType.setValMut(WasmEdge::ValMut::Const);
  Output = {};

  EXPECT_TRUE(Ser.serializeSection(createGlobalSec(GlobalType), Output));
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
  EXPECT_TRUE(Ser.serializeSection(createGlobalSec(GlobalType), Output));
  Expected = {
      0x06U, // Global section
      0x04U, // Content size = 4
      0x01U, // Vector length = 1
      0x6FU, // ExternRef type
      0x00U, // Const mutation
      0x0BU  // Expression
  };
  EXPECT_EQ(Output, Expected);

  Conf.addProposal(WasmEdge::Proposal::FunctionReferences);
  GlobalType.setValType(WasmEdge::ValType(WasmEdge::TypeCode::Ref,
                                          WasmEdge::TypeCode::ExternRef));
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createGlobalSec(GlobalType), Output));
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
  EXPECT_TRUE(Ser.serializeSection(createGlobalSec(GlobalType), Output));
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
  EXPECT_TRUE(Ser.serializeSection(createGlobalSec(GlobalType), Output));
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

  Conf.addProposal(WasmEdge::Proposal::GC);
  GlobalType.setValType(WasmEdge::TypeCode::NullFuncRef);
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createGlobalSec(GlobalType), Output));
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
  EXPECT_TRUE(Ser.serializeSection(createGlobalSec(GlobalType), Output));
  Expected[3] = 0x72U; // Opcode NullExternRef
  EXPECT_EQ(Output, Expected);

  GlobalType.setValType(WasmEdge::TypeCode::NullRef);
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createGlobalSec(GlobalType), Output));
  Expected[3] = 0x71U; // Opcode NullRef
  EXPECT_EQ(Output, Expected);

  GlobalType.setValType(WasmEdge::TypeCode::AnyRef);
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createGlobalSec(GlobalType), Output));
  Expected[3] = 0x6EU; // Opcode AnyRef
  EXPECT_EQ(Output, Expected);

  GlobalType.setValType(WasmEdge::TypeCode::EqRef);
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createGlobalSec(GlobalType), Output));
  Expected[3] = 0x6DU; // Opcode EqRef
  EXPECT_EQ(Output, Expected);

  GlobalType.setValType(WasmEdge::TypeCode::I31Ref);
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createGlobalSec(GlobalType), Output));
  Expected[3] = 0x6CU; // Opcode I31Ref
  EXPECT_EQ(Output, Expected);

  GlobalType.setValType(WasmEdge::TypeCode::StructRef);
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createGlobalSec(GlobalType), Output));
  Expected[3] = 0x6BU; // Opcode StructRef
  EXPECT_EQ(Output, Expected);

  GlobalType.setValType(WasmEdge::TypeCode::ArrayRef);
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createGlobalSec(GlobalType), Output));
  Expected[3] = 0x6AU; // Opcode ArrayRef
  EXPECT_EQ(Output, Expected);

  // Test without GC proposal
  Conf.removeProposal(WasmEdge::Proposal::GC);
  Output = {};
  EXPECT_FALSE(Ser.serializeSection(createGlobalSec(GlobalType), Output));

  // Tests for the previous types as RefTypes
  Conf.addProposal(WasmEdge::Proposal::GC);
  WasmEdge::AST::TableType TableType;
  TableType.setRefType(WasmEdge::TypeCode::NullFuncRef);
  TableType.getLimit().setMin(4294967295);
  TableType.getLimit().setType(WasmEdge::AST::Limit::LimitType::HasMin);

  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createTableSec(TableType), Output));
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
  EXPECT_TRUE(Ser.serializeSection(createTableSec(TableType), Output));
  Expected[3] = 0x72U; // NullExternRef type

  Output = {};
  TableType.setRefType(WasmEdge::TypeCode::NullRef);
  EXPECT_TRUE(Ser.serializeSection(createTableSec(TableType), Output));
  Expected[3] = 0x71U; // NullRef type

  Output = {};
  TableType.setRefType(WasmEdge::TypeCode::FuncRef);
  EXPECT_TRUE(Ser.serializeSection(createTableSec(TableType), Output));
  Expected[3] = 0x70U; // FuncRef type

  Output = {};
  TableType.setRefType(WasmEdge::TypeCode::ExternRef);
  EXPECT_TRUE(Ser.serializeSection(createTableSec(TableType), Output));
  Expected[3] = 0x6FU; // ExternRef type

  Output = {};
  TableType.setRefType(WasmEdge::TypeCode::AnyRef);
  EXPECT_TRUE(Ser.serializeSection(createTableSec(TableType), Output));
  Expected[3] = 0x6EU; // AnyRef type

  Output = {};
  TableType.setRefType(WasmEdge::TypeCode::EqRef);
  EXPECT_TRUE(Ser.serializeSection(createTableSec(TableType), Output));
  Expected[3] = 0x6DU; // EqRef type

  Output = {};
  TableType.setRefType(WasmEdge::TypeCode::I31Ref);
  EXPECT_TRUE(Ser.serializeSection(createTableSec(TableType), Output));
  Expected[3] = 0x6CU; // I31Ref type

  Output = {};
  TableType.setRefType(WasmEdge::TypeCode::StructRef);
  EXPECT_TRUE(Ser.serializeSection(createTableSec(TableType), Output));
  Expected[3] = 0x6BU; // StructRef type

  Output = {};
  TableType.setRefType(WasmEdge::TypeCode::ArrayRef);
  EXPECT_TRUE(Ser.serializeSection(createTableSec(TableType), Output));
  Expected[3] = 0x6AU; // ArrayRef type

  // Test Without GC proposal
  Conf.removeProposal(WasmEdge::Proposal::GC);
  Output = {};
  EXPECT_FALSE(Ser.serializeSection(createTableSec(TableType), Output));

  // Test I8 and I16 types
  Conf.addProposal(WasmEdge::Proposal::GC);
  GlobalType.setValType(WasmEdge::TypeCode::I8);
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createGlobalSec(GlobalType), Output));
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
  EXPECT_TRUE(Ser.serializeSection(createGlobalSec(GlobalType), Output));
  Expected[3] = 0x77U; // Opcode I16
  EXPECT_EQ(Output, Expected);

  Conf.removeProposal(WasmEdge::Proposal::GC);
  Output = {};
  EXPECT_FALSE(Ser.serializeSection(createGlobalSec(GlobalType), Output));

  Conf.addProposal(WasmEdge::Proposal::GC);
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
  EXPECT_TRUE(Ser.serializeSection(createGlobalSec(GlobalType), Output));
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
  EXPECT_TRUE(Ser.serializeSection(createGlobalSec(GlobalType), Output));
  EXPECT_EQ(Output, Expected);

  Conf.addProposal(WasmEdge::Proposal::ExceptionHandling);
  GlobalType.setValType(WasmEdge::TypeCode::ExnRef);
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createGlobalSec(GlobalType), Output));
  Expected = {
      0x06U, // Global section
      0x04U, // Content size = 4
      0x01U, // Vector length = 1
      0x69U, // ExnRef type
      0x00U, // Const mutation
      0x0BU  // Expression
  };
  EXPECT_EQ(Output, Expected);

  Conf.removeProposal(WasmEdge::Proposal::ExceptionHandling);
  Output = {};
  EXPECT_FALSE(Ser.serializeSection(createGlobalSec(GlobalType), Output));
}

TEST(serializeTypeTest, SerializeSubType) {
  std::vector<uint8_t> Output;
  std::vector<uint8_t> Expected;

  // 6. Test serialize SubType, CompositeType, RecuresiveType, and FieldTypes.
  //
  //   1. Test SubType (and CompositeType too) with final flag.
  //   2. Test SubType (CompositeType, and FieldType too) with final flag.
  //   3. Test SubType (CompositeType, and FieldType too) without final flag.
  //   4. Test non-final SubType with zero supertypes (emits 0x50 0x00).
  //   5. Test RecType (RecType ::= 0x4E vector(subtype)).
  //   6. Test RecType without GC proposal.

  WasmEdge::AST::SubType SubType;
  Conf.addProposal(WasmEdge::Proposal::GC);
  SubType.getCompositeType() =
      WasmEdge::AST::CompositeType(WasmEdge::AST::FunctionType());
  SubType.getSuperTypeIndices() = {0x01U, 0x02U, 0x03U};
  SubType.setFinal(true);
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createTypeSec(SubType), Output));
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
  EXPECT_TRUE(Ser.serializeSection(createTypeSec(SubType1), Output));
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
  EXPECT_TRUE(Ser.serializeSection(createTypeSec(SubType2), Output));
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
  EXPECT_TRUE(Ser.serializeSection(createTypeSec(SubType3), Output));
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
  EXPECT_TRUE(Ser.serializeSection(TypeSec, Output));
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

  Conf.removeProposal(WasmEdge::Proposal::GC);
  Output = {};
  EXPECT_FALSE(Ser.serializeSection(TypeSec, Output));
}

TEST(serializeTypeTest, SerializeCompositeType) {
  std::vector<uint8_t> Output;
  std::vector<uint8_t> Expected;

  //  7. Test Composite Types
  //
  //    1. Test CompositeType (Array).
  //    2. Test CompositeType (Array) without GC proposal.
  //    3. Test CompositeType (Struct).
  //    4. Test CompositeType (Struct) without GC proposal.

  WasmEdge::AST::SubType SubType;
  WasmEdge::AST::FieldType FType;
  WasmEdge::AST::CompositeType CompType;
  Conf.addProposal(WasmEdge::Proposal::GC);

  FType.setStorageType(WasmEdge::TypeCode::I8);
  FType.setValMut(WasmEdge::ValMut::Const);
  CompType.setArrayType(std::move(FType));
  Output = {};
  SubType.getCompositeType() = CompType;
  EXPECT_TRUE(Ser.serializeSection(createTypeSec(SubType), Output));
  Expected = {
      0x01U, // Type section
      0x04U, // Content size
      0x01U, // Vector length
      0x5EU, // Array type
      0x78U, // I8 type
      0x00U  // Const mutation
  };
  EXPECT_EQ(Output, Expected);

  Conf.removeProposal(WasmEdge::Proposal::GC);
  Output = {};
  EXPECT_FALSE(Ser.serializeSection(createTypeSec(SubType), Output));

  Conf.addProposal(WasmEdge::Proposal::GC);
  CompType.setStructType({FType, FType, FType});
  SubType.getCompositeType() = CompType;
  Output = {};
  EXPECT_TRUE(Ser.serializeSection(createTypeSec(SubType), Output));
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

  Conf.removeProposal(WasmEdge::Proposal::GC);
  Output = {};
  EXPECT_FALSE(Ser.serializeSection(createTypeSec(SubType), Output));
}

} // namespace
