// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/test/aot/AOTBlake3Test.cpp - blake3 hash unit tests ------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contents unit tests of calculating blake3 hash.
///
//===----------------------------------------------------------------------===//

#include "aot/blake3.h"

#include "common/types.h"
#include "experimental/span.hpp"

#include <array>
#include <cstddef>
#include <gtest/gtest.h>
#include <string_view>
#include <vector>

namespace {

using namespace std::literals::string_view_literals;
using HashArray = std::array<WasmEdge::Byte, 32>;

TEST(Blake3Test, Empty) {
  const auto Expect =
      "\xaf\x13\x49\xb9\xf5\xf9\xa1\xa6\xa0\x40\x4d\xea\x36\xdc\xc9\x49\x9b\xcb"
      "\x25\xc9\xad\xc1\x12\xb7\xcc\x9a\x93\xca\xe4\x1f\x32\x62"sv;
  WasmEdge::AOT::Blake3 Blake3;
  HashArray Output;
  Blake3.finalize(Output);
  for (size_t I = 0; I < Output.size(); ++I) {
    EXPECT_EQ(Output[I], static_cast<WasmEdge::Byte>(Expect[I]));
  }
}

TEST(Blake3Test, Small) {
  const auto Data = "a"sv;
  const auto Expect =
      "\x17\x76\x2f\xdd\xd9\x69\xa4\x53\x92\x5d\x65\x71\x7a\xc3\xee\xa2\x13\x20"
      "\xb6\x6b\x54\x34\x2f\xde\x15\x12\x8d\x6c\xaf\x21\x21\x5f"sv;
  WasmEdge::AOT::Blake3 Blake3;
  HashArray Output;
  Blake3.update(cxx20::span(
      reinterpret_cast<const WasmEdge::Byte *>(Data.data()), Data.size()));
  Blake3.finalize(Output);
  for (size_t I = 0; I < Output.size(); ++I) {
    EXPECT_EQ(Output[I], static_cast<WasmEdge::Byte>(Expect[I]));
  }
}

TEST(Blake3Test, Large) {
  const auto Data =
      "af1349b9f5f9a1a6a0404dea36dcc9499bcb25c9adc112b7cc9a93cae41f3262dba5865c"
      "0d91b17958e4d2cac98c338f85cbbda07b71a020ab16c391b5e7af4b7741362872909e93"
      "d6ce0779cd18c10aa35222d8b6a8f0bb6c416c69134b73a18409ee61fd95733781993e71"
      "d9fa298ce39a1150465ed0f2fb995757aefffbca"sv;
  const auto Expect =
      "\xe2\xf3\x57\x6d\xb1\x65\xc4\xc0\x43\x3f\xad\x18\x53\x34\x85\xf7\xeb\x00"
      "\x83\x3b\x33\x45\x7a\xf5\x96\x73\x73\x35\x05\xd1\x4f\x12"sv;
  WasmEdge::AOT::Blake3 Blake3;
  HashArray Output;
  Blake3.update(cxx20::span(
      reinterpret_cast<const WasmEdge::Byte *>(Data.data()), Data.size()));
  Blake3.finalize(Output);
  for (size_t I = 0; I < Output.size(); ++I) {
    EXPECT_EQ(Output[I], static_cast<WasmEdge::Byte>(Expect[I]));
  }
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
