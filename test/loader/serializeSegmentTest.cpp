/// Unit tests of serialize segments

#include "loader/serialize.h"

#include <cstdint>
#include <gtest/gtest.h>
#include <vector>

namespace {

WasmEdge::Loader::Serializer Ser;

TEST(SerializeSegmentTest, SerializeGlobalSegment) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;

  // 1. Test serialize global segment.
  //
  //   1.  Serialize global segment with expression of only End operation.
  //   2.  Serialize global segment with non-empty expression.

  WasmEdge::AST::GlobalSection GlobalSec;
  WasmEdge::AST::GlobalSegment GlobalSeg;

  GlobalSeg.getGlobalType() = WasmEdge::AST::GlobalType(WasmEdge::ValType::I32, WasmEdge::ValMut::Const);
  GlobalSeg.getExpr().getInstrs() = {
    WasmEdge::AST::Instruction(WasmEdge::OpCode::End)
  };
  GlobalSec.getContent() = {GlobalSeg};

  Output = Ser.serializeSection(GlobalSec);
  Expected = {
      0x06U,       // Global section
      0x04U,       // Content size = 4
      0x01U,       // Vector length = 1
      0x7FU, 0x00, // Global type
      0x0BU        // Expression
  };
  EXPECT_EQ(Output, Expected);

  GlobalSeg.getGlobalType() = WasmEdge::AST::GlobalType(WasmEdge::ValType::I32, WasmEdge::ValMut::Const);
  GlobalSeg.getExpr().getInstrs() = {
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__eqz),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__eq),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::I32__ne),
      WasmEdge::AST::Instruction(WasmEdge::OpCode::End)
  };
  GlobalSec.getContent() = {GlobalSeg};

  Output = Ser.serializeSection(GlobalSec);
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
  // TODO: add more tests.
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;

  // 2. Test load element segment.
  //
  //   1.  Load element segment with expression of only End operation and empty
  //       function indices list.
  //   2.  Load element segment with expression and function indices list.

  WasmEdge::AST::ElementSection ElementSec;
  WasmEdge::AST::ElementSegment ElementSeg;

  WasmEdge::AST::Instruction End(WasmEdge::OpCode::End);
  WasmEdge::AST::Instruction RefFunc(WasmEdge::OpCode::Ref__func);
  WasmEdge::AST::Instruction I32Eqz(WasmEdge::OpCode::I32__eqz);
  WasmEdge::AST::Instruction I32Eq(WasmEdge::OpCode::I32__eq);
  WasmEdge::AST::Instruction I32Ne(WasmEdge::OpCode::I32__ne);

  ElementSeg.setMode(WasmEdge::AST::ElementSegment::ElemMode::Active);
  ElementSeg.getExpr().getInstrs() = {
    End
  };
  ElementSec.getContent() = {ElementSeg};

  Output = Ser.serializeSection(ElementSec);
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
  ElementSeg.getExpr().getInstrs() = {
      I32Eqz, I32Eq, I32Ne,
      End
  };
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

  Output = Ser.serializeSection(ElementSec);
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
}

TEST(SerializeSegmentTest, SerializeCodeSegment) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;

  // 3. Test serialize code segment.
  //
  //   1.  Serialize code segment of empty locals and expression with only End
  //       operation.
  //   2.  Serialize code segment with expression and local lists.

  WasmEdge::AST::CodeSection CodeSec;
  WasmEdge::AST::CodeSegment CodeSeg;

  WasmEdge::AST::Instruction End(WasmEdge::OpCode::End);
  WasmEdge::AST::Instruction I32Eqz(WasmEdge::OpCode::I32__eqz);
  WasmEdge::AST::Instruction I32Eq(WasmEdge::OpCode::I32__eq);
  WasmEdge::AST::Instruction I32Ne(WasmEdge::OpCode::I32__ne);

  CodeSeg.setSegSize(2);
  CodeSeg.getExpr().getInstrs() = {
    End
  };
  CodeSec.getContent() = {CodeSeg};

  Output = Ser.serializeSection(CodeSec);
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
  CodeSeg.getLocals() = {
    {0x01U, WasmEdge::ValType::F64},
    {0x03U, WasmEdge::ValType::F32},
    {0x1FFFFFFU, WasmEdge::ValType::I64},
    {0x1FFFFF3U, WasmEdge::ValType::I32}
  };
  CodeSeg.getExpr().getInstrs() = {
      I32Eqz, I32Eq, I32Ne,
      End
  };
  CodeSec.getContent() = {CodeSeg};

  Output = Ser.serializeSection(CodeSec);
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
}

TEST(SerializeSegmentTest, SerializeDataSegment) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;

  // 4. Test serialize data segment.
  //
  //   1.  Serialize data segment of expression with only End operation and empty
  //       initialization data.
  //   2.  Serialize data segment with expression and initialization data.

  WasmEdge::AST::DataSection DataSec;
  WasmEdge::AST::DataSegment DataSeg;

  WasmEdge::AST::Instruction End(WasmEdge::OpCode::End);
  WasmEdge::AST::Instruction I32Eqz(WasmEdge::OpCode::I32__eqz);
  WasmEdge::AST::Instruction I32Eq(WasmEdge::OpCode::I32__eq);
  WasmEdge::AST::Instruction I32Ne(WasmEdge::OpCode::I32__ne);

  DataSeg.setMode(WasmEdge::AST::DataSegment::DataMode::Active);
  DataSeg.getExpr().getInstrs() = {
    End
  };
  DataSec.getContent() = {DataSeg};

  Output = Ser.serializeSection(DataSec);
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
  DataSeg.getExpr().getInstrs() = {
      I32Eqz, I32Eq, I32Ne,
      End
  };
  DataSeg.getData() = {'t', 'e', 's', 't'};
  DataSec.getContent() = {DataSeg};

  Output = Ser.serializeSection(DataSec);
  Expected = {
      0x0BU,                            // Data section
      0x0BU,                            // Content size = 11
      0x01U,                            // Vector length = 1
      0x00U,                            // Prefix checking byte
      0x45U, 0x46U, 0x47U, 0x0BU,       // Expression
      0x04U, 0x74U, 0x65U, 0x73U, 0x74U // Vector length = 4, "test"
  };
  EXPECT_EQ(Output, Expected);
}
}
