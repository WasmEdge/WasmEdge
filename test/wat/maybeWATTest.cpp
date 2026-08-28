// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "wat/parser.h"

#include "common/span.h"

#include <cstddef>
#include <gtest/gtest.h>

namespace {

// Make a Span<const uint8_t> from a string literal, for example
// "\xC3\xA9"_bytes.
WasmEdge::Span<const uint8_t> operator""_bytes(const char *Str,
                                               std::size_t Len) {
  return WasmEdge::Span<const uint8_t>(reinterpret_cast<const uint8_t *>(Str),
                                       Len);
}

// --- These inputs are WAT. The first byte that is not whitespace is '(' ---

TEST(MaybeWATTest, MinimalModule) {
  EXPECT_TRUE(WasmEdge::WAT::maybeWAT("(module)"_bytes));
}

TEST(MaybeWATTest, ModuleWithWhitespace) {
  EXPECT_TRUE(WasmEdge::WAT::maybeWAT("  (module)"_bytes));
  EXPECT_TRUE(WasmEdge::WAT::maybeWAT("\n(module)"_bytes));
  EXPECT_TRUE(WasmEdge::WAT::maybeWAT("\t(module)"_bytes));
  EXPECT_TRUE(WasmEdge::WAT::maybeWAT("\r\n(module)"_bytes));
  EXPECT_TRUE(WasmEdge::WAT::maybeWAT("    (module)"_bytes));
}

TEST(MaybeWATTest, ModuleWithBody) {
  EXPECT_TRUE(
      WasmEdge::WAT::maybeWAT("(module (func (export \"f\") (nop)))"_bytes));
}

TEST(MaybeWATTest, JustParen) {
  EXPECT_TRUE(WasmEdge::WAT::maybeWAT("("_bytes));
}

// --- These inputs are not WAT ---

TEST(MaybeWATTest, EmptyInput) {
  EXPECT_FALSE(WasmEdge::WAT::maybeWAT(""_bytes));
}

TEST(MaybeWATTest, NoParen) {
  EXPECT_FALSE(WasmEdge::WAT::maybeWAT("module  !"_bytes));
  EXPECT_FALSE(WasmEdge::WAT::maybeWAT(" module"_bytes));
}

TEST(MaybeWATTest, SingleByte) {
  EXPECT_FALSE(WasmEdge::WAT::maybeWAT("\x01"_bytes));
}

TEST(MaybeWATTest, NullByte) {
  EXPECT_FALSE(WasmEdge::WAT::maybeWAT("\x00"_bytes));
}

TEST(MaybeWATTest, WasmMagic) {
  EXPECT_FALSE(WasmEdge::WAT::maybeWAT("\x00asm\x01\x00\x00\x00"_bytes));
}

TEST(MaybeWATTest, OnlyWhitespace) {
  EXPECT_FALSE(WasmEdge::WAT::maybeWAT("   "_bytes));
}

// --- These inputs are WAT. A UTF-8 BOM or a comment comes before the '(' ---
// This is a regression test. A real .wat or .wast file often starts with a
// ';;' header or with a BOM. Such a file must go to the WAT parser, and not to
// the binary loader.

TEST(MaybeWATTest, Utf8Bom) {
  EXPECT_TRUE(WasmEdge::WAT::maybeWAT("\xEF\xBB\xBF(module)"_bytes));
  EXPECT_TRUE(WasmEdge::WAT::maybeWAT("\xEF\xBB\xBF  (module)"_bytes));
}

TEST(MaybeWATTest, LeadingLineComment) {
  EXPECT_TRUE(WasmEdge::WAT::maybeWAT(";; license header\n(module)"_bytes));
  EXPECT_TRUE(WasmEdge::WAT::maybeWAT(";; first\n;; second\n  (module)"_bytes));
}

TEST(MaybeWATTest, BomThenLineComment) {
  EXPECT_TRUE(WasmEdge::WAT::maybeWAT("\xEF\xBB\xBF;; c\n(module)"_bytes));
}

TEST(MaybeWATTest, LeadingBlockComment) {
  // A '(;' block comment starts with '(', so the function accepts it here.
  EXPECT_TRUE(WasmEdge::WAT::maybeWAT("(; c ;)(module)"_bytes));
}

// --- These inputs are not WAT. They hold only comments and no module ---

TEST(MaybeWATTest, OnlyLineComment) {
  EXPECT_FALSE(WasmEdge::WAT::maybeWAT(";; just a comment\n"_bytes));
  EXPECT_FALSE(WasmEdge::WAT::maybeWAT(";; no newline"_bytes));
}

TEST(MaybeWATTest, BomOnly) {
  EXPECT_FALSE(WasmEdge::WAT::maybeWAT("\xEF\xBB\xBF"_bytes));
}

} // namespace
