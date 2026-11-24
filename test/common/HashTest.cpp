// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "common/hash.h"

#include <cstdint>
#include <gtest/gtest.h>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace {
using namespace std::literals;

// Helper: hash a raw byte buffer via the public API.
uint64_t hashBytes(const void *Data, size_t Len) {
  WasmEdge::Hash::Hash H;
  return H(std::string_view(static_cast<const char *>(Data), Len));
}

TEST(HashTest, Bytes) {
  // Test hashing byte buffers.
  //
  //   1.  Deterministic.
  //   2.  Different inputs different hashes.
  //   3.  Empty input.
  //   4.  Single byte.
  //   5.  Size classes.
  //   6.  Avalanche.
  //   7.  Length sensitivity.
  //   8.  Tail read after loop is in bounds.

  // Determinism: same input always produces same output within a process.
  {
    const auto A = hashBytes("hello", 5);
    const auto B = hashBytes("hello", 5);
    EXPECT_EQ(A, B);
  }

  // Different inputs should (almost certainly) produce different hashes.
  {
    const auto A = hashBytes("hello", 5);
    const auto B = hashBytes("world", 5);
    EXPECT_NE(A, B);
  }

  // Empty input should produce a valid (non-crashing) hash.
  {
    const auto H = hashBytes("", 0);
    // Just verify it doesn't crash and returns something.
    (void)H;
  }

  // Single byte inputs should all produce distinct hashes.
  {
    std::unordered_set<uint64_t> Seen;
    for (int I = 0; I < 256; ++I) {
      uint8_t Byte = static_cast<uint8_t>(I);
      Seen.insert(hashBytes(&Byte, 1));
    }
    // 256 distinct single-byte inputs should give at least 256 distinct hashes.
    EXPECT_EQ(Seen.size(), 256u);
  }

  // Test each size class boundary: 1-3, 4-7, 8-16, 17-32, 33-48, 49+ bytes.
  {
    // Build inputs of varying lengths.
    std::vector<size_t> Sizes = {1,  2,  3,  4,  5,  7,  8,  12,  16, 17,
                                 24, 32, 33, 48, 49, 64, 96, 100, 128};
    std::unordered_set<uint64_t> Seen;
    for (auto Size : Sizes) {
      std::string Input(Size, 'x');
      Seen.insert(hashBytes(Input.data(), Input.size()));
    }
    // All different-length same-char inputs should hash differently.
    EXPECT_EQ(Seen.size(), Sizes.size());
  }

  // Avalanche: flipping one bit should change the hash.
  {
    char Buf[16] = {};
    const auto Base = hashBytes(Buf, 16);
    // Flip each bit in the first byte and check hash changes.
    for (int Bit = 0; Bit < 8; ++Bit) {
      Buf[0] = static_cast<char>(1 << Bit);
      EXPECT_NE(hashBytes(Buf, 16), Base) << "bit " << Bit;
      Buf[0] = 0;
    }
  }

  // Length sensitivity: same content prefix but different lengths should
  // differ.
  {
    const char Buf[64] = {};
    std::unordered_set<uint64_t> Seen;
    for (size_t Len = 0; Len <= 64; ++Len) {
      Seen.insert(hashBytes(Buf, Len));
    }
    EXPECT_EQ(Seen.size(), 65u);
  }

  // Tail read must stay in bounds when the >48-byte loop leaves a short
  // remainder; run under -fsanitize=undefined to detect the underflow.
  {
    // Lengths where size % 48 falls in [1, 15], plus in-bounds controls.
    const std::vector<size_t> Affected = {49, 50, 63, 97, 100, 145};
    const std::vector<size_t> Controls = {17, 32, 48, 64, 96, 128};

    std::unordered_set<uint64_t> Seen;
    for (auto Size : Affected) {
      std::string Input(Size, 'x');
      Seen.insert(hashBytes(Input.data(), Input.size()));
    }
    for (auto Size : Controls) {
      std::string Input(Size, 'x');
      Seen.insert(hashBytes(Input.data(), Input.size()));
    }
    EXPECT_EQ(Seen.size(), Affected.size() + Controls.size());
  }
}

TEST(HashTest, Functor) {
  // Test the hash functor.
  //
  //   1.  Overloads.
  //   2.  Integral hash.

  // The Hash functor works with string_view, std::string, and C-strings.
  {
    WasmEdge::Hash::Hash H;
    const auto FromView = H("test"sv);
    const auto FromStr = H(std::string("test"));
    const auto FromCStr = H("test");
    EXPECT_EQ(FromView, FromStr);
    EXPECT_EQ(FromView, FromCStr);
  }

  // Integral hashing should produce distinct values for small integers.
  {
    WasmEdge::Hash::Hash H;
    std::unordered_set<uint64_t> Seen;
    for (int I = 0; I < 1000; ++I) {
      Seen.insert(H(I));
    }
    EXPECT_EQ(Seen.size(), 1000u);
  }
}

} // namespace
