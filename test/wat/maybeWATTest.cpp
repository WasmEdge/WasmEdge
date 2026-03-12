// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2026 Second State INC

#include "wat/parser.h"

#include "common/span.h"

#include <cstddef>
#include <cstdint>
#include <gtest/gtest.h>

namespace {

// User-defined literal to create a Span<const uint8_t> from a string literal.
// Usage: "\xC3\xA9"_bytes
WasmEdge::Span<const uint8_t> operator""_bytes(const char *Str,
                                               std::size_t Len) {
  return WasmEdge::Span<const uint8_t>(reinterpret_cast<const uint8_t *>(Str),
                                       Len);
}

// --- Accepted as WAT ---

TEST(MaybeWATTest, MinimalModule) {
  EXPECT_TRUE(WasmEdge::WAT::maybeWAT("(module)"_bytes));
}

TEST(MaybeWATTest, ModuleWithWhitespace) {
  EXPECT_TRUE(WasmEdge::WAT::maybeWAT("  (module)"_bytes));
  EXPECT_TRUE(WasmEdge::WAT::maybeWAT("\n(module)"_bytes));
  EXPECT_TRUE(WasmEdge::WAT::maybeWAT("\t(module)"_bytes));
  EXPECT_TRUE(WasmEdge::WAT::maybeWAT("\r\n(module)"_bytes));
}

TEST(MaybeWATTest, ModuleWithBody) {
  EXPECT_TRUE(WasmEdge::WAT::maybeWAT(
      "(module (func (export \"f\") (nop)))"_bytes));
}

TEST(MaybeWATTest, UTF8InStrings) {
  // WAT with UTF-8 in string literals (e.g. export names)
  EXPECT_TRUE(WasmEdge::WAT::maybeWAT(
      "(module (func (export \"\xC3\xA9\")))"_bytes));
}

// --- Rejected (not WAT) ---

TEST(MaybeWATTest, EmptyInput) {
  EXPECT_FALSE(WasmEdge::WAT::maybeWAT(""_bytes));
}

TEST(MaybeWATTest, TooShort) {
  EXPECT_FALSE(WasmEdge::WAT::maybeWAT("(mod)"_bytes));
  EXPECT_FALSE(WasmEdge::WAT::maybeWAT("("_bytes));
}

TEST(MaybeWATTest, NoParen) {
  EXPECT_FALSE(WasmEdge::WAT::maybeWAT("module  !"_bytes));
}

TEST(MaybeWATTest, SingleByte) {
  const uint8_t Data[] = {0x01};
  EXPECT_FALSE(WasmEdge::WAT::maybeWAT(Data));
}

TEST(MaybeWATTest, NullByte) {
  const uint8_t Null[] = {0x00};
  EXPECT_FALSE(WasmEdge::WAT::maybeWAT(Null));
}

TEST(MaybeWATTest, WasmMagic) {
  // WASM binary magic: \0asm\1\0\0\0
  const uint8_t Data[] = {0x00, 0x61, 0x73, 0x6D, 0x01, 0x00, 0x00, 0x00};
  EXPECT_FALSE(WasmEdge::WAT::maybeWAT(Data));
}

TEST(MaybeWATTest, NullByteInMiddle) {
  const uint8_t Data[] = {'(', 'm', 'o', 'd', 0x00, 'l', 'e', ')'};
  EXPECT_FALSE(WasmEdge::WAT::maybeWAT(Data));
}

TEST(MaybeWATTest, InvalidUTF8WithParen) {
  // Starts with '(' but contains invalid UTF-8
  const uint8_t Data[] = {'(', 'm', 'o', 'd', 'u', 'l', 'e', 0xFF};
  EXPECT_FALSE(WasmEdge::WAT::maybeWAT(Data));
}

TEST(MaybeWATTest, LoneContinuationByte) {
  const uint8_t Data[] = {'(', 'm', 'o', 'd', 'u', 'l', 'e', 0x80};
  EXPECT_FALSE(WasmEdge::WAT::maybeWAT(Data));
}

TEST(MaybeWATTest, OverlongEncoding) {
  // C0 AF — overlong encoding of U+002F '/'
  const uint8_t Data[] = {'(', 'm', 'o', 'd', 'u', 'l', 0xC0, 0xAF};
  EXPECT_FALSE(WasmEdge::WAT::maybeWAT(Data));
}

TEST(MaybeWATTest, SurrogateHalf) {
  // U+D800 (high surrogate): ED A0 80
  const uint8_t Data[] = {'(', 'm', 'o', 'd', 'u', 0xED, 0xA0, 0x80};
  EXPECT_FALSE(WasmEdge::WAT::maybeWAT(Data));
}

TEST(MaybeWATTest, AboveMaxCodepoint) {
  // U+110000: F4 90 80 80
  const uint8_t Data[] = {'(', 'm', 'o', 'd', 0xF4, 0x90, 0x80, 0x80};
  EXPECT_FALSE(WasmEdge::WAT::maybeWAT(Data));
}

} // namespace
