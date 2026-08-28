// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/test/wat/converterTest.cpp - WAT converter unit tests ----===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// These unit tests send small WAT snippets through WAT::parseWat. They cover
/// the converter behavior that is difficult to write as a .wast spec
/// assertion. Such behavior is the literal ranges, the strict grammar, and the
/// strength against untrusted input. parseWat only converts, and it does not
/// validate.
/// As a result, each negative test asserts the specific ErrCode. A wrong code
/// then gives a mismatch, and not a has_value() test that passes.
///
//===----------------------------------------------------------------------===//

#include "wat/parser.h"

#include "common/configure.h"
#include "common/errcode.h"
#include "common/spdlog.h"

#include <clocale>
#include <gtest/gtest.h>
#include <memory>
#include <spdlog/sinks/ostream_sink.h>
#include <sstream>
#include <string>
#include <string_view>

namespace {

using namespace std::literals;
using WasmEdge::ErrCode;

// Parse with the default Configure. The result is true if the conversion is
// successful.
bool ok(std::string_view S) {
  WasmEdge::Configure Conf;
  return WasmEdge::WAT::parseWat(S, Conf).has_value();
}
// Parse with the default Configure and give the module. The test fails if
// the conversion is not successful.
WasmEdge::AST::Module parse(std::string_view S) {
  WasmEdge::Configure Conf;
  auto Res = WasmEdge::WAT::parseWat(S, Conf);
  EXPECT_TRUE(Res.has_value()) << S;
  return Res ? std::move(*Res) : WasmEdge::AST::Module();
}
// Find the first instruction with the opcode Code in the code section. The
// result is null if there is no such instruction.
const WasmEdge::AST::Instruction *findInstr(const WasmEdge::AST::Module &Mod,
                                            WasmEdge::OpCode Code) {
  for (const auto &Seg : Mod.getCodeSection().getContent()) {
    for (const auto &Instr : Seg.getExpr().getInstrs()) {
      if (Instr.getOpCode() == Code) {
        return &Instr;
      }
    }
  }
  return nullptr;
}
// Give the error code of a conversion that failed. If the conversion is
// successful, the result is Success.
ErrCode::Value err(std::string_view S) {
  WasmEdge::Configure Conf;
  auto Res = WasmEdge::WAT::parseWat(S, Conf);
  return Res ? ErrCode::Value::Success : Res.error().getEnum();
}
// Give the error code of a conversion with the Configure Conf. If the
// conversion is successful, the result is Success.
ErrCode::Value errConf(const WasmEdge::Configure &Conf, std::string_view S) {
  auto Res = WasmEdge::WAT::parseWat(S, Conf);
  return Res ? ErrCode::Value::Success : Res.error().getEnum();
}
// Make a Configure without the given proposals. The removal follows the order
// of the list, because a proposal can depend on another proposal.
WasmEdge::Configure
without(std::initializer_list<WasmEdge::Proposal> Proposals) {
  WasmEdge::Configure Conf;
  for (auto P : Proposals) {
    Conf.removeProposal(P);
  }
  return Conf;
}

//===----------------------------------------------------------------------===//
// Numeric literals and index/value ranges.
//===----------------------------------------------------------------------===//

// A numeric index accepts hexadecimal digits and underscore separators.
TEST(WatConverterTest, IndexLiterals) {
  EXPECT_TRUE(ok("(module (func) (func (call 0x0)))"));
  EXPECT_TRUE(ok("(module (func) (func (call 1_0)) (func) (func) (func) (func) "
                 "(func) (func) (func) (func) (func))"));
  EXPECT_TRUE(ok("(module (func) (func (call 0x0_0)))"));
  // The converter rejects a numeric index that is too large for a uint32, and
  // it does not wrap the index.
  EXPECT_EQ(err("(module (func (call 0xFFFFFFFFFF)))"),
            ErrCode::Value::InvalidFuncIdx);
  EXPECT_EQ(err("(module (func (call 99999999999)))"),
            ErrCode::Value::InvalidFuncIdx);
}

// The converter examines the range of a numeric branch label before the narrow
// cast to u32. A label of 2^32 is an unknown label, and it is not a br 0 that
// wrapped.
TEST(WatConverterTest, LabelIndexRange) {
  EXPECT_EQ(err("(module (func (block (br 4294967296))))"),
            ErrCode::Value::WatUnknownLabel);
  EXPECT_TRUE(ok("(module (func (block (br 0))))"));
}

// The converter examines the range of a numeric struct field index before the
// narrow cast to u32.
TEST(WatConverterTest, StructFieldIndexRange) {
  EXPECT_EQ(err("(module (type $s (struct (field i32))) "
                "(func (param (ref $s)) (struct.get $s 4294967296 "
                "(local.get 0)) drop))"),
            ErrCode::Value::WatUnknownId);
}

// The converter examines the range of the element count of array.new_fixed
// before the narrow cast to u32.
TEST(WatConverterTest, ArrayNewFixedCountRange) {
  EXPECT_EQ(err("(module (type $a (array i32)) "
                "(func (array.new_fixed $a 4294967296) drop))"),
            ErrCode::Value::WatConstantOutOfRange);
}

// A float literal reads the same in every locale. An embedder can call
// setlocale with a comma-decimal locale, and the decimal point of the text
// format stays a period. The test skips if no such locale exists.
TEST(WatConverterTest, FloatLiteralsIgnoreLocale) {
  const char *Prev = std::setlocale(LC_NUMERIC, nullptr);
  const std::string Saved = Prev ? Prev : "C";
  if (std::setlocale(LC_NUMERIC, "de_DE.UTF-8") == nullptr &&
      std::setlocale(LC_NUMERIC, "de_DE.utf8") == nullptr) {
    GTEST_SKIP() << "no comma-decimal locale";
  }
  auto Res =
      WasmEdge::WAT::parseWat("(module (func (result f32) (f32.const 1.5)) "
                              "(func (result f64) (f64.const 0x1.8p1)) "
                              "(func (result f64) (f64.const 2.5e1)))",
                              WasmEdge::Configure());
  std::setlocale(LC_NUMERIC, Saved.c_str());
  ASSERT_TRUE(Res.has_value());
  const auto &Code = Res->getCodeSection().getContent();
  ASSERT_EQ(Code.size(), 3U);
  EXPECT_EQ(Code[0].getExpr().getInstrs()[0].getNum().get<float>(), 1.5f);
  EXPECT_EQ(Code[1].getExpr().getInstrs()[0].getNum().get<double>(), 3.0);
  EXPECT_EQ(Code[2].getExpr().getInstrs()[0].getNum().get<double>(), 25.0);
}

// The converter rejects an f32 nan payload that is larger than the mantissa,
// and it does not truncate the payload.
TEST(WatConverterTest, NanPayloadRange) {
  EXPECT_EQ(err("(module (func (result f32) (f32.const nan:0x100000001)))"),
            ErrCode::Value::WatConstantOutOfRange);
  EXPECT_TRUE(ok("(module (func (result f32) (f32.const nan:0x1)))"));
}

//===----------------------------------------------------------------------===//
// String literals.
//===----------------------------------------------------------------------===//

TEST(WatConverterTest, StringLiterals) {
  // The decoder of the \u{...} escape rejects an escaped surrogate code point.
  // This test comes before the UTF-8 test on the raw bytes.
  EXPECT_EQ(err("(module (func (export \"\\u{d800}\")))"),
            ErrCode::Value::WatMalformedString);
  // The tokenizer rejects raw UTF-8 bytes in the source that are not valid.
  EXPECT_EQ(err("(module (func (export \"\xED\xA0\x80\")))"),
            ErrCode::Value::MalformedUTF8);
  EXPECT_EQ(err("(module (func (export \"\xC0\")))"),
            ErrCode::Value::MalformedUTF8);
}

// The converter rejects a raw control byte with no escape inside a string
// literal. The escaped form holds the same byte, and the converter accepts it.
TEST(WatConverterTest, RawControlCharInString) {
  EXPECT_EQ(err("(module (memory 1) (data (i32.const 0) \"a\x01\x62\"))"),
            ErrCode::Value::WatMalformedString);
  EXPECT_TRUE(ok("(module (memory 1) (data (i32.const 0) \"a\\01b\"))"));
}

// The converter joins the data string literals that have whitespace between
// them. Two adjacent literals with no gap, such as "a""b", are malformed,
// because WAT needs whitespace between two tokens.
TEST(WatConverterTest, DataStringConcatenation) {
  EXPECT_TRUE(ok("(module (memory 1) (data (i32.const 0) \"a\" \"b\"))"));
  EXPECT_TRUE(ok("(module (memory 1) (data (i32.const 0) \"a\" \"b\" \"c\"))"));
  EXPECT_FALSE(ok("(module (memory 1) (data (i32.const 0) \"a\"\"b\"))"));
}

//===----------------------------------------------------------------------===//
// Type definitions and grammar strictness.
//===----------------------------------------------------------------------===//

// A named struct field takes exactly one field type, and an array takes
// exactly one field type. A field with no name takes a run of field types.
TEST(WatConverterTest, GcFieldCardinality) {
  EXPECT_EQ(err("(module (type (struct (field $x))))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (type (struct (field $x i32 i64))))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (type (array i32 i64)))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (type (array)))"), ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (type (array (mut i32) i64)))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_TRUE(ok("(module (type (struct (field $x i32))))"));
  EXPECT_TRUE(ok("(module (type (struct (field i32 i64))))"));
  EXPECT_TRUE(ok("(module (type (struct (field))))"));
  EXPECT_TRUE(ok("(module (type (array i32)))"));
  EXPECT_TRUE(ok("(module (type (array (mut i8))))"));
  // A named field takes the index of its own field, and a run of unnamed
  // field types after it keeps the count right.
  auto Mod = parse("(module (type $s (struct (field i32) (field $x i64) "
                   "(field f32 f64))) "
                   "(func (param (ref $s)) (result i64) "
                   "(struct.get $s $x (local.get 0))))");
  const auto *Get = findInstr(Mod, WasmEdge::OpCode::Struct__get);
  ASSERT_NE(Get, nullptr);
  EXPECT_EQ(Get->getSourceIndex(), 1U);
  ASSERT_EQ(Mod.getTypeSection().getContent().size(), 2U);
  EXPECT_EQ(Mod.getTypeSection()
                .getContent()[0]
                .getCompositeType()
                .getFieldTypes()
                .size(),
            4U);
}

// A structtype has no abbreviation without the "field" keyword. Only an
// arraytype takes a bare fieldtype. As a result, the converter rejects a
// struct member that is not a (field ...) group.
TEST(WatConverterTest, GcStructRequiresField) {
  EXPECT_EQ(err("(module (type (struct (mut i32))))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (type (struct i32)))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (type (struct (ref func))))"),
            ErrCode::Value::WatUnexpectedToken);
  // The converter accepts the canonical (field ...) form. This form can hold a
  // (mut ...) fieldtype.
  EXPECT_TRUE(ok("(module (type (struct (field (mut i32)))))"));
}

// A typeuse must point to a function type. A struct type or an array type
// gives a clean WatUnknownType. It does not dereference a null getFuncType().
TEST(WatConverterTest, TypeUseMustBeFuncType) {
  EXPECT_EQ(err("(module (type $s (struct)) (func (type $s)))"),
            ErrCode::Value::WatUnknownType);
  EXPECT_EQ(err("(module (type $s (struct)) (func (type $s) (param i32)))"),
            ErrCode::Value::WatUnknownType);
  EXPECT_EQ(err("(module (type $a (array i32)) (func (type $a)))"),
            ErrCode::Value::WatUnknownType);
  EXPECT_TRUE(ok("(module (type $f (func (param i32))) (func (type $f)))"));
}

// A (type ...) with no type definition is malformed, and the converter rejects
// it. A silent drop makes the type indices that come after it wrong.
TEST(WatConverterTest, MalformedTypedef) {
  EXPECT_FALSE(ok("(module (type $t))"));
  EXPECT_FALSE(ok("(module (type))"));
  EXPECT_TRUE(ok("(module (type $t (func)))"));
}

// The converter rejects a (ref ...) type with a trailing operand.
TEST(WatConverterTest, RefTypeTrailingOperand) {
  EXPECT_EQ(err("(module (func (param (ref null func extern))))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_TRUE(ok("(module (func (param (ref null func))))"));
}

// This test covers the grammar of a local declaration. A named local takes one
// id and one valtype. A local with no name takes a run of valtypes.
TEST(WatConverterTest, LocalDeclGrammar) {
  EXPECT_FALSE(ok("(module (func (local $x)))"));
  EXPECT_FALSE(ok("(module (func (local $x $y i32)))"));
  EXPECT_TRUE(ok("(module (func (local $x i32)))"));
  EXPECT_TRUE(ok("(module (func (local i32 i64)))"));
  EXPECT_TRUE(ok("(module (func (local)))"));
}

// The converter rejects the tokens that come after the index of an export.
TEST(WatConverterTest, ExportTrailingTokens) {
  EXPECT_EQ(err("(module (func) (export \"a\" (func 0 1)))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_TRUE(ok("(module (func) (export \"a\" (func 0)))"));
}

//===----------------------------------------------------------------------===//
// Type uses.
//===----------------------------------------------------------------------===//

// A parameter id after an explicit (type $t) is a local name. The printers
// emit this shape for every function with a name section.
TEST(WatConverterTest, ParamIdAfterExplicitType) {
  using WasmEdge::OpCode;
  auto Mod = parse("(module (type $t (func (param i32) (result i32))) "
                   "(func $inc (type $t) (param $x i32) (result i32) "
                   "(local.get $x)))");
  const auto *Get = findInstr(Mod, OpCode::Local__get);
  ASSERT_NE(Get, nullptr);
  EXPECT_EQ(Get->getTargetIndex(), 0U);
  Mod = parse("(module (type $t (func (param i32 i64))) "
              "(func (type $t) (param $x i32) (param $y i64) (local $z f32) "
              "(local.get $z)))");
  Get = findInstr(Mod, OpCode::Local__get);
  ASSERT_NE(Get, nullptr);
  EXPECT_EQ(Get->getTargetIndex(), 2U);
}

// The printers emit a flat init expression after a table type, and an
// (item ...) elemexpr inside the inline elem of a table.
TEST(WatConverterTest, TableInitForms) {
  using WasmEdge::OpCode;
  auto Mod = parse("(module (func $g) (table 1 funcref ref.func $g))");
  ASSERT_EQ(Mod.getTableSection().getContent().size(), 1U);
  const auto &Init = Mod.getTableSection().getContent()[0].getExpr();
  ASSERT_EQ(Init.getInstrs().size(), 2U);
  EXPECT_EQ(Init.getInstrs()[0].getOpCode(), OpCode::Ref__func);
  Mod = parse("(module (func $g) "
              "(table funcref (elem (item (ref.func $g)) (ref.func $g))))");
  ASSERT_EQ(Mod.getElementSection().getContent().size(), 1U);
  EXPECT_EQ(Mod.getElementSection().getContent()[0].getInitExprs().size(), 2U);
  EXPECT_EQ(err("(module (func $g) (table funcref (elem $g (ref.func $g))))"),
            ErrCode::Value::WatUnexpectedToken);
}

// A (param ...) with an id takes exactly one valtype. A (param ...) with no
// id takes a run of valtypes. The two forms do not mix, as with (local ...).
TEST(WatConverterTest, ParamDeclGrammar) {
  EXPECT_EQ(err("(module (func (param $x)))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (func (param $x) (param $y i32) (result i32) "
                "(local.get $x)))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (func (param $x i32 i64)))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (func (param i32 $x)))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (type (func (param $x i32 i64))))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (func) (func (call_indirect (param $x i32) "
                "(i32.const 0))))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_TRUE(ok("(module (func (param $x i32) (param i32 i64)))"));
  EXPECT_TRUE(ok("(module (func (param) (param $x i32)))"));
}

// A typeuse takes at most one (type ...), and the (type ...) holds exactly
// one index.
TEST(WatConverterTest, TypeUseGrammar) {
  EXPECT_EQ(err("(module (type $t (func)) (type $u (func (result i32))) "
                "(func (type $t) (type $u) (i32.const 1)))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (type $t (func)) (type $u (func)) "
                "(func (type $t $u)))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (type $t (func)) (func (type $t (foo))))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (type $t (func)) (func (type)))"),
            ErrCode::Value::WatUnexpectedToken);
}

// A function type holds only (param ...) and (result ...) groups. The
// converter rejects any other child, and it does not drop it.
TEST(WatConverterTest, FuncTypeGrammar) {
  EXPECT_EQ(err("(module (type $t (func i32 (result i32))))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (type $t (func (local i32))))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (type (func) x))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (rec x (type (func))))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (rec (func)))"), ErrCode::Value::WatUnexpectedToken);
  EXPECT_TRUE(ok("(module (type $t (func (param i32) (result i32))))"));
}

// A block type holds at most one (type ...), and the clause holds exactly
// one index, as a typeuse does.
TEST(WatConverterTest, BlockTypeUseGrammar) {
  EXPECT_EQ(err("(module (type $a (func)) (type $b (func (result i32))) "
                "(func (result i32) (block (type $a) (type $b) "
                "(i32.const 1))))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (type $t (func)) (func (block (type $t 0))))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (type $t (func)) (func (block (type))))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (type $t (func)) (func block (type $t 0) end))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_TRUE(ok("(module (type $t (func)) (func (block (type $t))))"));
}

// A block type with a (type $b) and an inline result compares the inline
// type with the type $b. A type with the same structure before $b does not
// change the result, and the converter does not append a type.
TEST(WatConverterTest, BlockTypeWithExplicitTypeUse) {
  auto Mod = parse("(module (type $a (func (result i32))) "
                   "(type $b (func (result i32))) "
                   "(func (result i32) (block (type $b) (result i32) "
                   "(i32.const 1))))");
  EXPECT_EQ(Mod.getTypeSection().getContent().size(), 2U);
  EXPECT_EQ(err("(module (type $a (func (result i32))) "
                "(func (result i32) (block (type $a) (result i64) "
                "(i64.const 1))))"),
            ErrCode::Value::WatInlineFuncType);
  EXPECT_EQ(err("(module (type $a (struct)) (func (block (type $a))))"),
            ErrCode::Value::WatUnknownType);
}

// An implicit function type from an inline typeuse takes the next type index
// after the explicit types, in the order of the source. A numeric type index
// can point forward to such a type.
TEST(WatConverterTest, ForwardNumericReferenceToImplicitType) {
  auto Mod = parse("(module (type (func)) "
                   "(func (type 1) (i32.const 3)) "
                   "(func (param i32) (result i32) (local.get 0)))");
  ASSERT_EQ(Mod.getTypeSection().getContent().size(), 2U);
  EXPECT_EQ(Mod.getFunctionSection().getContent()[0], 1U);
  EXPECT_EQ(Mod.getFunctionSection().getContent()[1], 1U);
  // The implicit types come in the order of the source. A block type and a
  // call_indirect typeuse make implicit types too.
  Mod = parse("(module "
              "(func (block (param i32) (result i32) (i32.const 1)) drop) "
              "(func (call_indirect (param i64) (i64.const 1) (i32.const 0))) "
              "(func (type 2) (param i64)) "
              "(table 1 funcref))");
  ASSERT_EQ(Mod.getTypeSection().getContent().size(), 3U);
  EXPECT_EQ(Mod.getFunctionSection().getContent()[2], 2U);
  // A numeric type index that is out of range is not a syntax error. The
  // validator reports it, as it does for a binary module.
  EXPECT_TRUE(ok("(module (type (func)) (func (type 2)))"));
  EXPECT_EQ(err("(module (type (func)) (func (type $t)))"),
            ErrCode::Value::WatUnknownId);
}

//===----------------------------------------------------------------------===//
// Control flow and block structure.
//===----------------------------------------------------------------------===//

// A flat try_table, which is not folded, converts to a well-formed
// instruction. The instruction holds its control metadata. The metadata can
// hold a catch_all clause that points to an outer block label.
TEST(WatConverterTest, FlatTryTable) {
  EXPECT_TRUE(ok("(module (func try_table end))"));
  EXPECT_TRUE(ok("(module (func (result i32) "
                 "try_table (result i32) (i32.const 0) end))"));
  EXPECT_TRUE(ok("(module (func block try_table (catch_all 0) end end))"));
}

// The converter rejects an 'else' that has no outer 'if'.
TEST(WatConverterTest, StrayElse) {
  EXPECT_EQ(err("(module (func else))"), ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (func block else end))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_TRUE(ok("(module (func if else end))"));
}

// The converter rejects a flat block that has no 'end'.
TEST(WatConverterTest, UnclosedBlock) {
  EXPECT_EQ(err("(module (func block))"), ErrCode::Value::WatUnexpectedEnd);
  EXPECT_TRUE(ok("(module (func block end))"));
}

// The depth of the folded instructions has no bound. The converter walks
// the tree with an explicit stack, as the binary loader does, so deep folded
// nesting cannot overflow the native stack. A br_table ladder from a printer
// with the folded option nests one block for each case.
TEST(WatConverterTest, DeepNestingAccepted) {
  auto Nest = [](int N, std::string_view Open) {
    std::string S = "(module (func ";
    for (int I = 0; I < N; ++I) {
      S += Open;
    }
    for (int I = 0; I < N; ++I) {
      S += ")";
    }
    return S + "))";
  };
  EXPECT_TRUE(ok(Nest(20000, "(block "sv)));
  EXPECT_TRUE(ok(Nest(20000, "(loop "sv)));
  std::string Folded = "(module (func (result i32) ";
  for (int I = 0; I < 20000; ++I) {
    Folded += "(i32.add (i32.const 1) ";
  }
  Folded += "(i32.const 1)";
  for (int I = 0; I < 20000; ++I) {
    Folded += ")";
  }
  Folded += "))";
  EXPECT_TRUE(ok(Folded));
  std::string If = "(module (func ";
  for (int I = 0; I < 20000; ++I) {
    If += "(if (i32.const 1) (then ";
  }
  for (int I = 0; I < 20000; ++I) {
    If += "))";
  }
  If += "))";
  EXPECT_TRUE(ok(If));
}

//===----------------------------------------------------------------------===//
// The section forms: memory, table, and data.
//===----------------------------------------------------------------------===//

// The converter parses a 64-bit index type together with a shared limit.
TEST(WatConverterTest, MemoryForms) {
  WasmEdge::Configure Conf;
  Conf.addProposal(WasmEdge::Proposal::Threads);
  EXPECT_EQ(errConf(Conf, "(module (memory i64 1 2 shared))"),
            ErrCode::Value::Success);
}

// The converter rejects a memory that has no limits and no inline data.
TEST(WatConverterTest, MemoryRequiresLimitsOrData) {
  EXPECT_EQ(err("(module (memory))"), ErrCode::Value::WatUnexpectedEnd);
  EXPECT_FALSE(ok("(module (memory bogus))"));
  EXPECT_TRUE(ok("(module (memory 1))"));
  EXPECT_TRUE(ok("(module (memory (data \"abc\")))"));
}

// A table accepts the two explicit address types, i32 and i64.
TEST(WatConverterTest, TableAddressType) {
  EXPECT_TRUE(ok("(module (table i32 1 10 funcref))"));
  EXPECT_TRUE(ok("(module (table i64 1 10 funcref))"));
}

// This test covers active data with an explicit (memory 0) target and an i64
// offset expression.
TEST(WatConverterTest, DataForms) {
  EXPECT_TRUE(ok("(module (memory i64 1) "
                 "(data (memory 0) (i64.const 0) \"x\"))"));
}

//===----------------------------------------------------------------------===//
// The elem and data segments.
//===----------------------------------------------------------------------===//

// The inline (elem ...) of a table and the inline (data ...) of a memory make
// a segment. The segments after them take the next index, so a segment id
// after an inline segment resolves to the correct index.
TEST(WatConverterTest, InlineSegmentsTakeAnIndex) {
  using WasmEdge::OpCode;
  auto Mod = parse("(module (type $ft (func (result i32))) "
                   "(func $f (type $ft) (i32.const 1)) "
                   "(func $g (type $ft) (i32.const 2)) "
                   "(table $t funcref (elem $f)) "
                   "(elem $e func $g) "
                   "(table $t2 1 funcref) "
                   "(func (table.init $t2 $e (i32.const 0) (i32.const 0) "
                   "(i32.const 1))))");
  ASSERT_EQ(Mod.getElementSection().getContent().size(), 2U);
  const auto *Init = findInstr(Mod, OpCode::Table__init);
  ASSERT_NE(Init, nullptr);
  EXPECT_EQ(Init->getSourceIndex(), 1U);
  EXPECT_EQ(Init->getTargetIndex(), 1U);

  Mod = parse("(module (memory (data \"\\01\")) (data $d \"\\02\") "
              "(func (memory.init $d (i32.const 0) (i32.const 0) "
              "(i32.const 1))))");
  ASSERT_EQ(Mod.getDataSection().getContent().size(), 2U);
  const auto *MemInit = findInstr(Mod, OpCode::Memory__init);
  ASSERT_NE(MemInit, nullptr);
  EXPECT_EQ(MemInit->getSourceIndex(), 1U);
}

// The id after elem or data is the id of the segment, and not a table or a
// memory. A legacy numeric index is the table or the memory. Two segments
// with one id are malformed.
TEST(WatConverterTest, SegmentIdIsNotATarget) {
  auto Mod = parse("(module (memory $m 1) (memory $n 0) "
                   "(data $n (i32.const 0) \"x\"))");
  ASSERT_EQ(Mod.getDataSection().getContent().size(), 1U);
  EXPECT_EQ(Mod.getDataSection().getContent()[0].getIdx(), 0U);

  Mod = parse("(module (table $t 1 externref) (table $u 1 funcref) (func $g) "
              "(elem 1 (i32.const 0) $g))");
  ASSERT_EQ(Mod.getElementSection().getContent().size(), 1U);
  EXPECT_EQ(Mod.getElementSection().getContent()[0].getIdx(), 1U);

  Mod = parse("(module (table $t 1 funcref) (table $u 1 funcref) (func $g) "
              "(elem $u (table $u) (i32.const 0) func $g))");
  ASSERT_EQ(Mod.getElementSection().getContent().size(), 1U);
  EXPECT_EQ(Mod.getElementSection().getContent()[0].getIdx(), 1U);

  EXPECT_EQ(err("(module (elem $e func) (elem $e func))"),
            ErrCode::Value::WatDuplicateId);
  EXPECT_EQ(err("(module (data $d \"a\") (data $d \"b\"))"),
            ErrCode::Value::WatDuplicateId);
}

// Before the Bulk Memory and Reference Types proposals, a segment has no id.
// The id after elem or data is then the table or the memory. The wasm 1.0
// spec suite uses this form.
TEST(WatConverterTest, SegmentIdIsATargetInWasm1) {
  WasmEdge::Configure Conf;
  Conf.setWASMStandard(WasmEdge::Standard::WASM_1);
  EXPECT_EQ(errConf(Conf, "(module (table $t 10 funcref) (func $f) "
                          "(elem $t (i32.const 0) $f) "
                          "(elem $t (offset (i32.const 0)) $f $f))"),
            ErrCode::Value::Success);
  EXPECT_EQ(errConf(Conf, "(module (memory $m 1) "
                          "(data $m (i32.const 0) \"a\") "
                          "(data $m (offset (i32.const 0)) \"b\"))"),
            ErrCode::Value::Success);
  EXPECT_EQ(errConf(Conf, "(module (func $f) (elem $t (i32.const 0) $f))"),
            ErrCode::Value::WatUnknownId);
}

// A table use needs an offset. An active segment with no explicit reftype
// holds (ref func) references. The type of the table does not change it.
TEST(WatConverterTest, ActiveElemForms) {
  using WasmEdge::TypeCode;
  using WasmEdge::ValType;
  EXPECT_EQ(err("(module (table $t 1 funcref) (func $f) "
                "(elem (table $t) func $f))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (table 1 funcref) (func $f) (elem (table 0) $f))"),
            ErrCode::Value::WatUnexpectedToken);
  auto Mod = parse("(module (type $t (func)) (func $f (type $t)) "
                   "(table 1 (ref null $t)) (elem (i32.const 0) $f))");
  ASSERT_EQ(Mod.getElementSection().getContent().size(), 1U);
  EXPECT_EQ(Mod.getElementSection().getContent()[0].getRefType(),
            ValType(TypeCode::Ref, TypeCode::FuncRef));
  Mod = parse("(module (func $f) (table 1 funcref) "
              "(elem (i32.const 0) funcref (ref.func $f)))");
  ASSERT_EQ(Mod.getElementSection().getContent().size(), 1U);
  EXPECT_EQ(Mod.getElementSection().getContent()[0].getRefType(),
            ValType(TypeCode::FuncRef));
}

// One folded instruction can stand for the offset. The operands of the
// instruction come before it in the expression.
TEST(WatConverterTest, FoldedOffsetWithOperands) {
  using WasmEdge::OpCode;
  auto Mod = parse("(module (func $f) (table 4 funcref) "
                   "(elem (i32.add (i32.const 1) (i32.const 2)) $f))");
  ASSERT_EQ(Mod.getElementSection().getContent().size(), 1U);
  const auto &Instrs =
      Mod.getElementSection().getContent()[0].getExpr().getInstrs();
  ASSERT_EQ(Instrs.size(), 4U);
  EXPECT_EQ(Instrs[0].getOpCode(), OpCode::I32__const);
  EXPECT_EQ(Instrs[1].getOpCode(), OpCode::I32__const);
  EXPECT_EQ(Instrs[2].getOpCode(), OpCode::I32__add);
  EXPECT_EQ(Instrs[3].getOpCode(), OpCode::End);
  Mod = parse("(module (memory 1) "
              "(data (i32.add (i32.const 1) (i32.const 2)) \"x\"))");
  ASSERT_EQ(Mod.getDataSection().getContent().size(), 1U);
  EXPECT_EQ(Mod.getDataSection().getContent()[0].getExpr().getInstrs().size(),
            4U);
}

// An elemlist needs its prefix. Only the active abbreviation, which has no
// explicit (table ...) sexpr, can omit it, and it then holds funcidx*.
TEST(WatConverterTest, ElemListNeedsPrefix) {
  EXPECT_EQ(err("(module (elem))"), ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (elem $e))"), ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (elem $e declare))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (table 1 funcref) "
                "(elem (table 0) (offset (i32.const 0))))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_TRUE(ok("(module (elem func))"));
  EXPECT_TRUE(ok("(module (elem funcref))"));
  EXPECT_TRUE(ok("(module (func $f) (elem declare func $f))"));
  EXPECT_TRUE(ok("(module (table 1 funcref) "
                 "(elem (table 0) (offset (i32.const 0)) func))"));
  // The active abbreviation takes no prefix, with an offset expression, an
  // (offset ...) sexpr, or a legacy table index.
  EXPECT_TRUE(ok("(module (table 1 funcref) (elem (i32.const 0)))"));
  EXPECT_TRUE(ok("(module (table 1 funcref) (elem (offset (i32.const 0))))"));
  EXPECT_TRUE(ok("(module (table 1 funcref) (elem 0 (i32.const 0)))"));
  EXPECT_TRUE(ok("(module (table $t 1 funcref) (func $f) "
                 "(elem (i32.const 0) $f))"));
}

// An elemlist holds elemexprs after a reftype, or funcidx* after func or in
// the legacy form. The two kinds do not mix.
TEST(WatConverterTest, ElemListKinds) {
  EXPECT_EQ(err("(module (func $f) (elem func $f (item ref.func $f)))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (table 1 externref) "
                "(elem (i32.const 0) (ref.null extern)))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (func $f) (elem funcref $f))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_TRUE(ok("(module (func $f) (elem funcref (item ref.func $f) "
                 "(ref.func $f)))"));
  EXPECT_TRUE(ok("(module (func $f) (elem func $f $f))"));
  EXPECT_TRUE(ok("(module (func $f) (elem declare func $f))"));
}

// No import can come after a tag definition. The import section comes
// before the tag section in the binary, so the indices of the tags disagree
// otherwise.
TEST(WatConverterTest, ImportAfterTag) {
  EXPECT_EQ(err("(module (tag $a (param i32)) "
                "(import \"m\" \"n\" (tag $b (param i64))))"),
            ErrCode::Value::WatImportAfterTag);
  EXPECT_EQ(err("(module (tag $a) (tag (import \"m\" \"n\")))"),
            ErrCode::Value::WatImportAfterTag);
  EXPECT_EQ(err("(module (tag $a) (import \"m\" \"n\" (func)))"),
            ErrCode::Value::WatImportAfterTag);
  EXPECT_TRUE(ok("(module (import \"m\" \"n\" (tag $b (param i64))) "
                 "(tag $a (param i32)))"));
}

//===----------------------------------------------------------------------===//
// String escapes.
//===----------------------------------------------------------------------===//

// The escape decoder has one case for each escape that WAT permits. The spec
// suite does not use the \r escape and the \' escape, so the two escapes need
// a test here.
TEST(WatConverterTest, StringEscapeForms) {
  EXPECT_TRUE(ok("(module (memory 1) (data (i32.const 0) \"\\r\"))"));
  EXPECT_TRUE(ok("(module (memory 1) (data (i32.const 0) \"\\'\"))"));
}

// The \u{...} escape encodes one scalar value as UTF-8. A scalar less than
// U+0800 gives two bytes, and a scalar more than U+FFFF gives four bytes. The
// decoder rejects a scalar that is more than U+10FFFF.
TEST(WatConverterTest, UnicodeEscapeEncoding) {
  EXPECT_TRUE(ok("(module (memory 1) (data (i32.const 0) \"\\u{a9}\"))"));
  EXPECT_TRUE(ok("(module (memory 1) (data (i32.const 0) \"\\u{20ac}\"))"));
  EXPECT_TRUE(ok("(module (memory 1) (data (i32.const 0) \"\\u{1f600}\"))"));
  EXPECT_EQ(err("(module (memory 1) (data (i32.const 0) \"\\u{110000}\"))"),
            ErrCode::Value::WatMalformedString);
  // The hexnum inside the braces takes a '_' between two digits.
  auto Mod = parse("(module (import \"m\" \"\\u{1_F600}\" (func)))");
  ASSERT_EQ(Mod.getImportSection().getContent().size(), 1U);
  EXPECT_EQ(Mod.getImportSection().getContent()[0].getExternalName(),
            "\xF0\x9F\x98\x80");
  EXPECT_FALSE(ok("(module (import \"m\" \"\\u{_1F600}\" (func)))"));
  EXPECT_FALSE(ok("(module (import \"m\" \"\\u{1F600_}\" (func)))"));
  EXPECT_FALSE(ok("(module (import \"m\" \"\\u{1__F600}\" (func)))"));
}

//===----------------------------------------------------------------------===//
// Annotation ids.
//===----------------------------------------------------------------------===//

// A quoted annotation id holds UTF-8. The scanner accepts a sequence of two,
// three, or four bytes. The scanner rejects a lead byte that is not valid,
// and a continuation byte that is not valid. The scanner also rejects a \HH
// escape that gives a byte value of more than 0x7F.
TEST(WatConverterTest, AnnotationIdUtf8) {
  EXPECT_TRUE(ok("(module (@\"\xC2\xA9\") (func))"));
  EXPECT_TRUE(ok("(module (@\"\xE2\x82\xAC\") (func))"));
  EXPECT_TRUE(ok("(module (@\"\xF0\x9F\x98\x80\") (func))"));
  EXPECT_TRUE(ok("(module (@\"\\41\") (func))"));
  EXPECT_EQ(err("(module (@\"\xFF\") (func))"), ErrCode::Value::MalformedUTF8);
  EXPECT_EQ(err("(module (@\"\xC2\x41\") (func))"),
            ErrCode::Value::MalformedUTF8);
  EXPECT_EQ(err("(module (@\"\\80\") (func))"), ErrCode::Value::MalformedUTF8);
}

//===----------------------------------------------------------------------===//
// The root of the text.
//===----------------------------------------------------------------------===//

// The root holds one (module ...) or a bare sequence of module fields. A
// second module, the binary and quote forms of a WAST script, and a stray
// token are malformed.
TEST(WatConverterTest, RootHoldsOneModule) {
  EXPECT_EQ(err("(module (func $a)) (module (func $b))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module binary \"\\00asm\\01\\00\\00\\00\")"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module quote \"(func)\")"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (func (export \"f\"))) foo"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module 123 (func (export \"f\")))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(func) (module)"), ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(func) 0"), ErrCode::Value::WatUnexpectedToken);
  EXPECT_TRUE(ok("(module $m (func))"));
  EXPECT_TRUE(ok("(func) (memory 1)"));
  EXPECT_TRUE(ok(""));
}

// The (@" scan for an empty or malformed annotation id works on the
// annotation nodes. A (@" inside a comment, a string, or a name is data.
TEST(WatConverterTest, AnnotationIdScanIgnoresStringsAndComments) {
  EXPECT_TRUE(ok("(module (func (export \"(@\")))"));
  EXPECT_TRUE(ok("(module (memory 1) (data (i32.const 0) \"(@\"))"));
  EXPECT_TRUE(ok(";; (@\"\n(module)"));
  EXPECT_TRUE(ok("(module (;(@\";) (func))"));
  EXPECT_EQ(err("(module (@\"\") (func))"),
            ErrCode::Value::WatEmptyAnnotationId);
  // The scan for a bare empty annotation id uses the same lexical state. As
  // a result, a "(@" inside a comment or a string is data, also when the
  // source has no module content.
  EXPECT_TRUE(ok(";; (@"));
  EXPECT_TRUE(ok("(; (@ ;)"));
  EXPECT_TRUE(ok("(module (func (export \"(@ \")))"));
  EXPECT_EQ(err("(@)"), ErrCode::Value::WatEmptyAnnotationId);
  EXPECT_EQ(err("(@ x)"), ErrCode::Value::WatEmptyAnnotationId);
  EXPECT_EQ(err("(@(@a)x)"), ErrCode::Value::WatEmptyAnnotationId);
}

//===----------------------------------------------------------------------===//
// Identifier and index references.
//===----------------------------------------------------------------------===//

// A struct field reference takes a name or a decimal index. The converter
// rejects a name that the type does not declare, and it rejects a name on a
// type that declares no field name. The converter accepts an underscore
// separator, but it rejects a hex prefix.
TEST(WatConverterTest, StructFieldReferenceForms) {
  EXPECT_EQ(err("(module (type $s (struct (field $x i32))) "
                "(func (param (ref $s)) (struct.get $s $y (local.get 0)) "
                "drop))"),
            ErrCode::Value::WatUnknownId);
  EXPECT_EQ(err("(module (type $s (struct (field i32))) "
                "(func (param (ref $s)) (struct.get $s $x (local.get 0)) "
                "drop))"),
            ErrCode::Value::WatUnknownId);
  EXPECT_EQ(err("(module (type $s (struct (field i32))) "
                "(func (param (ref $s)) (struct.get $s 0x0 (local.get 0)) "
                "drop))"),
            ErrCode::Value::WatUnknownId);
  EXPECT_TRUE(ok("(module (type $s (struct (field i32) (field i32))) "
                 "(func (param (ref $s)) (struct.get $s 0_1 (local.get 0)) "
                 "drop))"));
}

// A bare type index is not a reftype. The grammar needs the (ref $t) form.
// The other toolchains reject the bare form, so the converter rejects it too.
TEST(WatConverterTest, BareTypeIndexRefType) {
  EXPECT_EQ(err("(module (type $t (func)) (table 1 $t))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_TRUE(ok("(module (type $t (func)) (table 1 (ref $t)))"));
  EXPECT_TRUE(ok("(module (type $t (func)) (table 1 (ref null $t)))"));
}

//===----------------------------------------------------------------------===//
// SIMD lanes.
//===----------------------------------------------------------------------===//

// The i64x2 shape takes a lane that fills all 64 bits. The parser reads such a
// lane as unsigned, and it does not reject the lane as a signed overflow.
TEST(WatConverterTest, V128I64x2UnsignedLane) {
  EXPECT_TRUE(ok("(module (func (result v128) "
                 "(v128.const i64x2 0xffffffffffffffff 0)))"));
}

//===----------------------------------------------------------------------===//
// Exception handling.
//===----------------------------------------------------------------------===//

// A try_table takes four kinds of catch clause. Each kind sets its own pair of
// flags in the control metadata. A flat try_table also takes a label.
TEST(WatConverterTest, TryTableCatchClauses) {
  EXPECT_TRUE(
      ok("(module (tag $e) (func block try_table (catch $e 0) end end))"));
  EXPECT_TRUE(
      ok("(module (tag $e) (func block try_table (catch_ref $e 0) end end))"));
  EXPECT_TRUE(ok("(module (func block try_table (catch_all_ref 0) end end))"));
  EXPECT_TRUE(ok("(module (func try_table $l end))"));
}

// A tag type must have no result. The validator reports the result, as it
// does for a binary module, so the converter accepts the text.
TEST(WatConverterTest, ImportedTagResultType) {
  EXPECT_TRUE(ok("(module (type $t (func (result i32))) "
                 "(tag (import \"m\" \"e\") (type $t)))"));
  EXPECT_TRUE(ok("(module (tag (result i32)))"));
}

//===----------------------------------------------------------------------===//
// The section abbreviations.
//===----------------------------------------------------------------------===//

// The inline data abbreviation of a memory is one (data ...) group. A bare
// string and a second (data ...) group are malformed. The memory gets the
// page count that holds the bytes.
TEST(WatConverterTest, MemoryInlineDataForms) {
  EXPECT_EQ(err("(module (memory \"abc\"))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (memory (data \"a\") (data \"b\")))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (memory (data \"a\") \"b\"))"),
            ErrCode::Value::WatUnexpectedToken);
  auto Mod = parse("(module (memory (data \"a\" \"b\")))");
  ASSERT_EQ(Mod.getMemorySection().getContent().size(), 1U);
  EXPECT_EQ(Mod.getMemorySection().getContent()[0].getLimit().getMin(), 1U);
  ASSERT_EQ(Mod.getDataSection().getContent().size(), 1U);
  EXPECT_EQ(Mod.getDataSection().getContent()[0].getData().size(), 2U);
}

//===----------------------------------------------------------------------===//
// The inline import and export abbreviations.
//===----------------------------------------------------------------------===//

// An inline import takes two string names, and an inline export takes one
// string name. The spec suite uses both abbreviations, but it does not give
// them a token of the wrong kind or an extra token. If the converter accepts
// such a field, the import descriptor gets an empty name.
TEST(WatConverterTest, InlineImportExportGrammar) {
  EXPECT_EQ(err("(module (func (import 0 \"b\")))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (func (import \"a\" 0)))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (func (import \"a\" \"b\" \"c\")))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (func (export 0)))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (func (export \"a\" \"b\")))"),
            ErrCode::Value::WatUnexpectedToken);
}

//===----------------------------------------------------------------------===//
// The malformed section fields.
//===----------------------------------------------------------------------===//

// Each section field has a fixed shape. The converter rejects a field that has
// too few tokens or too many tokens. If the converter accepts such a field,
// the indices of the fields that come after it are wrong.
TEST(WatConverterTest, SectionFieldGrammar) {
  EXPECT_EQ(err("(module (table))"), ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (table 1))"), ErrCode::Value::WatUnexpectedToken);
  // A token after the table type is the start of an init expression.
  EXPECT_EQ(err("(module (table 1 funcref funcref))"),
            ErrCode::Value::WatUnknownOperator);
  EXPECT_EQ(err("(module (table 1 funcref 1))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (global))"), ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (global (mut) (i32.const 0)))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (memory 1 2 3))"), ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (func) (start))"), ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (func) (start 0 0))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (memory 1) (data (memory) (i32.const 0) \"a\"))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (table 1 funcref) "
                "(elem (table) (i32.const 0) func))"),
            ErrCode::Value::WatUnexpectedToken);
}

// An export description names one kind of entity and one index. An unknown
// keyword is an unknown operator, and a missing index is an unexpected token.
TEST(WatConverterTest, ExportDescriptionGrammar) {
  EXPECT_EQ(err("(module (export \"a\"))"), ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (func) (export \"a\" (func)))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (func) (export \"a\" (bogus 0)))"),
            ErrCode::Value::WatUnknownOperator);
}

//===----------------------------------------------------------------------===//
// The malformed instruction operands.
//===----------------------------------------------------------------------===//

// A plain instruction takes a fixed number of immediate operands. The
// converter rejects a missing operand and an extra operand.
TEST(WatConverterTest, InstructionOperandCount) {
  EXPECT_EQ(err("(module (func (result i32) (i32.const 1 2)))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (func (param i32) (result i32) (local.get)))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (func) (func (call)))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (func (block (br))))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (func) (func (result funcref) (ref.func)))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (memory 1) (func (memory.copy 0 0 0)))"),
            ErrCode::Value::WatUnexpectedToken);
}

// A v128.const takes a shape keyword that WAT knows, and each lane must be a
// numeric literal. The spec suite covers the lane count, but it does not cover
// these two forms.
TEST(WatConverterTest, V128ConstShapeGrammar) {
  EXPECT_EQ(err("(module (func (result v128) (v128.const i9x17 1)))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (func (result v128) (v128.const f32x4 a b c d)))"),
            ErrCode::Value::WatUnknownOperator);
}

//===----------------------------------------------------------------------===//
// The malformed instruction shapes that must not crash.
//===----------------------------------------------------------------------===//

// An i8x16.shuffle takes exactly 16 lane indices, and each one is an unsigned
// integer. The error texts are the texts of the spec suite.
TEST(WatConverterTest, ShuffleLaneIndices) {
  const auto Shuffle = [](std::string_view Lanes) {
    return err("(module (func (param v128) (result v128) (i8x16.shuffle " +
               std::string(Lanes) + " (local.get 0) (local.get 0))))");
  };
  EXPECT_EQ(Shuffle("0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15"),
            ErrCode::Value::Success);
  EXPECT_EQ(Shuffle("0 1 2 3 4 5 6 7 8 9 10 11 12 13 14"),
            ErrCode::Value::WatWrongLaneIndexCount);
  EXPECT_EQ(Shuffle("0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16"),
            ErrCode::Value::WatWrongLaneIndexCount);
  EXPECT_EQ(Shuffle(""), ErrCode::Value::WatWrongLaneIndexCount);
  EXPECT_EQ(err("(module (func (result v128) (i8x16.shuffle)))"),
            ErrCode::Value::WatWrongLaneIndexCount);
  for (const auto Bad :
       {"-1"sv, "15.0"sv, "0.5"sv, "inf"sv, "-inf"sv, "nan"sv}) {
    EXPECT_EQ(Shuffle(std::string("0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 ") +
                      std::string(Bad)),
              ErrCode::Value::WatUnexpectedToken)
        << Bad;
  }
  EXPECT_EQ(Shuffle("0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 256"),
            ErrCode::Value::WatI8ConstOutOfRange);
  EXPECT_EQ(Shuffle("0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 32"),
            ErrCode::Value::InvalidLaneIdx);
}

// An instruction that needs an immediate can come without one. The node
// accessors of the tree-sitter wrapper are null-safe, so a missing immediate
// gives an error and not a null dereference.
TEST(WatConverterTest, MissingImmediateIsRejected) {
  EXPECT_EQ(err("(module (func (ref.null) drop))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (import \"m\" \"n\" (global)))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (func (drop (ref.test))))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (func (drop (ref.cast))))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (func (block (br_on_cast 0 anyref))))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (func (block (br_on_cast 0))))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (global))"), ErrCode::Value::WatUnexpectedToken);
}

// A br_table takes one label or more. An empty label list is malformed. If the
// converter accepts it, the validator reads an empty span.
TEST(WatConverterTest, BrTableNeedsLabel) {
  EXPECT_EQ(err("(module (func i32.const 0 br_table))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (func (br_table (i32.const 0))))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_TRUE(ok("(module (func i32.const 0 br_table 0))"));
  EXPECT_TRUE(ok("(module (func (br_table 0 (i32.const 0))))"));
}

// An if block takes at most one else. The converter rejects a second else in
// the flat form and in the folded form.
TEST(WatConverterTest, SecondElseIsRejected) {
  EXPECT_EQ(err("(module (func (result i32) i32.const 1 if (result i32) "
                "i32.const 1 else i32.const 2 else i32.const 3 end))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (func (result i32) (if (result i32) (i32.const 1) "
                "(then (i32.const 1)) (else (i32.const 2)) "
                "(else (i32.const 3)))))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_TRUE(ok("(module (func (result i32) i32.const 1 if (result i32) "
                 "i32.const 1 else i32.const 2 end))"));
}

// A flat end needs an open block. An end with no open block is malformed, and
// the converter does not turn it into the end of the function body.
TEST(WatConverterTest, StrayEndIsRejected) {
  EXPECT_EQ(err("(module (func (result i32) i32.const 1 end unreachable))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (func end unreachable))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (func (block end) unreachable))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (func end))"), ErrCode::Value::WatUnexpectedToken);
}

// A (sub ...) type needs a comptype. Without one, the converter must not push
// a type with an indeterminate composite type.
TEST(WatConverterTest, SubTypeNeedsCompType) {
  EXPECT_EQ(err("(module (type $t (sub final)))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (type $t (sub)))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (type $t (sub final (bogus))))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_TRUE(ok("(module (type $t (sub final (func))))"));
}

//===----------------------------------------------------------------------===//
// The proposal gates.
//===----------------------------------------------------------------------===//

// Each instruction that a proposal adds needs that proposal. The text path
// gives the same error code as the binary loader.
TEST(WatConverterTest, InstructionProposalGate) {
  using WasmEdge::Proposal;
  EXPECT_EQ(errConf(without({Proposal::NonTrapFloatToIntConversions}),
                    "(module (func (drop (i32.trunc_sat_f32_s "
                    "(f32.const 0)))))"),
            ErrCode::Value::IllegalOpCode);
  EXPECT_EQ(errConf(without({Proposal::SignExtensionOperators}),
                    "(module (func (drop (i32.extend8_s (i32.const 0)))))"),
            ErrCode::Value::IllegalOpCode);
  EXPECT_EQ(errConf(without({Proposal::SIMD}),
                    "(module (func (drop (v128.const i32x4 0 0 0 0))))"),
            ErrCode::Value::IllegalOpCode);
  EXPECT_EQ(errConf(without({Proposal::TailCall}),
                    "(module (func) (func (return_call 0)))"),
            ErrCode::Value::IllegalOpCode);
  EXPECT_EQ(errConf(without({Proposal::GC}),
                    "(module (func (drop (ref.i31 (i32.const 0)))))"),
            ErrCode::Value::IllegalOpCode);
  EXPECT_EQ(errConf(without({Proposal::ExceptionHandling}),
                    "(module (func try_table end))"),
            ErrCode::Value::IllegalOpCode);
  EXPECT_EQ(errConf(without({Proposal::ExceptionHandling}),
                    "(module (func (try_table)))"),
            ErrCode::Value::IllegalOpCode);
  // Threads is off in the default Configure.
  EXPECT_EQ(errConf(WasmEdge::Configure(),
                    "(module (memory 1) (func (drop (i32.atomic.load "
                    "(i32.const 0)))))"),
            ErrCode::Value::IllegalOpCode);
  EXPECT_EQ(errConf(WasmEdge::Configure(),
                    "(module (memory 1) (func "
                    "i32.const 0 i32.atomic.load drop))"),
            ErrCode::Value::IllegalOpCode);
  // These four atomic opcodes come before i32.atomic.load in the opcode
  // order, and they need Threads too.
  EXPECT_EQ(errConf(WasmEdge::Configure(), "(module (func atomic.fence))"),
            ErrCode::Value::IllegalOpCode);
  EXPECT_EQ(errConf(WasmEdge::Configure(),
                    "(module (memory 1) (func (drop (memory.atomic.notify "
                    "(i32.const 0) (i32.const 0)))))"),
            ErrCode::Value::IllegalOpCode);
  EXPECT_EQ(errConf(WasmEdge::Configure(),
                    "(module (memory 1) (func (drop (memory.atomic.wait32 "
                    "(i32.const 0) (i32.const 0) (i64.const 0)))))"),
            ErrCode::Value::IllegalOpCode);
  EXPECT_EQ(errConf(WasmEdge::Configure(),
                    "(module (memory 1) (func (drop (memory.atomic.wait64 "
                    "(i32.const 0) (i64.const 0) (i64.const 0)))))"),
            ErrCode::Value::IllegalOpCode);
  {
    WasmEdge::Configure Threads;
    Threads.addProposal(WasmEdge::Proposal::Threads);
    EXPECT_EQ(errConf(Threads, "(module (func atomic.fence))"),
              ErrCode::Value::Success);
  }
  // A typed select is a different opcode from a plain select.
  EXPECT_EQ(errConf(without({Proposal::GC, Proposal::FunctionReferences,
                             Proposal::ReferenceTypes}),
                    "(module (func (result i32) (select (result i32) "
                    "(i32.const 1) (i32.const 2) (i32.const 0))))"),
            ErrCode::Value::IllegalOpCode);
  EXPECT_TRUE(ok("(module (func (result i32) (select (result i32) "
                 "(i32.const 1) (i32.const 2) (i32.const 0))))"));
  // A table index of call_indirect and a memory index of a memory
  // instruction need a proposal when the index is not zero.
  EXPECT_EQ(errConf(without({Proposal::GC, Proposal::FunctionReferences,
                             Proposal::ReferenceTypes}),
                    "(module (table 1 funcref) (table 1 funcref) "
                    "(func (call_indirect 1 (i32.const 0))))"),
            ErrCode::Value::ExpectedZeroByte);
  EXPECT_EQ(errConf(without({Proposal::MultiMemories}),
                    "(module (memory 1) (memory 1) "
                    "(func (drop (i32.load 1 (i32.const 0)))))"),
            ErrCode::Value::MalformedMemoryOpFlags);
  EXPECT_EQ(errConf(without({Proposal::MultiMemories}),
                    "(module (memory 1) (memory 1) "
                    "(func (memory.fill 1 (i32.const 0) (i32.const 0) "
                    "(i32.const 0))))"),
            ErrCode::Value::ExpectedZeroByte);
  EXPECT_TRUE(ok("(module (memory 1) (memory 1) "
                 "(func (drop (i32.load 1 (i32.const 0)))))"));
}

// A 64-bit limit needs Memory64, and a shared limit needs Threads.
TEST(WatConverterTest, LimitsProposalGate) {
  using WasmEdge::Proposal;
  EXPECT_EQ(errConf(without({Proposal::Memory64}),
                    "(module (table $t i64 1 funcref))"),
            ErrCode::Value::IntegerTooLarge);
  EXPECT_EQ(errConf(without({Proposal::Memory64}), "(module (memory i64 1))"),
            ErrCode::Value::IntegerTooLarge);
  EXPECT_EQ(errConf(WasmEdge::Configure(), "(module (memory 1 1 shared))"),
            ErrCode::Value::MalformedLimitFlags);
  EXPECT_EQ(
      errConf(without({Proposal::Memory64}), "(module (memory 1 1 shared))"),
      ErrCode::Value::IntegerTooLarge);
  WasmEdge::Configure Threads;
  Threads.addProposal(Proposal::Threads);
  EXPECT_EQ(errConf(Threads, "(module (memory 1 1 shared))"),
            ErrCode::Value::Success);
  EXPECT_TRUE(ok("(module (table $t i64 1 funcref))"));
  // The inline elem and the inline data abbreviations take the address type
  // too, and they need the same proposal.
  EXPECT_EQ(errConf(without({Proposal::Memory64}),
                    "(module (table i64 funcref (elem)))"),
            ErrCode::Value::IntegerTooLarge);
  EXPECT_EQ(errConf(without({Proposal::Memory64}),
                    "(module (memory i64 (data \"abc\")))"),
            ErrCode::Value::IntegerTooLarge);
  EXPECT_TRUE(ok("(module (table i64 funcref (elem)))"));
  EXPECT_TRUE(ok("(module (memory i64 (data \"abc\")))"));
}

// A reference type keyword and a heap type keyword need the proposal that
// adds the type. The funcref keyword is a part of the core spec.
TEST(WatConverterTest, RefTypeProposalGate) {
  using WasmEdge::Proposal;
  const auto NoRef = without(
      {Proposal::GC, Proposal::FunctionReferences, Proposal::ReferenceTypes});
  EXPECT_EQ(errConf(NoRef, "(module (table $t 1 externref))"),
            ErrCode::Value::MalformedElemType);
  EXPECT_EQ(errConf(NoRef, "(module (global $g externref (ref.null extern)))"),
            ErrCode::Value::MalformedElemType);
  EXPECT_EQ(errConf(NoRef, "(module (table $t 1 funcref))"),
            ErrCode::Value::Success);
  EXPECT_EQ(errConf(without({Proposal::GC}),
                    "(module (global anyref (ref.null any)))"),
            ErrCode::Value::MalformedRefType);
  EXPECT_EQ(errConf(without({Proposal::GC}), "(module (func (param i31ref)))"),
            ErrCode::Value::MalformedRefType);
  EXPECT_EQ(errConf(without({Proposal::GC}),
                    "(module (func (param (ref null struct))))"),
            ErrCode::Value::MalformedRefType);
  EXPECT_EQ(errConf(without({Proposal::ExceptionHandling}),
                    "(module (global exnref (ref.null exn)))"),
            ErrCode::Value::MalformedValType);
  EXPECT_EQ(errConf(without({Proposal::SIMD}), "(module (func (param v128)))"),
            ErrCode::Value::MalformedValType);
}

// A (ref ...) type, a type index as a heap type, and a table init expression
// need Typed Function References.
TEST(WatConverterTest, FuncRefProposalGate) {
  using WasmEdge::Proposal;
  const auto NoFuncRef = without({Proposal::GC, Proposal::FunctionReferences});
  EXPECT_EQ(errConf(NoFuncRef, "(module (type $t (func)) "
                               "(global $g (ref null $t) (ref.null $t)))"),
            ErrCode::Value::MalformedRefType);
  EXPECT_EQ(errConf(NoFuncRef, "(module (type $t (func)) "
                               "(func (param (ref null $t))))"),
            ErrCode::Value::MalformedRefType);
  EXPECT_EQ(errConf(NoFuncRef, "(module (func (param (ref func))))"),
            ErrCode::Value::MalformedRefType);
  EXPECT_EQ(errConf(NoFuncRef, "(module (func (result funcref) "
                               "(ref.null 0)) (type (func)))"),
            ErrCode::Value::MalformedRefType);
  EXPECT_EQ(errConf(NoFuncRef, "(module (func $f) "
                               "(table 1 funcref (ref.func $f)))"),
            ErrCode::Value::MalformedTable);
  EXPECT_TRUE(ok("(module (func $f) (table 1 funcref (ref.func $f)))"));
  EXPECT_TRUE(ok("(module (func (param (ref func))))"));
}

// A struct type, an array type, a sub type, and a rec group need GC.
TEST(WatConverterTest, GcTypeProposalGate) {
  using WasmEdge::Proposal;
  const auto NoGc = without({Proposal::GC});
  EXPECT_EQ(errConf(NoGc, "(module (type $t (struct (field i32))))"),
            ErrCode::Value::IntegerTooLong);
  EXPECT_EQ(errConf(NoGc, "(module (type $t (array i32)))"),
            ErrCode::Value::IntegerTooLong);
  EXPECT_EQ(errConf(NoGc, "(module (rec (type $t (func))))"),
            ErrCode::Value::IntegerTooLong);
  EXPECT_EQ(errConf(NoGc, "(module (type $t (sub final (func))))"),
            ErrCode::Value::IntegerTooLong);
  EXPECT_EQ(errConf(NoGc, "(module (type $t (func)))"),
            ErrCode::Value::Success);
}

// A block type with a parameter, or with an explicit type index, needs
// Multi-value.
TEST(WatConverterTest, BlockTypeProposalGate) {
  using WasmEdge::Proposal;
  const auto NoMulti = without({Proposal::MultiValue});
  EXPECT_EQ(errConf(NoMulti, "(module (func (result i32) (i32.const 5) "
                             "(block (param i32) (result i32))))"),
            ErrCode::Value::MalformedValType);
  EXPECT_EQ(errConf(NoMulti,
                    "(module (type $t (func (param i32) (result i32))) "
                    "(func (result i32) (i32.const 5) "
                    "(block (type $t))))"),
            ErrCode::Value::MalformedValType);
  EXPECT_EQ(errConf(NoMulti, "(module (func (result i32) (i32.const 5) "
                             "block (param i32) (result i32) end))"),
            ErrCode::Value::MalformedValType);
  EXPECT_EQ(errConf(NoMulti, "(module (func (result i32) "
                             "(block (result i32) (i32.const 5))))"),
            ErrCode::Value::Success);
}

// A tag needs Exception Handling in a definition, an import, and an export.
// A mutable global import needs Import/Export of Mutable Globals.
TEST(WatConverterTest, FieldProposalGate) {
  using WasmEdge::Proposal;
  const auto NoEH = without({Proposal::ExceptionHandling});
  EXPECT_EQ(errConf(NoEH, "(module (tag))"), ErrCode::Value::MalformedSection);
  EXPECT_EQ(errConf(NoEH, "(module (import \"m\" \"t\" (tag)))"),
            ErrCode::Value::MalformedImportKind);
  EXPECT_EQ(errConf(NoEH, "(module (tag (import \"m\" \"t\")))"),
            ErrCode::Value::MalformedImportKind);
  EXPECT_EQ(errConf(without({Proposal::ImportExportMutGlobals}),
                    "(module (import \"m\" \"g\" (global (mut i32))))"),
            ErrCode::Value::InvalidMut);
  EXPECT_EQ(errConf(without({Proposal::ImportExportMutGlobals}),
                    "(module (global (import \"m\" \"g\") (mut i32)))"),
            ErrCode::Value::InvalidMut);
  EXPECT_EQ(errConf(without({Proposal::ImportExportMutGlobals}),
                    "(module (import \"m\" \"g\" (global i32)))"),
            ErrCode::Value::Success);
}

// These segments need Bulk Memory or Reference Types:
// - A passive segment.
// - A declarative segment.
// - A segment with a table index or a memory index that is not zero.
TEST(WatConverterTest, SegmentProposalGate) {
  using WasmEdge::Proposal;
  const auto NoBulk =
      without({Proposal::GC, Proposal::FunctionReferences,
               Proposal::ReferenceTypes, Proposal::BulkMemoryOperations});
  EXPECT_EQ(errConf(NoBulk, "(module (data \"x\"))"),
            ErrCode::Value::ExpectedZeroByte);
  EXPECT_EQ(errConf(NoBulk, "(module (func $f) (elem func $f))"),
            ErrCode::Value::ExpectedZeroByte);
  EXPECT_EQ(errConf(NoBulk, "(module (func $f) (elem declare func $f))"),
            ErrCode::Value::ExpectedZeroByte);
  EXPECT_EQ(errConf(NoBulk, "(module (memory 1) (memory 1) "
                            "(data (memory 1) (i32.const 0) \"x\"))"),
            ErrCode::Value::ExpectedZeroByte);
  EXPECT_EQ(errConf(NoBulk, "(module (memory 1) (func $f) (table 1 funcref) "
                            "(elem (i32.const 0) $f) "
                            "(data (i32.const 0) \"x\"))"),
            ErrCode::Value::Success);
}

// The implicit-type walk runs before pass 2 and ignores the errors of a
// typeuse. It must not log a proposal error that pass 2 does not report.
TEST(WatConverterTest, ImplicitTypeWalkDoesNotLogProposal) {
  using WasmEdge::Proposal;
  const auto Conf = without(
      {Proposal::RelaxSIMD, Proposal::SIMD, Proposal::ImportExportMutGlobals});
  std::ostringstream Log;
  auto Previous = spdlog::default_logger();
  spdlog::set_default_logger(std::make_shared<spdlog::logger>(
      "capture", std::make_shared<spdlog::sinks::ostream_sink_st>(Log)));
  const auto Code = errConf(Conf, "(module (import \"a\" \"b\" "
                                  "(global (mut i32))) (func (param v128)))");
  spdlog::set_default_logger(Previous);
  EXPECT_EQ(Code, ErrCode::Value::InvalidMut);
  EXPECT_NE(Log.str().find("Mutable Globals"), std::string::npos) << Log.str();
  EXPECT_EQ(Log.str().find("SIMD"), std::string::npos) << Log.str();
}

// The end of a folded block gets the offset of the closing parenthesis, as
// the flat end gets the offset of its keyword. A validation error on the end
// then points into the source.
TEST(WatConverterTest, FoldedBlockEndHasOffset) {
  using WasmEdge::OpCode;
  for (const auto Src :
       {"(module (func (block nop)))"sv, "(module (func (loop nop)))"sv,
        "(module (func (if (i32.const 0) (then))))"sv}) {
    const auto Mod = parse(Src);
    const auto *End = findInstr(Mod, OpCode::End);
    ASSERT_NE(End, nullptr) << Src;
    // The block sexpr is the third from the end of the source: the two last
    // parentheses close the func and the module.
    EXPECT_EQ(End->getOffset(), Src.size() - 3) << Src;
  }
}

//===----------------------------------------------------------------------===//
// The strict grammar of the immediates and the fields.
//===----------------------------------------------------------------------===//

// A typed select with an empty (result) is a typed select with no type. The
// validator then rejects the arity, as it does for the binary form.
TEST(WatConverterTest, SelectEmptyResultStaysTyped) {
  using WasmEdge::OpCode;
  auto Mod = parse("(module (func (result i32) (select (result) "
                   "(i32.const 1) (i32.const 2) (i32.const 0))))");
  const auto *Sel = findInstr(Mod, OpCode::Select_t);
  ASSERT_NE(Sel, nullptr);
  EXPECT_EQ(Sel->getValTypeList().size(), 0U);
  Mod = parse("(module (func (result i32) (select "
              "(i32.const 1) (i32.const 2) (i32.const 0))))");
  EXPECT_NE(findInstr(Mod, OpCode::Select), nullptr);
  EXPECT_EQ(findInstr(Mod, OpCode::Select_t), nullptr);
}

// A table.copy takes two table indices or none, as memory.copy does.
TEST(WatConverterTest, TableCopyIndexCount) {
  EXPECT_EQ(err("(module (table $a 1 funcref) (table $b 1 funcref) "
                "(func (table.copy $b (i32.const 0) (i32.const 0) "
                "(i32.const 1))))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_TRUE(ok("(module (table $a 1 funcref) (table $b 1 funcref) "
                 "(func (table.copy $b $a (i32.const 0) (i32.const 0) "
                 "(i32.const 1))))"));
  EXPECT_TRUE(ok("(module (table 1 funcref) (func (table.copy (i32.const 0) "
                 "(i32.const 0) (i32.const 1))))"));
}

// The address type comes before the limits, and shared comes after them.
TEST(WatConverterTest, LimitsKeywordOrder) {
  WasmEdge::Configure Conf;
  Conf.addProposal(WasmEdge::Proposal::Threads);
  EXPECT_EQ(errConf(Conf, "(module (memory 1 1 shared i64))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(errConf(Conf, "(module (memory 1 i64 1))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(errConf(Conf, "(module (memory shared 1 1))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(errConf(Conf, "(module (table 1 i64 funcref))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(errConf(Conf, "(module (table 1 1 shared funcref))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(errConf(Conf, "(module (memory i64 1 1 shared))"),
            ErrCode::Value::Success);
  EXPECT_EQ(errConf(Conf, "(module (memory i32 1 shared))"),
            ErrCode::Value::Success);
}

// The numeral after offset= and align= follows the lexical grammar of an
// unsigned integer: an underscore only between two digits, and a lower-case
// hex prefix.
TEST(WatConverterTest, MemArgNumeralGrammar) {
  EXPECT_EQ(err("(module (memory 1) (func (drop (i32.load offset=1__0 "
                "(i32.const 0)))))"),
            ErrCode::Value::WatUnknownOperator);
  EXPECT_EQ(err("(module (memory 1) (func (drop (i32.load offset=1_ "
                "(i32.const 0)))))"),
            ErrCode::Value::WatUnknownOperator);
  EXPECT_EQ(err("(module (memory 1) (func (drop (i32.load offset=0X10 "
                "(i32.const 0)))))"),
            ErrCode::Value::WatUnknownOperator);
  EXPECT_EQ(err("(module (memory 1) (func (drop (i32.load align=0x_4 "
                "(i32.const 0)))))"),
            ErrCode::Value::WatUnknownOperator);
  EXPECT_EQ(err("(module (memory 1) (func (drop (i32.load offset= "
                "(i32.const 0)))))"),
            ErrCode::Value::WatUnknownOperator);
  EXPECT_TRUE(ok("(module (memory 1) (func (drop (i32.load offset=1_0 "
                 "align=0x4 (i32.const 0)))))"));
}

// An empty label id is malformed. An end and an else take an id only, and
// not a numeric label.
TEST(WatConverterTest, LabelGrammar) {
  EXPECT_EQ(err("(module (func (block $ )))"),
            ErrCode::Value::WatEmptyIdentifier);
  EXPECT_EQ(err("(module (func block $\"\" end))"),
            ErrCode::Value::WatEmptyIdentifier);
  EXPECT_EQ(err("(module (func block end 0))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (func i32.const 0 if else 0 end 0))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_TRUE(ok("(module (func block $l end $l))"));
  EXPECT_TRUE(ok("(module (func i32.const 0 if $l else $l end $l))"));
}

// A token that is not an instruction is malformed inside every instruction
// sequence. The same rule applies inside a folded block body, a then clause,
// and an else clause.
TEST(WatConverterTest, StrayTokenInInstructionSequence) {
  EXPECT_EQ(err("(module (func (block nop 0)))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (func (loop 0)))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (func (if (i32.const 0) (then nop 0))))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (func (if (i32.const 0) (then) (else nop 0))))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (func nop \"x\"))"),
            ErrCode::Value::WatUnexpectedToken);
}

// A tag and a (mut ...) reject the tokens that come after their content, as
// every other field does.
TEST(WatConverterTest, TrailingTokensInTagAndMut) {
  EXPECT_EQ(err("(module (tag x))"), ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (tag (param i32) x))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (global (mut i32 i64) (i32.const 0)))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (type (struct (field (mut i32 i64)))))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_TRUE(ok("(module (tag (param i32)))"));
  EXPECT_TRUE(ok("(module (global (mut i32) (i32.const 0)))"));
}

//===----------------------------------------------------------------------===//
// The malformed type fields.
//===----------------------------------------------------------------------===//

// A type field also has a fixed shape. A (ref ...) needs a heaptype, a (mut
// ...) needs a valtype, and an arraytype needs a fieldtype.
TEST(WatConverterTest, TypeFieldGrammar) {
  EXPECT_EQ(err("(module (func (param (ref))))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (func (param (ref null bogus))))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (func (param bogus)))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (type (array)))"), ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (type (struct (field (mut)))))"),
            ErrCode::Value::WatUnexpectedToken);
}

} // namespace
