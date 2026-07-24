// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/test/wat/converterTest.cpp - WAT converter unit tests ----===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Unit tests feeding small WAT snippets through WAT::parseWat, covering
/// converter behavior awkward to express as .wast spec assertions: literal
/// ranges, grammar strictness, and robustness against untrusted input. parseWat
/// only converts (no validation), so negative cases assert the specific ErrCode
/// -- a wrong code surfaces as a mismatch, not a passing has_value() check.
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

// Parse with default Configure; true if conversion succeeded.
bool ok(std::string_view S) {
  WasmEdge::Configure Conf;
  return WasmEdge::WAT::parseWat(S, Conf).has_value();
}
// Error code of a failed conversion, or Success if it unexpectedly succeeded.
ErrCode::Value err(std::string_view S) {
  WasmEdge::Configure Conf;
  auto Res = WasmEdge::WAT::parseWat(S, Conf);
  return Res ? ErrCode::Value::Success : Res.error().getEnum();
}

//===----------------------------------------------------------------------===//
// Numeric literals and index/value ranges.
//===----------------------------------------------------------------------===//

// Numeric indices accept hexadecimal and underscore separators.
TEST(WatConverterTest, IndexLiterals) {
  EXPECT_TRUE(ok("(module (func) (func (call 0x0)))"));
  EXPECT_TRUE(ok("(module (func) (func (call 1_0)) (func) (func) (func) (func) "
                 "(func) (func) (func) (func) (func))"));
  EXPECT_TRUE(ok("(module (func) (func (call 0x0_0)))"));
  // A numeric index that overflows uint32 is rejected rather than wrapping.
  EXPECT_EQ(err("(module (func (call 0xFFFFFFFFFF)))"),
            ErrCode::Value::InvalidFuncIdx);
  EXPECT_EQ(err("(module (func (call 99999999999)))"),
            ErrCode::Value::InvalidFuncIdx);
}

// A numeric branch label is range-checked before narrowing to u32: 2^32 is an
// unknown label, not a wrapped-around br 0.
TEST(WatConverterTest, LabelIndexRange) {
  EXPECT_EQ(err("(module (func (block (br 4294967296))))"),
            ErrCode::Value::WatUnknownLabel);
  EXPECT_TRUE(ok("(module (func (block (br 0))))"));
}

// A numeric struct field index is range-checked before narrowing to u32.
TEST(WatConverterTest, StructFieldIndexRange) {
  EXPECT_EQ(err("(module (type $s (struct (field i32))) "
                "(func (param (ref $s)) (struct.get $s 4294967296 "
                "(local.get 0)) drop))"),
            ErrCode::Value::WatUnknownId);
}

// array.new_fixed's element count is range-checked before narrowing to u32.
TEST(WatConverterTest, ArrayNewFixedCountRange) {
  EXPECT_EQ(err("(module (type $a (array i32)) "
                "(func (array.new_fixed $a 4294967296) drop))"),
            ErrCode::Value::WatConstantOutOfRange);
}

// An f32 nan payload wider than the mantissa is rejected, not truncated.
TEST(WatConverterTest, NanPayloadRange) {
  EXPECT_EQ(err("(module (func (result f32) (f32.const nan:0x100000001)))"),
            ErrCode::Value::WatConstantOutOfRange);
  EXPECT_TRUE(ok("(module (func (result f32) (f32.const nan:0x1)))"));
}

//===----------------------------------------------------------------------===//
// String literals.
//===----------------------------------------------------------------------===//

TEST(WatConverterTest, StringLiterals) {
  // An escaped surrogate code point is rejected while decoding the \u{...}
  // escape itself, before raw-byte UTF-8 validation runs.
  EXPECT_EQ(err("(module (func (export \"\\u{d800}\")))"),
            ErrCode::Value::WatMalformedString);
  // Raw invalid UTF-8 bytes in the source are rejected by the tokenizer.
  EXPECT_EQ(err("(module (func (export \"\xED\xA0\x80\")))"),
            ErrCode::Value::MalformedUTF8);
  EXPECT_EQ(err("(module (func (export \"\xC0\")))"),
            ErrCode::Value::MalformedUTF8);
}

// Raw (unescaped) control bytes inside a string literal are rejected; the
// escaped form carries the same byte and is accepted.
TEST(WatConverterTest, RawControlCharInString) {
  EXPECT_EQ(err("(module (memory 1) (data (i32.const 0) \"a\x01\x62\"))"),
            ErrCode::Value::WatMalformedString);
  EXPECT_TRUE(ok("(module (memory 1) (data (i32.const 0) \"a\\01b\"))"));
}

// Whitespace-separated data string literals concatenate; adjacent literals with
// no gap ("a""b") are malformed (WAT requires whitespace between tokens).
TEST(WatConverterTest, DataStringConcatenation) {
  EXPECT_TRUE(ok("(module (memory 1) (data (i32.const 0) \"a\" \"b\"))"));
  EXPECT_TRUE(ok("(module (memory 1) (data (i32.const 0) \"a\" \"b\" \"c\"))"));
  EXPECT_FALSE(ok("(module (memory 1) (data (i32.const 0) \"a\"\"b\"))"));
}

//===----------------------------------------------------------------------===//
// Type definitions and grammar strictness.
//===----------------------------------------------------------------------===//

// structtype has no abbreviation that drops the "field" keyword (only arraytype
// takes a bare fieldtype), so a struct member that is not a (field ...) group
// is rejected rather than silently accepted.
TEST(WatConverterTest, GcStructRequiresField) {
  EXPECT_EQ(err("(module (type (struct (mut i32))))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (type (struct i32)))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (type (struct (ref func))))"),
            ErrCode::Value::WatUnexpectedToken);
  // The canonical (field ...) form -- including a (mut ...) fieldtype inside it
  // -- is accepted.
  EXPECT_TRUE(ok("(module (type (struct (field (mut i32)))))"));
}

// A typeuse must reference a function type; a struct/array type is a clean
// WatUnknownType rather than a null-deref of getFuncType().
TEST(WatConverterTest, TypeUseMustBeFuncType) {
  EXPECT_EQ(err("(module (type $s (struct)) (func (type $s)))"),
            ErrCode::Value::WatUnknownType);
  EXPECT_EQ(err("(module (type $s (struct)) (func (type $s) (param i32)))"),
            ErrCode::Value::WatUnknownType);
  EXPECT_EQ(err("(module (type $a (array i32)) (func (type $a)))"),
            ErrCode::Value::WatUnknownType);
  EXPECT_TRUE(ok("(module (type $f (func (param i32))) (func (type $f)))"));
}

// A (type ...) with no type definition is malformed and is rejected rather than
// silently dropped (which would desync following type indices).
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

// Local declaration grammar: a named local takes exactly one id and one
// valtype; an anonymous local takes a run of valtypes.
TEST(WatConverterTest, LocalDeclGrammar) {
  EXPECT_FALSE(ok("(module (func (local $x)))"));
  EXPECT_FALSE(ok("(module (func (local $x $y i32)))"));
  EXPECT_TRUE(ok("(module (func (local $x i32)))"));
  EXPECT_TRUE(ok("(module (func (local i32 i64)))"));
  EXPECT_TRUE(ok("(module (func (local)))"));
}

// Trailing tokens after an export's index are rejected.
TEST(WatConverterTest, ExportTrailingTokens) {
  EXPECT_EQ(err("(module (func) (export \"a\" (func 0 1)))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_TRUE(ok("(module (func) (export \"a\" (func 0)))"));
}

//===----------------------------------------------------------------------===//
// Control flow and block structure.
//===----------------------------------------------------------------------===//

// Flat (non-folded) try_table converts to a well-formed instruction carrying
// its control metadata, including a catch_all clause targeting an enclosing
// block label.
TEST(WatConverterTest, FlatTryTable) {
  EXPECT_TRUE(ok("(module (func try_table end))"));
  EXPECT_TRUE(ok("(module (func (result i32) "
                 "try_table (result i32) (i32.const 0) end))"));
  EXPECT_TRUE(ok("(module (func block try_table (catch_all 0) end end))"));
}

// A stray 'else' with no enclosing 'if' is rejected.
TEST(WatConverterTest, StrayElse) {
  EXPECT_EQ(err("(module (func else))"), ErrCode::Value::WatUnexpectedToken);
  EXPECT_EQ(err("(module (func block else end))"),
            ErrCode::Value::WatUnexpectedToken);
  EXPECT_TRUE(ok("(module (func if else end))"));
}

// A flat block without a matching 'end' is rejected.
TEST(WatConverterTest, UnclosedBlock) {
  EXPECT_EQ(err("(module (func block))"), ErrCode::Value::WatUnexpectedEnd);
  EXPECT_TRUE(ok("(module (func block end))"));
}

// Folded-instruction nesting is bounded so pathological untrusted input cannot
// overflow the native stack; nesting past the limit is rejected while modest
// nesting still converts.
TEST(WatConverterTest, DeepNestingRejected) {
  std::string S = "(module (func ";
  const int N = 2000; // well past MaxInstrNestDepth (64)
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

// Nesting exactly to the limit must convert without overflowing the native
// stack, and one level beyond it must be rejected. The limit is sized against
// Windows' 1 MiB stack, where each level costs several KiB.
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
// Section forms: memory, table, data.
//===----------------------------------------------------------------------===//

// A 64-bit index type combined with a shared limit parses.
TEST(WatConverterTest, MemoryForms) {
  EXPECT_TRUE(ok("(module (memory i64 1 2 shared))"));
}

// A memory with neither limits nor inline data is rejected.
TEST(WatConverterTest, MemoryRequiresLimitsOrData) {
  EXPECT_EQ(err("(module (memory))"), ErrCode::Value::WatUnexpectedEnd);
  EXPECT_FALSE(ok("(module (memory bogus))"));
  EXPECT_TRUE(ok("(module (memory 1))"));
  EXPECT_TRUE(ok("(module (memory (data \"abc\")))"));
}

// Both explicit address types (i32 and i64) are accepted on a table.
TEST(WatConverterTest, TableAddressType) {
  EXPECT_TRUE(ok("(module (table i32 1 10 funcref))"));
  EXPECT_TRUE(ok("(module (table i64 1 10 funcref))"));
}

// Active data with an explicit (memory 0) target and an i64 offset expression.
TEST(WatConverterTest, DataForms) {
  EXPECT_TRUE(ok("(module (memory i64 1) "
                 "(data (memory 0) (i64.const 0) \"x\"))"));
}

} // namespace
