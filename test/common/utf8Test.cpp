// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "common/utf8.h"

#include <gtest/gtest.h>
#include <string_view>

namespace {

using namespace std::literals;
using WasmEdge::isValidUTF8;
using WasmEdge::utf8SequenceLength;

TEST(UTF8Test, Ascii) {
  EXPECT_TRUE(isValidUTF8(""sv));
  EXPECT_TRUE(isValidUTF8("abc"sv));
  EXPECT_TRUE(isValidUTF8("\x00\x7F"sv));
  EXPECT_EQ(utf8SequenceLength("a"sv, 0), 1U);
}

TEST(UTF8Test, SequenceLengths) {
  EXPECT_EQ(utf8SequenceLength("\xC2\x80"sv, 0), 2U);
  EXPECT_EQ(utf8SequenceLength("\xE2\x82\xAC"sv, 0), 3U);
  EXPECT_EQ(utf8SequenceLength("\xF0\x9F\x98\x80"sv, 0), 4U);
  EXPECT_TRUE(isValidUTF8("\xC2\xA9\xE2\x82\xAC\xF0\x9F\x98\x80"sv));
}

TEST(UTF8Test, HighestScalar) {
  EXPECT_TRUE(isValidUTF8("\xF4\x8F\xBF\xBF"sv));
  EXPECT_FALSE(isValidUTF8("\xF4\x90\x80\x80"sv));
  EXPECT_FALSE(isValidUTF8("\xF5\x80\x80\x80"sv));
}

TEST(UTF8Test, Overlong) {
  EXPECT_FALSE(isValidUTF8("\xC0\x80"sv));
  EXPECT_FALSE(isValidUTF8("\xC1\xBF"sv));
  EXPECT_FALSE(isValidUTF8("\xE0\x9F\xBF"sv));
  EXPECT_FALSE(isValidUTF8("\xF0\x8F\xBF\xBF"sv));
}

TEST(UTF8Test, Surrogates) {
  EXPECT_TRUE(isValidUTF8("\xED\x9F\xBF"sv));
  EXPECT_FALSE(isValidUTF8("\xED\xA0\x80"sv));
  EXPECT_FALSE(isValidUTF8("\xED\xBF\xBF"sv));
  EXPECT_TRUE(isValidUTF8("\xEE\x80\x80"sv));
}

TEST(UTF8Test, Truncation) {
  EXPECT_FALSE(isValidUTF8("\xC2"sv));
  EXPECT_FALSE(isValidUTF8("\xE2\x82"sv));
  EXPECT_FALSE(isValidUTF8("\xF0\x9F\x98"sv));
  EXPECT_FALSE(isValidUTF8("a\xF0"sv));
}

TEST(UTF8Test, BadBytes) {
  EXPECT_FALSE(isValidUTF8("\x80"sv));
  EXPECT_FALSE(isValidUTF8("\xBF"sv));
  EXPECT_FALSE(isValidUTF8("\xC2\x41"sv));
  EXPECT_FALSE(isValidUTF8("\xF8\x88\x80\x80\x80"sv));
  EXPECT_FALSE(isValidUTF8("\xFF"sv));
}

} // namespace
