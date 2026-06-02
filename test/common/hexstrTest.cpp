// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/test/common/hexstrTest.cpp - Hex string unit tests -------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains unit tests for the hex string conversion utilities
/// defined in common/hexstr.h.
///
//===----------------------------------------------------------------------===//

#include "common/hexstr.h"

#include <cstdint>
#include <gtest/gtest.h>
#include <string>
#include <vector>

namespace {
using namespace std::literals;

// ============================================================================
// convertCharToHex
// ============================================================================

TEST(HexStrTest, ConvertCharToHex_Digits) {
  // All decimal digit characters '0'–'9' map to 0–9.
  EXPECT_EQ(WasmEdge::convertCharToHex('0'), UINT8_C(0));
  EXPECT_EQ(WasmEdge::convertCharToHex('1'), UINT8_C(1));
  EXPECT_EQ(WasmEdge::convertCharToHex('5'), UINT8_C(5));
  EXPECT_EQ(WasmEdge::convertCharToHex('9'), UINT8_C(9));
}

TEST(HexStrTest, ConvertCharToHex_LowerHex) {
  // Lowercase hex letters 'a'–'f' map to 10–15.
  EXPECT_EQ(WasmEdge::convertCharToHex('a'), UINT8_C(10));
  EXPECT_EQ(WasmEdge::convertCharToHex('b'), UINT8_C(11));
  EXPECT_EQ(WasmEdge::convertCharToHex('c'), UINT8_C(12));
  EXPECT_EQ(WasmEdge::convertCharToHex('d'), UINT8_C(13));
  EXPECT_EQ(WasmEdge::convertCharToHex('e'), UINT8_C(14));
  EXPECT_EQ(WasmEdge::convertCharToHex('f'), UINT8_C(15));
}

TEST(HexStrTest, ConvertCharToHex_UpperHex) {
  // Uppercase hex letters 'A'–'F' map to 10–15.
  EXPECT_EQ(WasmEdge::convertCharToHex('A'), UINT8_C(10));
  EXPECT_EQ(WasmEdge::convertCharToHex('B'), UINT8_C(11));
  EXPECT_EQ(WasmEdge::convertCharToHex('C'), UINT8_C(12));
  EXPECT_EQ(WasmEdge::convertCharToHex('D'), UINT8_C(13));
  EXPECT_EQ(WasmEdge::convertCharToHex('E'), UINT8_C(14));
  EXPECT_EQ(WasmEdge::convertCharToHex('F'), UINT8_C(15));
}

TEST(HexStrTest, ConvertCharToHex_InvalidChars) {
  // All characters outside 0-9, a-f, A-F silently return 0.
  // 'g' is just past 'f', 'G' is just past 'F'.
  EXPECT_EQ(WasmEdge::convertCharToHex('g'), UINT8_C(0));
  EXPECT_EQ(WasmEdge::convertCharToHex('G'), UINT8_C(0));
  EXPECT_EQ(WasmEdge::convertCharToHex('z'), UINT8_C(0));
  EXPECT_EQ(WasmEdge::convertCharToHex(' '), UINT8_C(0));
  EXPECT_EQ(WasmEdge::convertCharToHex('\0'), UINT8_C(0));
  // '/' is one below '0', ':' is one above '9'.
  EXPECT_EQ(WasmEdge::convertCharToHex('/'), UINT8_C(0));
  EXPECT_EQ(WasmEdge::convertCharToHex(':'), UINT8_C(0));
}

// ============================================================================
// convertBytesToHexStr
// ============================================================================

TEST(HexStrTest, ConvertBytesToHexStr_Empty) {
  // Empty span: Dst is cleared to empty string.
  std::string Dst = "old_value"s;
  WasmEdge::convertBytesToHexStr({}, Dst);
  EXPECT_EQ(Dst, ""s);
}

TEST(HexStrTest, ConvertBytesToHexStr_SingleByte_BigEndian) {
  std::string Dst;
  const std::vector<uint8_t> Src = {0xAB};
  WasmEdge::convertBytesToHexStr(Src, Dst);
  EXPECT_EQ(Dst, "ab"s);
}

TEST(HexStrTest, ConvertBytesToHexStr_SingleByte_LittleEndian) {
  // One byte: reverse of one element is itself.
  std::string Dst;
  const std::vector<uint8_t> Src = {0xAB};
  WasmEdge::convertBytesToHexStr(Src, Dst, 0, true);
  EXPECT_EQ(Dst, "ab"s);
}

TEST(HexStrTest, ConvertBytesToHexStr_MultiBytes_BigEndian) {
  // Big-endian: first byte appears first in output.
  std::string Dst;
  const std::vector<uint8_t> Src = {0x01, 0x02, 0x03};
  WasmEdge::convertBytesToHexStr(Src, Dst);
  EXPECT_EQ(Dst, "010203"s);
}

TEST(HexStrTest, ConvertBytesToHexStr_MultiBytes_LittleEndian) {
  // Little-endian: bytes are reversed — last byte appears first in output.
  std::string Dst;
  const std::vector<uint8_t> Src = {0x01, 0x02, 0x03};
  WasmEdge::convertBytesToHexStr(Src, Dst, 0, true);
  EXPECT_EQ(Dst, "030201"s);
}

TEST(HexStrTest, ConvertBytesToHexStr_ZeroByte) {
  // 0x00 must format as "00", not "0".
  std::string Dst;
  const std::vector<uint8_t> Src = {0x00};
  WasmEdge::convertBytesToHexStr(Src, Dst);
  EXPECT_EQ(Dst, "00"s);
}

TEST(HexStrTest, ConvertBytesToHexStr_AllFF) {
  std::string Dst;
  const std::vector<uint8_t> Src = {0xFF, 0xFF};
  WasmEdge::convertBytesToHexStr(Src, Dst);
  EXPECT_EQ(Dst, "ffff"s);
}

TEST(HexStrTest, ConvertBytesToHexStr_Padding_Shorter) {
  // When hex output is shorter than Padding, leading '0's are prepended.
  // {0x01} → "01" (2 chars). Padding=6 → prepend 4 zeros → "000001".
  std::string Dst;
  const std::vector<uint8_t> Src = {0x01};
  WasmEdge::convertBytesToHexStr(Src, Dst, 6);
  EXPECT_EQ(Dst, "000001"s);
}

TEST(HexStrTest, ConvertBytesToHexStr_Padding_Equal) {
  // Padding == hex length: no change.
  std::string Dst;
  const std::vector<uint8_t> Src = {0xAB, 0xCD};
  WasmEdge::convertBytesToHexStr(Src, Dst, 4);
  EXPECT_EQ(Dst, "abcd"s);
}

TEST(HexStrTest, ConvertBytesToHexStr_Padding_Longer) {
  // When hex output is already longer than Padding, no truncation occurs.
  std::string Dst;
  const std::vector<uint8_t> Src = {0xAB, 0xCD, 0xEF};
  WasmEdge::convertBytesToHexStr(Src, Dst, 2);
  EXPECT_EQ(Dst, "abcdef"s);
}

TEST(HexStrTest, ConvertBytesToHexStr_Padding_LittleEndian) {
  // Padding also works with little-endian: {0x01, 0x00} reversed → "0001",
  // Padding=6 → "000001".
  std::string Dst;
  const std::vector<uint8_t> Src = {0x01, 0x00};
  WasmEdge::convertBytesToHexStr(Src, Dst, 6, true);
  EXPECT_EQ(Dst, "000001"s);
}

TEST(HexStrTest, ConvertBytesToHexStr_ClearsExistingContent) {
  // Dst.clear() is called first, so stale content is replaced.
  std::string Dst = "staledata"s;
  const std::vector<uint8_t> Src = {0xBE, 0xEF};
  WasmEdge::convertBytesToHexStr(Src, Dst);
  EXPECT_EQ(Dst, "beef"s);
}

// ============================================================================
// convertValVecToHexStr
// ============================================================================
// convertValVecToHexStr is a thin wrapper: convertBytesToHexStr(Src, Dst,
// Padding, /*IsLittleEndian=*/true). We verify the delegation is correct.

TEST(HexStrTest, ConvertValVecToHexStr_Empty) {
  std::string Dst = "old"s;
  WasmEdge::convertValVecToHexStr({}, Dst);
  EXPECT_EQ(Dst, ""s);
}

TEST(HexStrTest, ConvertValVecToHexStr_IsLittleEndian) {
  // {0x01, 0x02, 0x03} with LE → reversed → "030201".
  std::string Dst;
  const std::vector<uint8_t> Src = {0x01, 0x02, 0x03};
  WasmEdge::convertValVecToHexStr(Src, Dst);
  EXPECT_EQ(Dst, "030201"s);
}

TEST(HexStrTest, ConvertValVecToHexStr_Padding) {
  // {0x01} LE → "01" (2 chars), Padding=8 → "00000001".
  std::string Dst;
  const std::vector<uint8_t> Src = {0x01};
  WasmEdge::convertValVecToHexStr(Src, Dst, 8);
  EXPECT_EQ(Dst, "00000001"s);
}

TEST(HexStrTest, ConvertValVecToHexStr_SingleByte) {
  // Single byte: LE reverse of one byte is itself.
  std::string Dst;
  const std::vector<uint8_t> Src = {0xFF};
  WasmEdge::convertValVecToHexStr(Src, Dst);
  EXPECT_EQ(Dst, "ff"s);
}

// ============================================================================
// convertHexStrToBytes
// ============================================================================

TEST(HexStrTest, ConvertHexStrToBytes_Empty) {
  // Empty src: Dst is cleared and stays empty (early return before padding).
  std::vector<uint8_t> Dst = {0x01, 0x02};
  WasmEdge::convertHexStrToBytes(""sv, Dst);
  EXPECT_TRUE(Dst.empty());
}

TEST(HexStrTest, ConvertHexStrToBytes_EvenHexStr_BigEndian) {
  // "abcd" BE → [0xAB, 0xCD]
  std::vector<uint8_t> Dst;
  WasmEdge::convertHexStrToBytes("abcd"sv, Dst, 0, false);
  ASSERT_EQ(Dst.size(), 2U);
  EXPECT_EQ(Dst[0], UINT8_C(0xAB));
  EXPECT_EQ(Dst[1], UINT8_C(0xCD));
}

TEST(HexStrTest, ConvertHexStrToBytes_EvenHexStr_LittleEndian) {
  // "abcd" LE: iterate in reverse pairs.
  // String is "abcd". rbegin→'d','c' → low=d(13), high=c(12)*16=192 →
  // byte=205=0xCD. Next pair 'b','a' → low=b(11), high=a(10)*16=160 →
  // byte=171=0xAB. Result: [0xCD, 0xAB].
  std::vector<uint8_t> Dst;
  WasmEdge::convertHexStrToBytes("abcd"sv, Dst, 0, true);
  ASSERT_EQ(Dst.size(), 2U);
  EXPECT_EQ(Dst[0], UINT8_C(0xCD));
  EXPECT_EQ(Dst[1], UINT8_C(0xAB));
}

TEST(HexStrTest, ConvertHexStrToBytes_OddLengthStr) {
  // Odd-length string "abc" → prepend '0' → "0abc" → [0x0A, 0xBC].
  std::vector<uint8_t> Dst;
  WasmEdge::convertHexStrToBytes("abc"sv, Dst, 0, false);
  ASSERT_EQ(Dst.size(), 2U);
  EXPECT_EQ(Dst[0], UINT8_C(0x0A));
  EXPECT_EQ(Dst[1], UINT8_C(0xBC));
}

TEST(HexStrTest, ConvertHexStrToBytes_SinglePair) {
  // "ff" → [0xFF]
  std::vector<uint8_t> Dst;
  WasmEdge::convertHexStrToBytes("ff"sv, Dst, 0, false);
  ASSERT_EQ(Dst.size(), 1U);
  EXPECT_EQ(Dst[0], UINT8_C(0xFF));
}

TEST(HexStrTest, ConvertHexStrToBytes_Padding_OddBecomeEven) {
  // Odd Padding=3 → auto-incremented to 4.
  // "ab" (len 2) < 4 → pad to "00ab" → [0x00, 0xAB].
  std::vector<uint8_t> Dst;
  WasmEdge::convertHexStrToBytes("ab"sv, Dst, 3, false);
  ASSERT_EQ(Dst.size(), 2U);
  EXPECT_EQ(Dst[0], UINT8_C(0x00));
  EXPECT_EQ(Dst[1], UINT8_C(0xAB));
}

TEST(HexStrTest, ConvertHexStrToBytes_Padding_EvenPad) {
  // Padding=6, src="ab" → pad to "0000ab" → [0x00, 0x00, 0xAB].
  std::vector<uint8_t> Dst;
  WasmEdge::convertHexStrToBytes("ab"sv, Dst, 6, false);
  ASSERT_EQ(Dst.size(), 3U);
  EXPECT_EQ(Dst[0], UINT8_C(0x00));
  EXPECT_EQ(Dst[1], UINT8_C(0x00));
  EXPECT_EQ(Dst[2], UINT8_C(0xAB));
}

TEST(HexStrTest, ConvertHexStrToBytes_Padding_SrcLongerThanPadding) {
  // Src longer than Padding: no truncation, padding does not apply.
  // "aabbcc" (len 6) > Padding=2 → no extra zeros → [0xAA, 0xBB, 0xCC].
  std::vector<uint8_t> Dst;
  WasmEdge::convertHexStrToBytes("aabbcc"sv, Dst, 2, false);
  ASSERT_EQ(Dst.size(), 3U);
  EXPECT_EQ(Dst[0], UINT8_C(0xAA));
  EXPECT_EQ(Dst[1], UINT8_C(0xBB));
  EXPECT_EQ(Dst[2], UINT8_C(0xCC));
}

TEST(HexStrTest, ConvertHexStrToBytes_UppercaseChars) {
  // Uppercase "ABCD" → same as lowercase "abcd" BE → [0xAB, 0xCD].
  std::vector<uint8_t> Dst;
  WasmEdge::convertHexStrToBytes("ABCD"sv, Dst, 0, false);
  ASSERT_EQ(Dst.size(), 2U);
  EXPECT_EQ(Dst[0], UINT8_C(0xAB));
  EXPECT_EQ(Dst[1], UINT8_C(0xCD));
}

TEST(HexStrTest, ConvertHexStrToBytes_AllZeroes) {
  // "0000" → [0x00, 0x00]
  std::vector<uint8_t> Dst;
  WasmEdge::convertHexStrToBytes("0000"sv, Dst, 0, false);
  ASSERT_EQ(Dst.size(), 2U);
  EXPECT_EQ(Dst[0], UINT8_C(0x00));
  EXPECT_EQ(Dst[1], UINT8_C(0x00));
}

TEST(HexStrTest, ConvertHexStrToBytes_EmptyPaddingIsNoop) {
  // Empty src always returns empty dst regardless of Padding value.
  // The Padding logic is skipped due to the early return on empty src.
  std::vector<uint8_t> Dst = {0x99};
  WasmEdge::convertHexStrToBytes(""sv, Dst, 8, false);
  EXPECT_TRUE(Dst.empty());
}

TEST(HexStrTest, ConvertHexStrToBytes_ClearsExistingDst) {
  // Dst.clear() is called first.
  std::vector<uint8_t> Dst = {0xDE, 0xAD};
  WasmEdge::convertHexStrToBytes("cafe"sv, Dst, 0, false);
  ASSERT_EQ(Dst.size(), 2U);
  EXPECT_EQ(Dst[0], UINT8_C(0xCA));
  EXPECT_EQ(Dst[1], UINT8_C(0xFE));
}

// ============================================================================
// convertHexStrToValVec
// ============================================================================
// Delegates to convertHexStrToBytes with IsLittleEndian=false (big-endian).

TEST(HexStrTest, ConvertHexStrToValVec_Empty) {
  std::vector<uint8_t> Dst = {0x01};
  WasmEdge::convertHexStrToValVec(""sv, Dst);
  EXPECT_TRUE(Dst.empty());
}

TEST(HexStrTest, ConvertHexStrToValVec_BigEndian) {
  // "0102" BE → [0x01, 0x02]
  std::vector<uint8_t> Dst;
  WasmEdge::convertHexStrToValVec("0102"sv, Dst);
  ASSERT_EQ(Dst.size(), 2U);
  EXPECT_EQ(Dst[0], UINT8_C(0x01));
  EXPECT_EQ(Dst[1], UINT8_C(0x02));
}

TEST(HexStrTest, ConvertHexStrToValVec_Padding) {
  // "ff" (len 2) < Padding=4 → "00ff" → [0x00, 0xFF].
  std::vector<uint8_t> Dst;
  WasmEdge::convertHexStrToValVec("ff"sv, Dst, 4);
  ASSERT_EQ(Dst.size(), 2U);
  EXPECT_EQ(Dst[0], UINT8_C(0x00));
  EXPECT_EQ(Dst[1], UINT8_C(0xFF));
}

// ============================================================================
// convertUIntToHexStr
// ============================================================================

TEST(HexStrTest, ConvertUIntToHexStr_Zero) {
  // 0 with default MinLen=8 → "0x00000000"
  EXPECT_EQ(WasmEdge::convertUIntToHexStr(0ULL), "0x00000000"s);
}

TEST(HexStrTest, ConvertUIntToHexStr_One) {
  // 1 with default MinLen=8 → "0x00000001"
  EXPECT_EQ(WasmEdge::convertUIntToHexStr(1ULL), "0x00000001"s);
}

TEST(HexStrTest, ConvertUIntToHexStr_MaxUInt32) {
  // 0xFFFFFFFF with MinLen=8 → "0xffffffff"
  EXPECT_EQ(WasmEdge::convertUIntToHexStr(0xFFFFFFFFULL, 8), "0xffffffff"s);
}

TEST(HexStrTest, ConvertUIntToHexStr_MaxUInt64) {
  // UINT64_MAX with MinLen=16 → "0xffffffffffffffff"
  EXPECT_EQ(WasmEdge::convertUIntToHexStr(UINT64_MAX, 16),
            "0xffffffffffffffff"s);
}

TEST(HexStrTest, ConvertUIntToHexStr_MinLenZero) {
  // MinLen=0 means no zero-padding; value 0xFF → "0xff".
  EXPECT_EQ(WasmEdge::convertUIntToHexStr(0xFFULL, 0), "0xff"s);
}

TEST(HexStrTest, ConvertUIntToHexStr_MinLenSmallerThanValue) {
  // Value wider than MinLen: no truncation, output grows to fit.
  // 0xABCD with MinLen=2 → "0xabcd" (4 hex digits, not truncated).
  EXPECT_EQ(WasmEdge::convertUIntToHexStr(0xABCDULL, 2), "0xabcd"s);
}

TEST(HexStrTest, ConvertUIntToHexStr_MinLenClamped) {
  // MinLen > 16 is clamped to 16: same result as MinLen=16.
  // Value 0 with MinLen=100 → "0x0000000000000000" (16 zeros).
  EXPECT_EQ(WasmEdge::convertUIntToHexStr(0ULL, 100),
            WasmEdge::convertUIntToHexStr(0ULL, 16));
  EXPECT_EQ(WasmEdge::convertUIntToHexStr(0ULL, 100),
            "0x0000000000000000"s);
}

TEST(HexStrTest, ConvertUIntToHexStr_MinLenExact16) {
  // MinLen=16 (at the boundary, no clamping):
  EXPECT_EQ(WasmEdge::convertUIntToHexStr(0ULL, 16),
            "0x0000000000000000"s);
}

TEST(HexStrTest, ConvertUIntToHexStr_MinLen17ClampsTo16) {
  // MinLen=17 clamps to 16 — indistinguishable from MinLen=16.
  EXPECT_EQ(WasmEdge::convertUIntToHexStr(0ULL, 17),
            WasmEdge::convertUIntToHexStr(0ULL, 16));
}

TEST(HexStrTest, ConvertUIntToHexStr_AlwaysLowerCase) {
  // The implementation uses {:x} so output is always lowercase.
  EXPECT_EQ(WasmEdge::convertUIntToHexStr(0xABCDEFULL, 6), "0xabcdef"s);
}

TEST(HexStrTest, ConvertUIntToHexStr_AlwaysHasPrefix) {
  // Result always starts with "0x".
  const auto Result = WasmEdge::convertUIntToHexStr(42ULL, 0);
  EXPECT_EQ(Result.substr(0, 2), "0x"s);
}

// ============================================================================
// Round-trip tests
// ============================================================================

TEST(HexStrTest, RoundTrip_BytesToHexStr_ToBigEndianBytes) {
  // bytes → hex (BE) → bytes (BE) gives back original bytes.
  const std::vector<uint8_t> Original = {0xDE, 0xAD, 0xBE, 0xEF};
  std::string Hex;
  WasmEdge::convertBytesToHexStr(Original, Hex);

  std::vector<uint8_t> Recovered;
  WasmEdge::convertHexStrToBytes(Hex, Recovered, 0, false);

  ASSERT_EQ(Recovered.size(), Original.size());
  for (size_t I = 0; I < Original.size(); ++I) {
    EXPECT_EQ(Recovered[I], Original[I]) << "Mismatch at index " << I;
  }
}

TEST(HexStrTest, RoundTrip_ValVecToHexStr_ToValVec) {
  // ValVec uses LE for hex output but convertHexStrToValVec uses BE decode.
  // They are NOT inverses of each other — we verify ValVec→hex→bytes(LE) =
  // original. Use convertBytesToHexStr(LE) then convertHexStrToBytes(LE).
  const std::vector<uint8_t> Original = {0x01, 0x02, 0x03, 0x04};
  std::string Hex;
  WasmEdge::convertBytesToHexStr(Original, Hex, 0, true); // LE hex

  std::vector<uint8_t> Recovered;
  WasmEdge::convertHexStrToBytes(Hex, Recovered, 0, true); // LE decode

  ASSERT_EQ(Recovered.size(), Original.size());
  for (size_t I = 0; I < Original.size(); ++I) {
    EXPECT_EQ(Recovered[I], Original[I]) << "Mismatch at index " << I;
  }
}

TEST(HexStrTest, RoundTrip_SingleByte_AllValues) {
  // Every possible byte value round-trips correctly through BE hex.
  for (uint32_t V = 0; V <= 0xFF; ++V) {
    const std::vector<uint8_t> Src = {static_cast<uint8_t>(V)};
    std::string Hex;
    WasmEdge::convertBytesToHexStr(Src, Hex);

    std::vector<uint8_t> Dst;
    WasmEdge::convertHexStrToBytes(Hex, Dst, 0, false);

    ASSERT_EQ(Dst.size(), 1U) << "Failed for byte value " << V;
    EXPECT_EQ(Dst[0], static_cast<uint8_t>(V)) << "Failed for byte value " << V;
  }
}

} // namespace
