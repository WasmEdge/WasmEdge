// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "wat/parser.h"

#include "common/span.h"

#include <cstddef>
#include <gtest/gtest.h>

namespace {

// Span<const uint8_t> from a string literal, e.g. "\xC3\xA9"_bytes.
WasmEdge::Span<const uint8_t> operator""_bytes(const char *Str,
                                               std::size_t Len) {
  return WasmEdge::Span<const uint8_t>(reinterpret_cast<const uint8_t *>(Str),
                                       Len);
}

// --- Accepted as WAT (first non-whitespace byte is '(') ---

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

// --- Rejected (clearly not WAT) ---

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

// --- Accepted as WAT: leading UTF-8 BOM or comments before the '(' ---
// Regression: real .wat/.wast files often open with a ';;' header or BOM and
// must route to the WAT parser, not the binary loader.

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
  // A '(;' block comment already begins with '(' and is accepted directly.
  EXPECT_TRUE(WasmEdge::WAT::maybeWAT("(; c ;)(module)"_bytes));
}

// --- Rejected: comment-only content with no module ---

TEST(MaybeWATTest, OnlyLineComment) {
  EXPECT_FALSE(WasmEdge::WAT::maybeWAT(";; just a comment\n"_bytes));
  EXPECT_FALSE(WasmEdge::WAT::maybeWAT(";; no newline"_bytes));
}

TEST(MaybeWATTest, BomOnly) {
  EXPECT_FALSE(WasmEdge::WAT::maybeWAT("\xEF\xBB\xBF"_bytes));
}

} // namespace
