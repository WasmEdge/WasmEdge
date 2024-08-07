// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "loader/serialize.h"

#include <cstdint>
#include <gtest/gtest.h>
#include <vector>

namespace {

WasmEdge::Configure Conf;
WasmEdge::Loader::Serializer Ser(Conf);

TEST(SerializeSegmentTest, SerializeGlobalSegment) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;

  // 1. Test serialize global segment.
  //
  //   1.  Serialize global segment with expression of only End operation.
  //   2.  Serialize global segment with non-empty expression.

  WasmEdge::AST::GlobalSection GlobalSec;
  WasmEdge::AST::GlobalSegment GlobalSeg;

  GlobalSeg.getGlobalType() = WasmEdge::AST::GlobalType(
      WasmEdge::TypeCode::I32, WasmEdge::ValMut::Const);
  GlobalSeg.getExpr().getInstrs() = {
      WasmEdge::AST::Instruction(WasmEdge::OpCode::End)};
  GlobalSec.getContent() = {GlobalSeg};

  Output = {};
  EXPECT_TRUE(Ser.serializeSection(GlobalSec, Output));
  Expected = {
      0x06U,       // Global section
      0x04U,       // Content size = 4
      0x01U,       // Vector length = 1
      0x7FU, 0x00, // Global type
      0x0BU        // Expression
  };
  EXPECT_EQ(Output, Expected);

  GlobalSeg.getGlobalType() = WasmEdge::AST::GlobalType(
      WasmEdge::TypeCode::I32, WasmEdge::ValMut::Const);
  GlobalSeg.getExpr().getInstrs() = {
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__eqz),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__eq),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__ne),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::End)};
  GlobalSec.getContent() = {GlobalSeg};

  Output = {};
  EXPECT_TRUE(Ser.serializeSection(GlobalSec, Output));
  Expected = {
      0x06U,                     // Global section
      0x07U,                     // Content size = 7
      0x01U,                     // Vector length = 1
      0x7FU, 0x00U,              // Global type
      0x45U, 0x46U, 0x47U, 0x0BU // Expression
  };
  EXPECT_EQ(Output, Expected);
}

TEST(SerializeSegmentTest, SerializeElementSegment) {
  WasmEdge::Configure ConfNoRefType;
  ConfNoRefType.removeProposal(WasmEdge::Proposal::BulkMemoryOperations);
  ConfNoRefType.removeProposal(WasmEdge::Proposal::ReferenceTypes);
  WasmEdge::Loader::Serializer SerNoRefType(ConfNoRefType);

  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;

  // 2. Test serialize element segment.
  //
  //   1.  Serialize element segment with expression of only End operation and
  //       empty function indices list.
  //   2.  Serialize element segment with expression and function indices list.
  //   3.  Serialize element segment with element kind and function indices
  //       list.
  //   4.  Serialize element segment with expression, element kind and function
  //       indices list.
  //   5.  Serialize element segment with element kind and function indices
  //       list.
  //   6.  Serialize element segment with offset expression and init expression
  //       list.
  //   7.  Serialize element segment with reference type and init expression
  //       list.
  //   8.  Serialize element segment with table index, offset expression,
  //       reference type and init expression list.
  //   9.  Serialize element segment with reference type and init expression
  //       list.
  //   10. Serialize element segment with invalid checking byte without
  //       Ref-Types proposal.

  WasmEdge::AST::ElementSection ElementSec;
  WasmEdge::AST::ElementSegment ElementSeg;

  WasmEdge::AST::Instruction End(WasmEdge::OpCode::End);
  WasmEdge::AST::Instruction RefFunc(WasmEdge::OpCode::Ref__func);
  WasmEdge::AST::Instruction I32Eqz(WasmEdge::OpCode::I32__eqz);
  WasmEdge::AST::Instruction I32Eq(WasmEdge::OpCode::I32__eq);
  WasmEdge::AST::Instruction I32Ne(WasmEdge::OpCode::I32__ne);

  ElementSeg.setMode(WasmEdge::AST::ElementSegment::ElemMode::Active);
  ElementSeg.getExpr().getInstrs() = {End};
  ElementSec.getContent() = {ElementSeg};

  Output = {};
  EXPECT_TRUE(Ser.serializeSection(ElementSec, Output));
  Expected = {
      0x09U, // Element section
      0x04U, // Content size = 4
      0x01U, // Vector length = 1
      0x00U, // Prefix checking byte
      0x0BU, // Offset expression
      0x00U  // Function indices list
  };
  EXPECT_EQ(Output, Expected);

  ElementSeg.setMode(WasmEdge::AST::ElementSegment::ElemMode::Active);
  ElementSeg.getExpr().getInstrs() = {I32Eqz, I32Eq, I32Ne, End};
  RefFunc.getTargetIndex() = 0xFFFFFFFFU;
  ElementSeg.getInitExprs().emplace_back();
  ElementSeg.getInitExprs().back().getInstrs().emplace_back(std::move(RefFunc));
  ElementSeg.getInitExprs().back().getInstrs().emplace_back(std::move(End));
  RefFunc.getTargetIndex() = 0x00U;
  ElementSeg.getInitExprs().emplace_back();
  ElementSeg.getInitExprs().back().getInstrs().emplace_back(std::move(RefFunc));
  ElementSeg.getInitExprs().back().getInstrs().emplace_back(std::move(End));
  RefFunc.getTargetIndex() = 12345U;
  ElementSeg.getInitExprs().emplace_back();
  ElementSeg.getInitExprs().back().getInstrs().emplace_back(std::move(RefFunc));
  ElementSeg.getInitExprs().back().getInstrs().emplace_back(std::move(End));
  ElementSec.getContent() = {ElementSeg};

  Output = {};
  EXPECT_TRUE(Ser.serializeSection(ElementSec, Output));
  Expected = {
      0x09U,                             // Element section
      0x0FU,                             // Content size = 15
      0x01U,                             // Vector length = 1
      0x00U,                             // Prefix checking byte
      0x45U, 0x46U, 0x47U, 0x0BU,        // Offset expression
      0x03U,                             // Vector length = 3
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU, // vec[0]
      0x00U,                             // vec[1]
      0xB9U, 0x60U                       // vec[2]
  };
  EXPECT_EQ(Output, Expected);

  ElementSeg.setMode(WasmEdge::AST::ElementSegment::ElemMode::Passive);
  ElementSeg.getExpr().getInstrs().clear();
  ElementSec.getContent() = {ElementSeg};

  Output = {};
  EXPECT_TRUE(Ser.serializeSection(ElementSec, Output));
  Expected = {
      0x09U,                             // Element section
      0x0CU,                             // Content size = 12
      0x01U,                             // Vector length = 1
      0x01U,                             // Prefix checking byte
      0x00U,                             // ElementKind
      0x03U,                             // Vector length = 3
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU, // vec[0]
      0x00U,                             // vec[1]
      0xB9U, 0x60U                       // vec[2]
  };
  EXPECT_EQ(Output, Expected);

  ElementSeg.setMode(WasmEdge::AST::ElementSegment::ElemMode::Active);
  ElementSeg.setIdx(0x01U);
  ElementSeg.getExpr().getInstrs() = {I32Eqz, I32Eq, I32Ne, End};
  ElementSec.getContent() = {ElementSeg};

  Output = {};
  EXPECT_TRUE(Ser.serializeSection(ElementSec, Output));
  Expected = {
      0x09U,                             // Element section
      0x11U,                             // Content size = 17
      0x01U,                             // Vector length = 1
      0x02U,                             // Prefix checking byte
      0x01U,                             // TableIdx
      0x45U, 0x46U, 0x47U, 0x0BU,        // Offset expression
      0x00U,                             // ElementKind
      0x03U,                             // Vector length = 3
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU, // vec[0]
      0x00U,                             // vec[1]
      0xB9U, 0x60U                       // vec[2]
  };
  EXPECT_EQ(Output, Expected);

  ElementSeg.setMode(WasmEdge::AST::ElementSegment::ElemMode::Declarative);
  ElementSeg.setIdx(0x00U);
  ElementSeg.getExpr().getInstrs().clear();
  ElementSec.getContent() = {ElementSeg};

  Output = {};
  EXPECT_TRUE(Ser.serializeSection(ElementSec, Output));
  Expected = {
      0x09U,                             // Element section
      0x0CU,                             // Content size = 12
      0x01U,                             // Vector length = 1
      0x03U,                             // Prefix checking byte
      0x00U,                             // ElementKind
      0x03U,                             // Vector length = 3
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU, // vec[0]
      0x00U,                             // vec[1]
      0xB9U, 0x60U                       // vec[2]
  };
  EXPECT_EQ(Output, Expected);

  ElementSeg.setMode(WasmEdge::AST::ElementSegment::ElemMode::Declarative);
  ElementSeg.setIdx(0x00U);
  ElementSeg.getExpr().getInstrs().clear();
  ElementSec.getContent() = {ElementSeg};

  Output = {};
  EXPECT_TRUE(Ser.serializeSection(ElementSec, Output));
  Expected = {
      0x09U,                             // Element section
      0x0CU,                             // Content size = 12
      0x01U,                             // Vector length = 1
      0x03U,                             // Prefix checking byte
      0x00U,                             // ElementKind
      0x03U,                             // Vector length = 3
      0xFFU, 0xFFU, 0xFFU, 0xFFU, 0x0FU, // vec[0]
      0x00U,                             // vec[1]
      0xB9U, 0x60U                       // vec[2]
  };
  EXPECT_EQ(Output, Expected);

  ElementSeg.setMode(WasmEdge::AST::ElementSegment::ElemMode::Active);
  ElementSeg.getExpr().getInstrs() = {I32Eqz, I32Eq, I32Ne, End};
  ElementSeg.getInitExprs().clear();
  ElementSeg.getInitExprs().emplace_back();
  ElementSeg.getInitExprs().back().getInstrs() = {I32Eqz, I32Eq, I32Ne, End};
  ElementSec.getContent() = {ElementSeg};

  Output = {};
  EXPECT_TRUE(Ser.serializeSection(ElementSec, Output));
  Expected = {
      0x09U,                      // Element section
      0x0BU,                      // Content size = 11
      0x01U,                      // Vector length = 1
      0x04U,                      // Prefix checking byte
      0x45U, 0x46U, 0x47U, 0x0BU, // Offset expression
      0x01U,                      // Vector length = 1
      0x45U, 0x46U, 0x47U, 0x0BU, // Vec[0]
  };
  EXPECT_EQ(Output, Expected);

  ElementSeg.setMode(WasmEdge::AST::ElementSegment::ElemMode::Passive);
  ElementSeg.getExpr().getInstrs().clear();
  ElementSeg.setRefType(WasmEdge::TypeCode::ExternRef);
  ElementSec.getContent() = {ElementSeg};

  Output = {};
  EXPECT_TRUE(Ser.serializeSection(ElementSec, Output));
  Expected = {
      0x09U,                      // Element section
      0x08U,                      // Content size = 8
      0x01U,                      // Vector length = 1
      0x05U,                      // Prefix checking byte
      0x6FU,                      // RefType
      0x01U,                      // Vector length = 1
      0x45U, 0x46U, 0x47U, 0x0BU, // Vec[0]
  };
  EXPECT_EQ(Output, Expected);

  ElementSeg.setMode(WasmEdge::AST::ElementSegment::ElemMode::Active);
  ElementSeg.setIdx(0x01U);
  ElementSeg.getExpr().getInstrs() = {I32Eqz, I32Eq, I32Ne, End};
  ElementSec.getContent() = {ElementSeg};

  Output = {};
  EXPECT_TRUE(Ser.serializeSection(ElementSec, Output));
  Expected = {
      0x09U,                      // Element section
      0x0DU,                      // Content size = 13
      0x01U,                      // Vector length = 1
      0x06U,                      // Prefix checking byte
      0x01U,                      // TableIdx
      0x45U, 0x46U, 0x47U, 0x0BU, // Offset Expression
      0x6FU,                      // RefType
      0x01U,                      // Vector length = 1
      0x45U, 0x46U, 0x47U, 0x0BU, // Vec[0]
  };
  EXPECT_EQ(Output, Expected);

  ElementSeg.setMode(WasmEdge::AST::ElementSegment::ElemMode::Declarative);
  ElementSeg.setIdx(0x00U);
  ElementSeg.getExpr().getInstrs().clear();
  ElementSec.getContent() = {ElementSeg};

  Output = {};
  EXPECT_TRUE(Ser.serializeSection(ElementSec, Output));
  Expected = {
      0x09U,                      // Element section
      0x08U,                      // Content size = 8
      0x01U,                      // Vector length = 1
      0x07U,                      // Prefix checking byte
      0x6FU,                      // RefType
      0x01U,                      // Vector length = 1
      0x45U, 0x46U, 0x47U, 0x0BU, // Vec[0]
  };
  EXPECT_EQ(Output, Expected);

  EXPECT_FALSE(SerNoRefType.serializeSection(ElementSec, Output));
}

TEST(SerializeSegmentTest, SerializeCodeSegment) {
  WasmEdge::Configure ConfNoRefType;
  ConfNoRefType.removeProposal(WasmEdge::Proposal::BulkMemoryOperations);
  ConfNoRefType.removeProposal(WasmEdge::Proposal::ReferenceTypes);
  WasmEdge::Loader::Serializer SerNoRefType(ConfNoRefType);

  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;

  // 3. Test serialize code segment.
  //
  //   1.  Serialize code segment of empty locals and expression with only End
  //       operation.
  //   2.  Serialize code segment with expression and local lists.
  //   3.  Serialize code segment with invalid local number type without
  //       Ref-Types proposal.

  WasmEdge::AST::CodeSection CodeSec;
  WasmEdge::AST::CodeSegment CodeSeg;

  WasmEdge::AST::Instruction End(WasmEdge::OpCode::End);
  WasmEdge::AST::Instruction I32Eqz(WasmEdge::OpCode::I32__eqz);
  WasmEdge::AST::Instruction I32Eq(WasmEdge::OpCode::I32__eq);
  WasmEdge::AST::Instruction I32Ne(WasmEdge::OpCode::I32__ne);

  CodeSeg.setSegSize(2);
  CodeSeg.getExpr().getInstrs() = {End};
  CodeSec.getContent() = {CodeSeg};

  Output = {};
  EXPECT_TRUE(Ser.serializeSection(CodeSec, Output));
  Expected = {
      0x0AU, // Code section
      0x04U, // Content size = 4
      0x01U, // Vector length = 1
      0x02U, // Code segment size = 2
      0x00U, // Vector length = 0
      0x0BU  // Expression
  };
  EXPECT_EQ(Output, Expected);

  CodeSeg.setSegSize(19);
  CodeSeg.getLocals() = {{0x01U, WasmEdge::TypeCode::F64},
                         {0x03U, WasmEdge::TypeCode::F32},
                         {0x1FFFFFFU, WasmEdge::TypeCode::I64},
                         {0x1FFFFF3U, WasmEdge::TypeCode::I32}};
  CodeSeg.getExpr().getInstrs() = {I32Eqz, I32Eq, I32Ne, End};
  CodeSec.getContent() = {CodeSeg};

  Output = {};
  EXPECT_TRUE(Ser.serializeSection(CodeSec, Output));
  Expected = {
      0x0AU,                             // Code section
      0x15U,                             // Content size = 21
      0x01U,                             // Vector length = 1
      0x13U,                             // Code segment size = 19
      0x04U,                             // Vector length = 4
      0x01U, 0x7CU,                      // vec[0]
      0x03U, 0x7DU,                      // vec[1]
      0xFFU, 0xFFU, 0xFFU, 0x0FU, 0x7EU, // vec[2]
      0xF3U, 0xFFU, 0xFFU, 0x0FU, 0x7FU, // vec[3]
      0x45U, 0x46U, 0x47U, 0x0BU         // Expression
  };
  EXPECT_EQ(Output, Expected);

  CodeSeg.getLocals() = {{0x01U, WasmEdge::TypeCode::ExternRef}};
  CodeSec.getContent() = {CodeSeg};
  EXPECT_FALSE(SerNoRefType.serializeSection(CodeSec, Output));
}

TEST(SerializeSegmentTest, SerializeDataSegment) {
  WasmEdge::Configure ConfNoRefType;
  ConfNoRefType.removeProposal(WasmEdge::Proposal::BulkMemoryOperations);
  ConfNoRefType.removeProposal(WasmEdge::Proposal::ReferenceTypes);
  WasmEdge::Loader::Serializer SerNoRefType(ConfNoRefType);

  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;

  // 4. Test serialize data segment.
  //
  //   1.  Serialize data segment of expression with only End operation and
  //       empty
  //       initialization data.
  //   2.  Serialize data segment with expression and initialization data.
  //   3.  Serialize data segment with invalid checking byte without Bulk-Mem
  //       proposal.

  WasmEdge::AST::DataSection DataSec;
  WasmEdge::AST::DataSegment DataSeg;

  WasmEdge::AST::Instruction End(WasmEdge::OpCode::End);
  WasmEdge::AST::Instruction I32Eqz(WasmEdge::OpCode::I32__eqz);
  WasmEdge::AST::Instruction I32Eq(WasmEdge::OpCode::I32__eq);
  WasmEdge::AST::Instruction I32Ne(WasmEdge::OpCode::I32__ne);

  DataSeg.setMode(WasmEdge::AST::DataSegment::DataMode::Active);
  DataSeg.getExpr().getInstrs() = {End};
  DataSec.getContent() = {DataSeg};

  Output = {};
  EXPECT_TRUE(Ser.serializeSection(DataSec, Output));
  Expected = {
      0x0BU, // Data section
      0x04U, // Content size = 4
      0x01U, // Vector length = 1
      0x00U, // Prefix checking byte
      0x0BU, // Expression
      0x00U  // Vector length = 0
  };
  EXPECT_EQ(Output, Expected);

  DataSeg.setMode(WasmEdge::AST::DataSegment::DataMode::Active);
  DataSeg.getExpr().getInstrs() = {I32Eqz, I32Eq, I32Ne, End};
  DataSeg.getData() = {'t', 'e', 's', 't'};
  DataSec.getContent() = {DataSeg};

  Output = {};
  EXPECT_TRUE(Ser.serializeSection(DataSec, Output));
  Expected = {
      0x0BU,                            // Data section
      0x0BU,                            // Content size = 11
      0x01U,                            // Vector length = 1
      0x00U,                            // Prefix checking byte
      0x45U, 0x46U, 0x47U, 0x0BU,       // Expression
      0x04U, 0x74U, 0x65U, 0x73U, 0x74U // Vector length = 4, "test"
  };
  EXPECT_EQ(Output, Expected);

  DataSeg.setMode(WasmEdge::AST::DataSegment::DataMode::Passive);
  DataSeg.getExpr().getInstrs().clear();
  DataSec.getContent() = {DataSeg};

  Output = {};
  EXPECT_TRUE(Ser.serializeSection(DataSec, Output));
  Expected = {
      0x0BU,                            // Data section
      0x07U,                            // Content size = 7
      0x01U,                            // Vector length = 1
      0x01U,                            // Prefix checking byte
      0x04U, 0x74U, 0x65U, 0x73U, 0x74U // Vector length = 4, "test"
  };
  EXPECT_EQ(Output, Expected);

  DataSeg.setMode(WasmEdge::AST::DataSegment::DataMode::Active);
  DataSeg.setIdx(0x01U);
  DataSeg.getExpr().getInstrs() = {I32Eqz, I32Eq, I32Ne, End};
  DataSec.getContent() = {DataSeg};

  Output = {};
  EXPECT_TRUE(Ser.serializeSection(DataSec, Output));
  Expected = {
      0x0BU,                            // Data section
      0x0CU,                            // Content size = 12
      0x01U,                            // Vector length = 1
      0x02U,                            // Prefix checking byte
      0x01U,                            // MemoryIdx
      0x45U, 0x46U, 0x47U, 0x0BU,       // Expression
      0x04U, 0x74U, 0x65U, 0x73U, 0x74U // Vector length = 4, "test"
  };
  EXPECT_EQ(Output, Expected);

  EXPECT_FALSE(SerNoRefType.serializeSection(DataSec, Output));
}
} // namespace
