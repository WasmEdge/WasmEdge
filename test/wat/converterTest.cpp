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
/// Therefore each negative test asserts the specific ErrCode. A wrong code
/// then gives a mismatch, and not a has_value() test that passes.
///
//===----------------------------------------------------------------------===//

#include "wat/parser.h"

#include "common/configure.h"
#include "common/errcode.h"

#include <gtest/gtest.h>
#include <string>
#include <string_view>

namespace {

using WasmEdge::ErrCode;

// Parse with the default Configure. The result is true if the conversion is
// successful.
bool ok(std::string_view S) {
  WasmEdge::Configure Conf;
  return WasmEdge::WAT::parseWat(S, Conf).has_value();
}
// Give the error code of a conversion that failed. If the conversion is
// successful, the result is Success.
ErrCode::Value err(std::string_view S) {
  WasmEdge::Configure Conf;
  auto Res = WasmEdge::WAT::parseWat(S, Conf);
  return Res ? ErrCode::Value::Success : Res.error().getEnum();
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

// A structtype has no abbreviation without the "field" keyword. Only an
// arraytype takes a bare fieldtype. Therefore the converter rejects a struct
// member that is not a (field ...) group.
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

// A (ref ...) type with a trailing operand is rejected.
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

// The depth of the folded instructions has a bound, so that pathological
// untrusted input cannot overflow the native stack. The converter rejects a
// depth that is more than the limit, but it converts a small depth.
TEST(WatConverterTest, DeepNestingRejected) {
  std::string S = "(module (func ";
  const int N = 2000; // This depth is much more than MaxInstrNestDepth (64).
  for (int I = 0; I < N; ++I) {
    S += "(block ";
  }
  for (int I = 0; I < N; ++I) {
    S += ")";
  }
  S += "))";
  EXPECT_EQ(err(S), ErrCode::Value::WatNestingTooDeep);
  EXPECT_TRUE(ok("(module (func (block (block (block nop)))))"));
}

// A depth that is equal to the limit must convert without a native stack
// overflow. A depth of one more level must fail. The limit holds for the 1 MiB
// stack of Windows, where each level costs some KiB.
TEST(WatConverterTest, NestingLimitBoundary) {
  auto Nest = [](int N) {
    std::string S = "(module (func ";
    for (int I = 0; I < N; ++I) {
      S += "(block ";
    }
    for (int I = 0; I < N; ++I) {
      S += ")";
    }
    return S + "))";
  };
  EXPECT_TRUE(ok(Nest(64)));
  EXPECT_EQ(err(Nest(65)), ErrCode::Value::WatNestingTooDeep);
}

//===----------------------------------------------------------------------===//
// The section forms: memory, table, and data.
//===----------------------------------------------------------------------===//

// The converter parses a 64-bit index type together with a shared limit.
TEST(WatConverterTest, MemoryForms) {
  EXPECT_TRUE(ok("(module (memory i64 1 2 shared))"));
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

// A bare type index is a reftype that is not nullable. A table takes such a
// reftype.
TEST(WatConverterTest, BareTypeIndexRefType) {
  EXPECT_TRUE(ok("(module (type $t (func)) (table 1 $t))"));
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

// A tag type must have no result. The spec suite asserts this rule for the
// explicit (import ... (tag ...)) form. The converter has a second path for
// the inline (tag (import ...)) abbreviation, and this test covers that path.
TEST(WatConverterTest, ImportedTagResultType) {
  EXPECT_EQ(err("(module (type $t (func (result i32))) "
                "(tag (import \"m\" \"e\") (type $t)))"),
            ErrCode::Value::WatNonEmptyTagResult);
}

//===----------------------------------------------------------------------===//
// The section abbreviations.
//===----------------------------------------------------------------------===//

// The inline data abbreviation of a memory also takes a bare string, and not
// only the (data ...) group that the spec suite uses. The memory then gets the
// page count that holds the bytes.
TEST(WatConverterTest, MemoryInlineDataForms) {
  EXPECT_TRUE(ok("(module (memory \"abc\"))"));
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
  EXPECT_EQ(err("(module (table 1 funcref funcref))"),
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
