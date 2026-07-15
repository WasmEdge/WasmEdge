// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "common/endian.h"

#include <cstdint>
#include <gtest/gtest.h>

namespace {

TEST(ByteswapTest, KnownValues) {
  EXPECT_EQ(WasmEdge::byteswap<uint16_t>(0x0102),
            static_cast<uint16_t>(0x0201));
  EXPECT_EQ(WasmEdge::byteswap<uint32_t>(0x01020304U), 0x04030201U);
  EXPECT_EQ(WasmEdge::byteswap<uint64_t>(0x0102030405060708ULL),
            0x0807060504030201ULL);
}

TEST(ByteswapTest, IsInvolution) {
  for (uint16_t V :
       {static_cast<uint16_t>(0x0000), static_cast<uint16_t>(0x0001),
        static_cast<uint16_t>(0xabcd), static_cast<uint16_t>(0xffff)}) {
    EXPECT_EQ(WasmEdge::byteswap(WasmEdge::byteswap(V)), V);
  }
  for (uint32_t V : {0x00000000U, 0x00000001U, 0xdeadbeefU, 0x12345678U}) {
    EXPECT_EQ(WasmEdge::byteswap(WasmEdge::byteswap(V)), V);
  }
  for (uint64_t V : {0x0000000000000000ULL, 0x0000000000000001ULL,
                     0x0123456789abcdefULL, 0xfedcba9876543210ULL}) {
    EXPECT_EQ(WasmEdge::byteswap(WasmEdge::byteswap(V)), V);
  }
}

TEST(ByteswapTest, SignedIntegers) {
  const int16_t Known16 = static_cast<int16_t>(0x0102);
  EXPECT_EQ(WasmEdge::byteswap(Known16),
            static_cast<int16_t>(0x0201));

  const int32_t Known32 = static_cast<int32_t>(0x01020304);
  EXPECT_EQ(WasmEdge::byteswap(Known32),
            static_cast<int32_t>(0x04030201));

  EXPECT_EQ(WasmEdge::byteswap<int16_t>(static_cast<int16_t>(-1)),
            static_cast<int16_t>(-1));
  EXPECT_EQ(WasmEdge::byteswap<int32_t>(static_cast<int32_t>(-1)),
            static_cast<int32_t>(-1));
  EXPECT_EQ(WasmEdge::byteswap<int64_t>(static_cast<int64_t>(-1)),
            static_cast<int64_t>(-1));

  const int16_t Negative16 = static_cast<int16_t>(-123);
  EXPECT_EQ(WasmEdge::byteswap(WasmEdge::byteswap(Negative16)),
            Negative16);

  const int32_t Negative32 = static_cast<int32_t>(-12345);
  EXPECT_EQ(WasmEdge::byteswap(WasmEdge::byteswap(Negative32)),
            Negative32);

  const int64_t Negative64 = static_cast<int64_t>(-123456789);
  EXPECT_EQ(WasmEdge::byteswap(WasmEdge::byteswap(Negative64)),
            Negative64);
}

} // namespace
