// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2026 Second State INC

#include "wat/parser.h"

#include "common/span.h"

#include <cstddef>
#include <gtest/gtest.h>

namespace {

// User-defined literal to create a Span<const uint8_t> from a string literal.
// Usage: "\xC3\xA9"_bytes
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

} // namespace
