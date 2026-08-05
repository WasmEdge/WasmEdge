// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "common/int128.h"
#include "fmt/format.h"

#include <cstdint>
#include <limits>
#include <gtest/gtest.h>

namespace {
using namespace std::literals;

TEST(Int128Test, Int128OutputTest) {
  {
    const WasmEdge::uint128_t Value = 0U;
    EXPECT_EQ(fmt::format("{}"sv, WasmEdge::uint128(Value)), "0");
    EXPECT_EQ(fmt::format("{:x}"sv, WasmEdge::uint128(Value)), "0");
    EXPECT_EQ(fmt::format("{:#x}"sv, WasmEdge::uint128(Value)), "0x0");
    EXPECT_EQ(fmt::format("{:b}"sv, WasmEdge::uint128(Value)), "0");
    EXPECT_EQ(fmt::format("{:#b}"sv, WasmEdge::uint128(Value)), "0b0");
    EXPECT_EQ(fmt::format("{:o}"sv, WasmEdge::uint128(Value)), "0");
  }

  {
    const WasmEdge::uint128_t Value = WasmEdge::uint128_t(1U) << 69U;
    EXPECT_EQ(fmt::format("{}"sv, WasmEdge::uint128(Value)),
              "590295810358705651712");
    EXPECT_EQ(fmt::format("{:x}"sv, WasmEdge::uint128(Value)),
              "200000000000000000");
  }

  {
    const WasmEdge::uint128_t Value = WasmEdge::uint128_t(1U) << 127U;
    EXPECT_EQ(fmt::format("{}"sv, WasmEdge::uint128(Value)),
              "170141183460469231731687303715884105728");
    EXPECT_EQ(fmt::format("{:x}"sv, WasmEdge::uint128(Value)),
              "80000000000000000000000000000000");
  }

  {
    const WasmEdge::uint128_t Value = ~WasmEdge::uint128_t(0U);
    EXPECT_EQ(fmt::format("{}"sv, WasmEdge::uint128(Value)),
              "340282366920938463463374607431768211455");
    EXPECT_EQ(fmt::format("{:x}"sv, WasmEdge::uint128(Value)),
              "ffffffffffffffffffffffffffffffff");
    EXPECT_EQ(fmt::format("{:#X}"sv, WasmEdge::uint128(Value)),
              "0XFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF");
  }

  {
    const WasmEdge::uint128_t P10(10000000000000U);
    const WasmEdge::uint128_t Value =
        WasmEdge::uint128_t(1234567890123U) * P10 * P10 +
        WasmEdge::uint128_t(1234567890123U);
    EXPECT_EQ(fmt::format("{}"sv, WasmEdge::uint128(Value)),
              "123456789012300000000000001234567890123");
    EXPECT_EQ(fmt::format("{:x}"sv, WasmEdge::uint128(Value)),
              "5ce0e9a55ff035e3783f03ea3dfb04cb");
    EXPECT_EQ(
        fmt::format("{:b}"sv, WasmEdge::uint128(Value)),
        "1011100111000001110100110100101010111111111000000110101111000110111100"
        "000111111000000111110101000111101111110110000010011001011");
  }

  {
    const WasmEdge::uint128_t P10(10000000000000U);
    const WasmEdge::uint128_t Value =
        WasmEdge::uint128_t(1234567890123U) * P10 * P10;
    EXPECT_EQ(fmt::format("{}"sv, WasmEdge::uint128(Value)),
              "123456789012300000000000000000000000000");
    EXPECT_EQ(fmt::format("{:o}"sv, WasmEdge::uint128(Value)),
              "1347016464527770065706740770054531400000000");
  }

  {
    const WasmEdge::uint128_t P10(10000000000000U);
    const WasmEdge::uint128_t Value =
        WasmEdge::uint128_t(1234567890123U) * P10 * P10 +
        WasmEdge::uint128_t(1234567890123U) * P10;
    EXPECT_EQ(fmt::format("{}"sv, WasmEdge::uint128(Value)),
              "123456789012312345678901230000000000000");
  }

  {
    auto Convert = [](WasmEdge::uint128_t V) -> fmt::detail::uint128_t {
      return (fmt::detail::uint128_t(static_cast<uint64_t>(V >> 64U)) << 64U) |
             fmt::detail::uint128_t(static_cast<uint64_t>(V));
    };
    std::string S0 = "1"s;
    std::string S9;
    WasmEdge::uint128_t X(1U);
    for (unsigned int I = 1U; I <= 38U; ++I) {
      X *= 10U;
      const auto Y = X - WasmEdge::uint128_t(1U);
      EXPECT_EQ(fmt::detail::count_digits(Convert(Y - WasmEdge::uint128_t(1U))),
                I);
      EXPECT_EQ(fmt::detail::count_digits(Convert(Y)), I);
      EXPECT_EQ(fmt::detail::count_digits(Convert(X)), I + 1);
      EXPECT_EQ(fmt::detail::count_digits(Convert(X + WasmEdge::uint128_t(1U))),
                I + 1);
      S0 += '0';
      S9 += '9';
      EXPECT_EQ(fmt::format("{}"sv, WasmEdge::uint128(X)), S0);
      EXPECT_EQ(fmt::format("{}"sv, WasmEdge::uint128(Y)), S9);
    }
  }
}

TEST(Int128Test, Int128ArithmeticTest) {
  // Boundary Constants
  const WasmEdge::uint128_t U64Max = std::numeric_limits<uint64_t>::max();
  const WasmEdge::uint128_t U128Max = ~WasmEdge::uint128_t(0);
  
  // Addition Overflow and Wrapping
  EXPECT_EQ(U64Max + WasmEdge::uint128_t(1), WasmEdge::uint128_t(1) << 64U);
  EXPECT_EQ(U128Max + WasmEdge::uint128_t(1), WasmEdge::uint128_t(0));
  
  // Subtraction Underflow
  EXPECT_EQ(WasmEdge::uint128_t(0) - WasmEdge::uint128_t(1), U128Max);
  EXPECT_EQ((WasmEdge::uint128_t(1) << 64U) - WasmEdge::uint128_t(1), U64Max);
  
  // Multiplication
  const WasmEdge::uint128_t U32Max = std::numeric_limits<uint32_t>::max();
  EXPECT_EQ(U32Max * U32Max, U64Max - U32Max * 2); 
  
  const WasmEdge::uint128_t MulWide = U64Max * U64Max;
  WasmEdge::uint128_t ExpectedMulWide = U128Max - (U64Max << 1U);
  EXPECT_EQ(MulWide, ExpectedMulWide);

  // Multiplication Sanity Checks
  EXPECT_EQ(U128Max * WasmEdge::uint128_t(0), WasmEdge::uint128_t(0));
  EXPECT_EQ(U128Max * WasmEdge::uint128_t(1), U128Max);

  // Multiplication Overflow Wrapping
  EXPECT_EQ(U128Max * WasmEdge::uint128_t(2), U128Max - WasmEdge::uint128_t(1));
  EXPECT_EQ(U128Max * U128Max, WasmEdge::uint128_t(1));
  
  // Division and Remainder Sanity Checks
  EXPECT_EQ(WasmEdge::uint128_t(0) / WasmEdge::uint128_t(1), WasmEdge::uint128_t(0));
  EXPECT_EQ(WasmEdge::uint128_t(1) / WasmEdge::uint128_t(1), WasmEdge::uint128_t(1));
  EXPECT_EQ(WasmEdge::uint128_t(10) / WasmEdge::uint128_t(2), WasmEdge::uint128_t(5));

  // Division and Remainder Boundaries
  EXPECT_EQ(U128Max / U128Max, WasmEdge::uint128_t(1));
  EXPECT_EQ(U128Max % U128Max, WasmEdge::uint128_t(0));
  EXPECT_EQ(U128Max / WasmEdge::uint128_t(2), U128Max >> 1U);
}

TEST(Int128Test, Int128BitwiseTest) {
  const WasmEdge::uint128_t Zero = 0U;
  const WasmEdge::uint128_t U128Max = ~Zero;
  const WasmEdge::uint128_t U64Max = std::numeric_limits<uint64_t>::max();
  const WasmEdge::uint128_t HighMask = U128Max ^ U64Max;

  // Bitwise AND
  EXPECT_EQ(U128Max & U64Max, U64Max);
  EXPECT_EQ(U128Max & Zero, Zero);
  EXPECT_EQ(HighMask & U64Max, Zero);

  // Bitwise OR
  EXPECT_EQ(HighMask | U64Max, U128Max);
  EXPECT_EQ(Zero | U64Max, U64Max);

  // Bitwise XOR
  EXPECT_EQ(U128Max ^ U128Max, Zero);
  EXPECT_EQ(HighMask ^ U64Max, U128Max);

  // Bitwise NOT
  EXPECT_EQ(~U128Max, Zero);
  EXPECT_EQ(~Zero, U128Max);
}

TEST(Int128Test, Int128ShiftTest) {
  const WasmEdge::uint128_t One = 1U;
  const WasmEdge::uint128_t U128Max = ~WasmEdge::uint128_t(0);
  const WasmEdge::uint128_t U64Max = std::numeric_limits<uint64_t>::max();

  // Left and Right Shift Boundaries
  EXPECT_EQ((One << 64U) >> 64U, One);
  EXPECT_EQ((One << 127U) >> 127U, One);
  EXPECT_EQ(U128Max >> 64U, U64Max);
  // Shift Boundaries
  const WasmEdge::uint128_t ExpectedShift = WasmEdge::uint128_t(U64Max) << 64U;
  EXPECT_EQ(U128Max << 64U, ExpectedShift);
}

TEST(Int128Test, Int128ComparisonTest) {
  const WasmEdge::uint128_t One = 1U;
  const WasmEdge::uint128_t Two = 2U;
  const WasmEdge::uint128_t Large = WasmEdge::uint128_t(1) << 100U;

  EXPECT_TRUE(One == One);
  EXPECT_FALSE(One == Two);

  EXPECT_TRUE(One != Two);
  EXPECT_FALSE(One != One);

  EXPECT_TRUE(One < Two);
  EXPECT_TRUE(One < Large);
  EXPECT_FALSE(Two < One);

  EXPECT_TRUE(Large > Two);
  EXPECT_FALSE(One > Large);

  EXPECT_TRUE(One <= One);
  EXPECT_TRUE(One <= Two);

  EXPECT_TRUE(Two >= Two);
  EXPECT_TRUE(Large >= Two);
}

} // namespace
