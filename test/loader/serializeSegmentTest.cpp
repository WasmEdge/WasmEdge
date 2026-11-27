// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "loader/serialize.h"

#include <cstdint>
#include <gtest/gtest.h>
#include <vector>

namespace {

WasmEdge::Loader::Serializer Ser;

TEST(SerializeSegmentTest, SerializeTableSegment) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;

  // 1. Test serialize table segment.
  //
  //   1.  Serialize plain table type (MVP form) with min-max limit.
  //   2.  Serialize plain table type (MVP form) with min-only limit and a
  //       different reference type.
  //   3.  Serialize table segment with init expression (the 0x40 0x00 form)
  //       using an expression of only the End operation.
  //   4.  Serialize table segment with init expression carrying a non-empty
  //       ref.func initializer.

  // 1. Plain table type, funcref, limit [0, 14]. No init expression is present,
  //    so only the table type (reftype + limit) is emitted.
  {
    WasmEdge::AST::TableSection TableSec;
    WasmEdge::AST::TableSegment TableSeg;
    TableSeg.getTableType() =
        WasmEdge::AST::TableType(WasmEdge::TypeCode::FuncRef, 0, 14);
    TableSec.getContent() = {TableSeg};

    Output = {};
    Ser.serializeSection(TableSec, Output);
    Expected = {
        0x04U,                     // Table section
        0x05U,                     // Content size = 5
        0x01U,                     // Vector length = 1
        0x70U, 0x01U, 0x00U, 0x0EU // Table type: funcref + limit [0, 14]
    };
    EXPECT_EQ(Output, Expected);
  }

  // 2. Plain table type, externref, min-only limit [5]. Exercises the no-max
  //    limit flag (0x00) and a non-funcref reference type.
  {
    WasmEdge::AST::TableSection TableSec;
    WasmEdge::AST::TableSegment TableSeg;
    TableSeg.getTableType() =
        WasmEdge::AST::TableType(WasmEdge::TypeCode::ExternRef, 5);
    TableSec.getContent() = {TableSeg};

    Output = {};
    Ser.serializeSection(TableSec, Output);
    Expected = {
        0x04U,              // Table section
        0x04U,              // Content size = 4
        0x01U,              // Vector length = 1
        0x6FU, 0x00U, 0x05U // Table type: externref + limit [5]
    };
    EXPECT_EQ(Output, Expected);
  }

  // 3. Table segment with init expression. A
  //    non-empty expression triggers the 0x40 0x00 prefix followed by the table
  //    type and the expression.
  {
    WasmEdge::AST::TableSection TableSec;
    WasmEdge::AST::TableSegment TableSeg;
    TableSeg.getTableType() =
        WasmEdge::AST::TableType(WasmEdge::TypeCode::FuncRef, 0);
    TableSeg.getExpr().getInstrs().appendEnd();
    TableSec.getContent() = {TableSeg};

    Output = {};
    Ser.serializeSection(TableSec, Output);
    Expected = {
        0x04U,               // Table section
        0x07U,               // Content size = 7
        0x01U,               // Vector length = 1
        0x40U, 0x00U,        // Init-expression form prefix
        0x70U, 0x00U, 0x00U, // Table type: funcref + limit [0]
        0x0BU                // Expression
    };
    EXPECT_EQ(Output, Expected);
  }

  // 4. Table segment with init expression carrying a ref.func initializer and a
  //    min-max limit [1, 16].
  {
    WasmEdge::AST::TableSection TableSec;
    WasmEdge::AST::TableSegment TableSeg;
    TableSeg.getTableType() =
        WasmEdge::AST::TableType(WasmEdge::TypeCode::FuncRef, 1, 16);
    TableSeg.getExpr().getInstrs().appendIndex(WasmEdge::OpCode::Ref__func,
                                               0x00U);
    TableSeg.getExpr().getInstrs().appendEnd();
    TableSec.getContent() = {TableSeg};

    Output = {};
    Ser.serializeSection(TableSec, Output);
    Expected = {
        0x04U,                      // Table section
        0x0AU,                      // Content size = 10
        0x01U,                      // Vector length = 1
        0x40U, 0x00U,               // Init-expression form prefix
        0x70U, 0x01U, 0x01U, 0x10U, // Table type: funcref + limit [1, 16]
        0xD2U, 0x00U,               // ref.func 0
        0x0BU                       // End
    };
    EXPECT_EQ(Output, Expected);
  }
}

TEST(SerializeSegmentTest, SerializeGlobalSegment) {
  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;

  // 2. Test serialize global segment.
  //
  //   1.  Serialize global segment with expression of only End operation.
  //   2.  Serialize global segment with non-empty expression.

  WasmEdge::AST::GlobalSection GlobalSec;
  WasmEdge::AST::GlobalSegment GlobalSeg;

  GlobalSeg.getGlobalType() = WasmEdge::AST::GlobalType(
      WasmEdge::TypeCode::I32, WasmEdge::ValMut::Const);
  GlobalSeg.getExpr().getInstrs() = {};
  GlobalSeg.getExpr().getInstrs().appendEnd();
  GlobalSec.getContent() = {GlobalSeg};

  Output = {};
  Ser.serializeSection(GlobalSec, Output);
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
  GlobalSeg.getExpr().getInstrs() = {};
  GlobalSeg.getExpr().getInstrs().append(WasmEdge::OpCode::I32__eqz);
  GlobalSeg.getExpr().getInstrs().append(WasmEdge::OpCode::I32__eq);
  GlobalSeg.getExpr().getInstrs().append(WasmEdge::OpCode::I32__ne);
  GlobalSeg.getExpr().getInstrs().appendEnd();
  GlobalSec.getContent() = {GlobalSeg};

  Output = {};
  Ser.serializeSection(GlobalSec, Output);
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

  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;

  // 3. Test serialize element segment.
  //
  //   1.  Serialize element segment with expression of only End operation and
  //       empty function indices list.
  //   2.  Serialize element segment with expression and function indices list.
  //   3.  Serialize element segment with element kind and function indices
  //       list.
  //   4.  Serialize passive and declarative element segments with a non-zero
  //       table index.
  //   5.  Serialize element segment with expression, element kind and function
  //       indices list.
  //   6.  Serialize element segment with element kind and function indices
  //       list.
  //   7.  Serialize element segment with offset expression and init expression
  //       list.
  //   8.  Serialize element segment with reference type and init expression
  //       list.
  //   9.  Serialize element segment with table index, offset expression,
  //       reference type and init expression list.
  //   10.  Serialize element segment with reference type and init expression
  //       list.

  WasmEdge::AST::ElementSection ElementSec;
  WasmEdge::AST::ElementSegment ElementSeg;
  ElementSeg.setRefType(
      WasmEdge::ValType(WasmEdge::TypeCode::Ref, WasmEdge::TypeCode::FuncRef));

  ElementSeg.setMode(WasmEdge::AST::ElementSegment::ElemMode::Active);
  ElementSeg.getExpr().getInstrs() = {};
  ElementSeg.getExpr().getInstrs().appendEnd();
  ElementSec.getContent() = {ElementSeg};

  Output = {};
  Ser.serializeSection(ElementSec, Output);
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
  ElementSeg.getExpr().getInstrs() = {};
  ElementSeg.getExpr().getInstrs().append(WasmEdge::OpCode::I32__eqz);
  ElementSeg.getExpr().getInstrs().append(WasmEdge::OpCode::I32__eq);
  ElementSeg.getExpr().getInstrs().append(WasmEdge::OpCode::I32__ne);
  ElementSeg.getExpr().getInstrs().appendEnd();
  ElementSeg.getInitExprs().emplace_back();
  ElementSeg.getInitExprs().back().getInstrs().appendIndex(
      WasmEdge::OpCode::Ref__func, 0xFFFFFFFFU);
  ElementSeg.getInitExprs().back().getInstrs().appendEnd();
  ElementSeg.getInitExprs().emplace_back();
  ElementSeg.getInitExprs().back().getInstrs().appendIndex(
      WasmEdge::OpCode::Ref__func, 0x00U);
  ElementSeg.getInitExprs().back().getInstrs().appendEnd();
  ElementSeg.getInitExprs().emplace_back();
  ElementSeg.getInitExprs().back().getInstrs().appendIndex(
      WasmEdge::OpCode::Ref__func, 12345U);
  ElementSeg.getInitExprs().back().getInstrs().appendEnd();
  ElementSec.getContent() = {ElementSeg};

  Output = {};
  Ser.serializeSection(ElementSec, Output);
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
  Ser.serializeSection(ElementSec, Output);
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

  ElementSeg.setMode(WasmEdge::AST::ElementSegment::ElemMode::Passive);
  ElementSeg.setIdx(0x01U);
  ElementSec.getContent() = {ElementSeg};

  Output = {};
  Ser.serializeSection(ElementSec, Output);
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

  ElementSeg.setMode(WasmEdge::AST::ElementSegment::ElemMode::Declarative);
  ElementSec.getContent() = {ElementSeg};

  Output = {};
  Ser.serializeSection(ElementSec, Output);
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
  ElementSeg.setIdx(0x01U);
  ElementSeg.getExpr().getInstrs() = {};
  ElementSeg.getExpr().getInstrs().append(WasmEdge::OpCode::I32__eqz);
  ElementSeg.getExpr().getInstrs().append(WasmEdge::OpCode::I32__eq);
  ElementSeg.getExpr().getInstrs().append(WasmEdge::OpCode::I32__ne);
  ElementSeg.getExpr().getInstrs().appendEnd();
  ElementSec.getContent() = {ElementSeg};

  Output = {};
  Ser.serializeSection(ElementSec, Output);
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
  Ser.serializeSection(ElementSec, Output);
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
  Ser.serializeSection(ElementSec, Output);
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
  ElementSeg.setRefType(WasmEdge::TypeCode::FuncRef);
  ElementSeg.getExpr().getInstrs() = {};
  ElementSeg.getExpr().getInstrs().append(WasmEdge::OpCode::I32__eqz);
  ElementSeg.getExpr().getInstrs().append(WasmEdge::OpCode::I32__eq);
  ElementSeg.getExpr().getInstrs().append(WasmEdge::OpCode::I32__ne);
  ElementSeg.getExpr().getInstrs().appendEnd();
  ElementSeg.getInitExprs().clear();
  ElementSeg.getInitExprs().emplace_back();
  ElementSeg.getInitExprs().back().getInstrs().append(
      WasmEdge::OpCode::I32__eqz);
  ElementSeg.getInitExprs().back().getInstrs().append(
      WasmEdge::OpCode::I32__eq);
  ElementSeg.getInitExprs().back().getInstrs().append(
      WasmEdge::OpCode::I32__ne);
  ElementSeg.getInitExprs().back().getInstrs().appendEnd();
  ElementSec.getContent() = {ElementSeg};

  Output = {};
  Ser.serializeSection(ElementSec, Output);
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
  Ser.serializeSection(ElementSec, Output);
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
  ElementSeg.getExpr().getInstrs() = {};
  ElementSeg.getExpr().getInstrs().append(WasmEdge::OpCode::I32__eqz);
  ElementSeg.getExpr().getInstrs().append(WasmEdge::OpCode::I32__eq);
  ElementSeg.getExpr().getInstrs().append(WasmEdge::OpCode::I32__ne);
  ElementSeg.getExpr().getInstrs().appendEnd();
  ElementSec.getContent() = {ElementSeg};

  Output = {};
  Ser.serializeSection(ElementSec, Output);
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
  Ser.serializeSection(ElementSec, Output);
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
}

TEST(SerializeSegmentTest, SerializeElementSegmentAllModes) {
  // 4. Pin every element segment mode. Modes 0x00 to 0x03 mean (ref func) and
  // mode 0x04 means funcref, and 0x00 and 0x04 also imply table index 0.

  const WasmEdge::ValType NonNullFuncRef(WasmEdge::TypeCode::Ref,
                                         WasmEdge::TypeCode::FuncRef);
  const WasmEdge::ValType FuncRef(WasmEdge::TypeCode::RefNull,
                                  WasmEdge::TypeCode::FuncRef);
  const WasmEdge::ValType ExternRef(WasmEdge::TypeCode::RefNull,
                                    WasmEdge::TypeCode::ExternRef);

  auto refFunc = [](uint32_t Idx) {
    WasmEdge::AST::Expression Expr;
    Expr.getInstrs().appendIndex(WasmEdge::OpCode::Ref__func, Idx);
    Expr.getInstrs().appendEnd();
    return Expr;
  };
  auto refNull = [](const WasmEdge::ValType &VType) {
    WasmEdge::AST::Expression Expr;
    Expr.getInstrs().appendValType(WasmEdge::OpCode::Ref__null, VType);
    Expr.getInstrs().appendEnd();
    return Expr;
  };
  auto offset = []() {
    WasmEdge::AST::Expression Expr;
    Expr.getInstrs().appendNum(WasmEdge::OpCode::I32__const,
                               static_cast<uint32_t>(0));
    Expr.getInstrs().appendEnd();
    return Expr;
  };

  auto serialize = [](const WasmEdge::AST::ElementSegment &Seg) {
    WasmEdge::AST::ElementSection Sec;
    Sec.getContent() = {Seg};
    std::vector<uint8_t> Output;
    Ser.serializeSection(Sec, Output);
    // Drop the section id, section size and vector length.
    return std::vector<uint8_t>(Output.begin() + 3, Output.end());
  };

  using ElemMode = WasmEdge::AST::ElementSegment::ElemMode;

  // Mode 0x00: active on table 0, (ref func), ref.func initialisers.
  {
    WasmEdge::AST::ElementSegment Seg;
    Seg.setMode(ElemMode::Active);
    Seg.setIdx(0);
    Seg.setRefType(NonNullFuncRef);
    Seg.getExpr() = offset();
    Seg.getInitExprs() = {refFunc(1)};
    EXPECT_EQ(serialize(Seg),
              std::vector<uint8_t>({0x00U, 0x41U, 0x00U, 0x0BU, 0x01U, 0x01U}));
  }
  // Mode 0x01: passive, (ref func), ref.func initialisers.
  {
    WasmEdge::AST::ElementSegment Seg;
    Seg.setMode(ElemMode::Passive);
    Seg.setRefType(NonNullFuncRef);
    Seg.getInitExprs() = {refFunc(1)};
    EXPECT_EQ(serialize(Seg),
              std::vector<uint8_t>({0x01U, 0x00U, 0x01U, 0x01U}));
  }
  // Mode 0x02: active on a table other than 0.
  {
    WasmEdge::AST::ElementSegment Seg;
    Seg.setMode(ElemMode::Active);
    Seg.setIdx(2);
    Seg.setRefType(NonNullFuncRef);
    Seg.getExpr() = offset();
    Seg.getInitExprs() = {refFunc(1)};
    EXPECT_EQ(serialize(Seg),
              std::vector<uint8_t>(
                  {0x02U, 0x02U, 0x41U, 0x00U, 0x0BU, 0x00U, 0x01U, 0x01U}));
  }
  // Mode 0x03: declarative, (ref func), ref.func initialisers.
  {
    WasmEdge::AST::ElementSegment Seg;
    Seg.setMode(ElemMode::Declarative);
    Seg.setRefType(NonNullFuncRef);
    Seg.getInitExprs() = {refFunc(1)};
    EXPECT_EQ(serialize(Seg),
              std::vector<uint8_t>({0x03U, 0x00U, 0x01U, 0x01U}));
  }
  // Mode 0x04: active on table 0, funcref, expression initialisers.
  {
    WasmEdge::AST::ElementSegment Seg;
    Seg.setMode(ElemMode::Active);
    Seg.setIdx(0);
    Seg.setRefType(FuncRef);
    Seg.getExpr() = offset();
    Seg.getInitExprs() = {refNull(FuncRef)};
    EXPECT_EQ(serialize(Seg),
              std::vector<uint8_t>(
                  {0x04U, 0x41U, 0x00U, 0x0BU, 0x01U, 0xD0U, 0x70U, 0x0BU}));
  }
  // Mode 0x05: passive with an explicit reference type.
  {
    WasmEdge::AST::ElementSegment Seg;
    Seg.setMode(ElemMode::Passive);
    Seg.setRefType(ExternRef);
    Seg.getInitExprs() = {refNull(ExternRef)};
    EXPECT_EQ(serialize(Seg),
              std::vector<uint8_t>({0x05U, 0x6FU, 0x01U, 0xD0U, 0x6FU, 0x0BU}));
  }
  // Mode 0x06: active with an explicit reference type.
  {
    WasmEdge::AST::ElementSegment Seg;
    Seg.setMode(ElemMode::Active);
    Seg.setIdx(2);
    Seg.setRefType(ExternRef);
    Seg.getExpr() = offset();
    Seg.getInitExprs() = {refNull(ExternRef)};
    EXPECT_EQ(serialize(Seg),
              std::vector<uint8_t>({0x06U, 0x02U, 0x41U, 0x00U, 0x0BU, 0x6FU,
                                    0x01U, 0xD0U, 0x6FU, 0x0BU}));
  }
  // Mode 0x07: declarative with an explicit reference type.
  {
    WasmEdge::AST::ElementSegment Seg;
    Seg.setMode(ElemMode::Declarative);
    Seg.setRefType(ExternRef);
    Seg.getInitExprs() = {refNull(ExternRef)};
    EXPECT_EQ(serialize(Seg),
              std::vector<uint8_t>({0x07U, 0x6FU, 0x01U, 0xD0U, 0x6FU, 0x0BU}));
  }

  // An empty externref segment must keep its element type instead of being
  // written as elemkind 0x00, which means funcref.
  {
    WasmEdge::AST::ElementSegment Seg;
    Seg.setMode(ElemMode::Passive);
    Seg.setRefType(ExternRef);
    EXPECT_EQ(serialize(Seg), std::vector<uint8_t>({0x05U, 0x6FU, 0x00U}));
  }
  // An active externref segment on table 0 cannot use mode 0x04, whose element
  // type is funcref, so it falls back to mode 0x06 with an explicit index.
  {
    WasmEdge::AST::ElementSegment Seg;
    Seg.setMode(ElemMode::Active);
    Seg.setIdx(0);
    Seg.setRefType(ExternRef);
    Seg.getExpr() = offset();
    Seg.getInitExprs() = {refNull(ExternRef)};
    EXPECT_EQ(serialize(Seg),
              std::vector<uint8_t>({0x06U, 0x00U, 0x41U, 0x00U, 0x0BU, 0x6FU,
                                    0x01U, 0xD0U, 0x6FU, 0x0BU}));
  }
  // An empty (ref func) segment uses elemkind too.
  {
    WasmEdge::AST::ElementSegment Seg;
    Seg.setMode(ElemMode::Passive);
    Seg.setRefType(NonNullFuncRef);
    EXPECT_EQ(serialize(Seg), std::vector<uint8_t>({0x01U, 0x00U, 0x00U}));
  }
  // A funcref segment of ref.func initialisers keeps its type in modes 0x04 to
  // 0x07, as elemkind means (ref func).
  {
    WasmEdge::AST::ElementSegment Seg;
    Seg.setMode(ElemMode::Active);
    Seg.setIdx(0);
    Seg.setRefType(FuncRef);
    Seg.getExpr() = offset();
    Seg.getInitExprs() = {refFunc(1)};
    EXPECT_EQ(serialize(Seg),
              std::vector<uint8_t>(
                  {0x04U, 0x41U, 0x00U, 0x0BU, 0x01U, 0xD2U, 0x01U, 0x0BU}));
  }
  {
    WasmEdge::AST::ElementSegment Seg;
    Seg.setMode(ElemMode::Passive);
    Seg.setRefType(FuncRef);
    Seg.getInitExprs() = {refFunc(1)};
    EXPECT_EQ(serialize(Seg),
              std::vector<uint8_t>({0x05U, 0x70U, 0x01U, 0xD2U, 0x01U, 0x0BU}));
  }
  {
    WasmEdge::AST::ElementSegment Seg;
    Seg.setMode(ElemMode::Passive);
    Seg.setRefType(FuncRef);
    EXPECT_EQ(serialize(Seg), std::vector<uint8_t>({0x05U, 0x70U, 0x00U}));
  }
  {
    WasmEdge::AST::ElementSegment Seg;
    Seg.setMode(ElemMode::Active);
    Seg.setIdx(2);
    Seg.setRefType(FuncRef);
    Seg.getExpr() = offset();
    Seg.getInitExprs() = {refFunc(1)};
    EXPECT_EQ(serialize(Seg),
              std::vector<uint8_t>({0x06U, 0x02U, 0x41U, 0x00U, 0x0BU, 0x70U,
                                    0x01U, 0xD2U, 0x01U, 0x0BU}));
  }
  {
    WasmEdge::AST::ElementSegment Seg;
    Seg.setMode(ElemMode::Declarative);
    Seg.setRefType(FuncRef);
    Seg.getInitExprs() = {refFunc(1)};
    EXPECT_EQ(serialize(Seg),
              std::vector<uint8_t>({0x07U, 0x70U, 0x01U, 0xD2U, 0x01U, 0x0BU}));
  }
}

TEST(SerializeSegmentTest, SerializeCodeSegment) {

  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;

  // 5. Test serialize code segment.
  //
  //   1.  Serialize code segment of empty locals and expression with only End
  //       operation.
  //   2.  Serialize code segment with expression and local lists.

  WasmEdge::AST::CodeSection CodeSec;
  WasmEdge::AST::CodeSegment CodeSeg;

  CodeSeg.setSegSize(2);
  CodeSeg.getExpr().getInstrs() = {};
  CodeSeg.getExpr().getInstrs().appendEnd();
  CodeSec.getContent() = {CodeSeg};

  Output = {};
  Ser.serializeSection(CodeSec, Output);
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
  CodeSeg.getExpr().getInstrs() = {};
  CodeSeg.getExpr().getInstrs().append(WasmEdge::OpCode::I32__eqz);
  CodeSeg.getExpr().getInstrs().append(WasmEdge::OpCode::I32__eq);
  CodeSeg.getExpr().getInstrs().append(WasmEdge::OpCode::I32__ne);
  CodeSeg.getExpr().getInstrs().appendEnd();
  CodeSec.getContent() = {CodeSeg};

  Output = {};
  Ser.serializeSection(CodeSec, Output);
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
}

TEST(SerializeSegmentTest, SerializeDataSegment) {

  std::vector<uint8_t> Expected;
  std::vector<uint8_t> Output;

  // 6. Test serialize data segment.
  //
  //   1.  Serialize data segment of expression with only End operation and
  //       empty initialization data.
  //   2.  Serialize data segment with expression and initialization data.

  WasmEdge::AST::DataSection DataSec;
  WasmEdge::AST::DataSegment DataSeg;

  DataSeg.setMode(WasmEdge::AST::DataSegment::DataMode::Active);
  DataSeg.getExpr().getInstrs() = {};
  DataSeg.getExpr().getInstrs().appendEnd();
  DataSec.getContent() = {DataSeg};

  Output = {};
  Ser.serializeSection(DataSec, Output);
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
  DataSeg.getExpr().getInstrs() = {};
  DataSeg.getExpr().getInstrs().append(WasmEdge::OpCode::I32__eqz);
  DataSeg.getExpr().getInstrs().append(WasmEdge::OpCode::I32__eq);
  DataSeg.getExpr().getInstrs().append(WasmEdge::OpCode::I32__ne);
  DataSeg.getExpr().getInstrs().appendEnd();
  DataSeg.getData() = {'t', 'e', 's', 't'};
  DataSec.getContent() = {DataSeg};

  Output = {};
  Ser.serializeSection(DataSec, Output);
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
  Ser.serializeSection(DataSec, Output);
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
  DataSeg.getExpr().getInstrs() = {};
  DataSeg.getExpr().getInstrs().append(WasmEdge::OpCode::I32__eqz);
  DataSeg.getExpr().getInstrs().append(WasmEdge::OpCode::I32__eq);
  DataSeg.getExpr().getInstrs().append(WasmEdge::OpCode::I32__ne);
  DataSeg.getExpr().getInstrs().appendEnd();
  DataSec.getContent() = {DataSeg};

  Output = {};
  Ser.serializeSection(DataSec, Output);
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
}

} // namespace
