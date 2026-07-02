// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/hexstr.h"

#include <cstdint>
#include <gtest/gtest.h>
#include <string>
#include <vector>

namespace {
using namespace std::literals;

// The hex/byte conversion helpers back the AOT cache key formatting
// (lib/aot/cache.cpp) and the wasi_logging plugin. Wrong padding, endianness,
// or round-trip handling silently corrupts cache keys, so these cases lock the
// contract for every conversion direction.

TEST(HexStrTest, CharToHex) {
  EXPECT_EQ(WasmEdge::convertCharToHex('0'), 0);
  EXPECT_EQ(WasmEdge::convertCharToHex('9'), 9);
  EXPECT_EQ(WasmEdge::convertCharToHex('a'), 10);
  EXPECT_EQ(WasmEdge::convertCharToHex('f'), 15);
  EXPECT_EQ(WasmEdge::convertCharToHex('A'), 10);
  EXPECT_EQ(WasmEdge::convertCharToHex('F'), 15);
  // Out-of-range characters map to 0.
  EXPECT_EQ(WasmEdge::convertCharToHex('g'), 0);
  EXPECT_EQ(WasmEdge::convertCharToHex(' '), 0);
  EXPECT_EQ(WasmEdge::convertCharToHex('\0'), 0);
}

TEST(HexStrTest, BytesToHexBigEndian) {
  const std::vector<uint8_t> Bytes{0x01, 0x23, 0xab};
  std::string Out;
  WasmEdge::convertBytesToHexStr(Bytes, Out);
  EXPECT_EQ(Out, "0123ab"sv);
}

TEST(HexStrTest, BytesToHexLittleEndian) {
  const std::vector<uint8_t> Bytes{0x01, 0x23, 0xab};
  std::string Out;
  WasmEdge::convertBytesToHexStr(Bytes, Out, 0, true);
  EXPECT_EQ(Out, "ab2301"sv);
}

TEST(HexStrTest, BytesToHexPadding) {
  const std::vector<uint8_t> Bytes{0xab};
  std::string Out;
  // Padding longer than the content left-pads with '0'.
  WasmEdge::convertBytesToHexStr(Bytes, Out, 6);
  EXPECT_EQ(Out, "0000ab"sv);
  // Padding shorter than the content is a no-op.
  WasmEdge::convertBytesToHexStr(Bytes, Out, 1);
  EXPECT_EQ(Out, "ab"sv);
}

TEST(HexStrTest, BytesToHexEmpty) {
  const std::vector<uint8_t> Bytes{};
  std::string Out = "dirty";
  WasmEdge::convertBytesToHexStr(Bytes, Out);
  EXPECT_TRUE(Out.empty());
}

TEST(HexStrTest, HexToBytesBigEndian) {
  std::vector<uint8_t> Dst;
  WasmEdge::convertHexStrToBytes("0123ab"sv, Dst);
  EXPECT_EQ(Dst, (std::vector<uint8_t>{0x01, 0x23, 0xab}));
}

TEST(HexStrTest, HexToBytesOddLengthPrependsZero) {
  std::vector<uint8_t> Dst;
  WasmEdge::convertHexStrToBytes("123"sv, Dst);
  EXPECT_EQ(Dst, (std::vector<uint8_t>{0x01, 0x23}));
}

TEST(HexStrTest, HexToBytesEmpty) {
  std::vector<uint8_t> Dst{0xff};
  WasmEdge::convertHexStrToBytes(""sv, Dst);
  EXPECT_TRUE(Dst.empty());
}

TEST(HexStrTest, HexToBytesInvalidCharsMapToZero) {
  // Non-hex characters decode to 0 via convertCharToHex rather than erroring.
  std::vector<uint8_t> Dst;
  WasmEdge::convertHexStrToBytes("zz"sv, Dst);
  EXPECT_EQ(Dst, (std::vector<uint8_t>{0x00}));
  WasmEdge::convertHexStrToBytes("0g"sv, Dst);
  EXPECT_EQ(Dst, (std::vector<uint8_t>{0x00}));
  WasmEdge::convertHexStrToBytes("a!"sv, Dst);
  EXPECT_EQ(Dst, (std::vector<uint8_t>{0xa0}));
}

TEST(HexStrTest, RoundTripBigEndian) {
  const std::vector<uint8_t> Bytes{0xde, 0xad, 0xbe, 0xef, 0x00, 0x7f};
  std::string Hex;
  WasmEdge::convertBytesToHexStr(Bytes, Hex);
  std::vector<uint8_t> Back;
  WasmEdge::convertHexStrToBytes(Hex, Back);
  EXPECT_EQ(Back, Bytes);
}

TEST(HexStrTest, RoundTripLittleEndian) {
  const std::vector<uint8_t> Bytes{0xde, 0xad, 0xbe, 0xef};
  std::string Hex;
  WasmEdge::convertBytesToHexStr(Bytes, Hex, 0, true);
  std::vector<uint8_t> Back;
  WasmEdge::convertHexStrToBytes(Hex, Back, 2, true);
  EXPECT_EQ(Back, Bytes);
}

TEST(HexStrTest, ValVecConversions) {
  // convertValVecToHexStr emits little-endian (byte-reversed) hex, while
  // convertHexStrToValVec parses big-endian; they are not inverse operations.
  const std::vector<uint8_t> Bytes{0x01, 0x02, 0x03};
  std::string Hex;
  WasmEdge::convertValVecToHexStr(Bytes, Hex);
  EXPECT_EQ(Hex, "030201"sv);
  std::vector<uint8_t> Dst;
  WasmEdge::convertHexStrToValVec("030201"sv, Dst);
  EXPECT_EQ(Dst, (std::vector<uint8_t>{0x03, 0x02, 0x01}));
}

TEST(HexStrTest, UIntToHexStr) {
  EXPECT_EQ(WasmEdge::convertUIntToHexStr(0), "0x00000000"sv);
  EXPECT_EQ(WasmEdge::convertUIntToHexStr(0xabcd, 4), "0xabcd"sv);
  EXPECT_EQ(WasmEdge::convertUIntToHexStr(0xff, 2), "0xff"sv);
  // MinLen is clamped to 16 hex digits (the width of a uint64_t).
  EXPECT_EQ(WasmEdge::convertUIntToHexStr(1, 100), "0x0000000000000001"sv);
}

} // namespace
