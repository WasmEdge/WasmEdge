// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "common/int128.h"
#include "fmt/format.h"

#include <cstdint>
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

TEST(Int128Test, Int128ClzTest) {
  EXPECT_EQ(WasmEdge::clz(static_cast<uint32_t>(0)), 32);
  EXPECT_EQ(WasmEdge::clz(static_cast<uint32_t>(1)), 31);
  EXPECT_EQ(WasmEdge::clz(static_cast<uint32_t>(0x80000000U)), 0);
  EXPECT_EQ(WasmEdge::clz(static_cast<uint64_t>(0)), 64);
  EXPECT_EQ(WasmEdge::clz(static_cast<uint64_t>(1)), 63);
  EXPECT_EQ(WasmEdge::clz(static_cast<uint64_t>(0x8000000000000000ULL)), 0);
}


TEST(Int128Test, ArithmeticAddition) {
  // Simple addition without carry
  EXPECT_EQ(WasmEdge::uint128(0, 10U) + WasmEdge::uint128(0, 20U),
            WasmEdge::uint128(0, 30U));
  EXPECT_EQ(WasmEdge::uint128(5U, 10U) + WasmEdge::uint128(2U, 20U),
            WasmEdge::uint128(7U, 30U));

  // Carry propagation from low to high limb
  const uint64_t MaxU64 = std::numeric_limits<uint64_t>::max();
  EXPECT_EQ(WasmEdge::uint128(0, MaxU64) + WasmEdge::uint128(0, 1U),
            WasmEdge::uint128(1U, 0U));
  EXPECT_EQ(WasmEdge::uint128(10U, MaxU64) + WasmEdge::uint128(5U, 10U),
            WasmEdge::uint128(16U, 9U));

  // Overflow past 128-bit wraps around
  EXPECT_EQ(WasmEdge::uint128(MaxU64, MaxU64) + WasmEdge::uint128(0, 1U),
            WasmEdge::uint128(0, 0));

  // Addition assignment operator (+=)
  WasmEdge::uint128 Val(1U, MaxU64);
  Val += WasmEdge::uint128(2U, 5U);
  EXPECT_EQ(Val, WasmEdge::uint128(4U, 4U));
}

TEST(Int128Test, ArithmeticSubtraction) {
  // Simple subtraction without borrow
  EXPECT_EQ(WasmEdge::uint128(0, 50U) - WasmEdge::uint128(0, 20U),
            WasmEdge::uint128(0, 30U));
  EXPECT_EQ(WasmEdge::uint128(10U, 50U) - WasmEdge::uint128(3U, 20U),
            WasmEdge::uint128(7U, 30U));

  // Borrow propagation from high to low limb
  const uint64_t MaxU64 = std::numeric_limits<uint64_t>::max();
  EXPECT_EQ(WasmEdge::uint128(1U, 0U) - WasmEdge::uint128(0, 1U),
            WasmEdge::uint128(0, MaxU64));
  EXPECT_EQ(WasmEdge::uint128(5U, 10U) - WasmEdge::uint128(2U, 20U),
            WasmEdge::uint128(2U, MaxU64 - 9U));

  // Underflow wrapping
  EXPECT_EQ(WasmEdge::uint128(0, 0) - WasmEdge::uint128(0, 1U),
            WasmEdge::uint128(MaxU64, MaxU64));

  // Subtraction assignment operator (-=)
  WasmEdge::uint128 Val(5U, 0U);
  Val -= WasmEdge::uint128(1U, 1U);
  EXPECT_EQ(Val, WasmEdge::uint128(3U, MaxU64));
}

TEST(Int128Test, ArithmeticMultiplication) {
  // Multiplication by zero and one
  WasmEdge::uint128 Val(12345U, 67890U);
  EXPECT_EQ(Val * WasmEdge::uint128(0U), WasmEdge::uint128(0U));
  EXPECT_EQ(Val * WasmEdge::uint128(1U), Val);

  // 64-bit lower product that carries into high limb
  const uint64_t TwoPow32 = 1ULL << 32;
  EXPECT_EQ(WasmEdge::uint128(0, TwoPow32) * WasmEdge::uint128(0, TwoPow32),
            WasmEdge::uint128(1U, 0U));

  // Cross-limb multiplication
  WasmEdge::uint128 A(0, 0x100000000ULL);
  WasmEdge::uint128 B(0, 0x200000000ULL);
  EXPECT_EQ(A * B, WasmEdge::uint128(2U, 0U));

  // Multiplication assignment (*=)
  WasmEdge::uint128 MulVal(0, 15U);
  MulVal *= WasmEdge::uint128(0, 4U);
  EXPECT_EQ(MulVal, WasmEdge::uint128(0, 60U));
}

TEST(Int128Test, DivisionAndModulo) {
  // Division by 64-bit integer
  WasmEdge::uint128 A(0, 100U);
  EXPECT_EQ(A / static_cast<uint64_t>(10U), WasmEdge::uint128(0, 10U));
  EXPECT_EQ(A / static_cast<uint64_t>(200U), WasmEdge::uint128(0, 0));

  // 128-bit value divided by 64-bit divisor
  WasmEdge::uint128 Big(1U, 0U);
  EXPECT_EQ(Big / static_cast<uint64_t>(2U),
            WasmEdge::uint128(0, 0x8000000000000000ULL));
  EXPECT_EQ(Big / static_cast<uint64_t>(4U),
            WasmEdge::uint128(0, 0x4000000000000000ULL));

  // 128-bit division and modulo by 128-bit divisor
  WasmEdge::uint128 X(0x123456789abcdef0ULL, 0xfedcba9876543210ULL);
  WasmEdge::uint128 Divisor(0, 0x100000000ULL);
  WasmEdge::uint128 Quotient = X / Divisor;
  WasmEdge::uint128 Remainder = X % Divisor;
  EXPECT_EQ(Quotient * Divisor + Remainder, X);
  EXPECT_LT(Remainder, Divisor);

  // Self-division
  EXPECT_EQ(X / X, WasmEdge::uint128(1U));
  EXPECT_EQ(X % X, WasmEdge::uint128(0U));

  // Numerator less than denominator
  EXPECT_EQ(WasmEdge::uint128(10U) / WasmEdge::uint128(20U),
            WasmEdge::uint128(0U));
  EXPECT_EQ(WasmEdge::uint128(10U) % WasmEdge::uint128(20U),
            WasmEdge::uint128(10U));

  // Compound assignments (/=, %=)
  WasmEdge::uint128 QVal(0, 100U);
  QVal /= WasmEdge::uint128(0, 7U);
  EXPECT_EQ(QVal, WasmEdge::uint128(0, 14U));

  WasmEdge::uint128 RVal(0, 100U);
  RVal %= WasmEdge::uint128(0, 7U);
  EXPECT_EQ(RVal, WasmEdge::uint128(0, 2U));
}

TEST(Int128Test, BitwiseOperations) {
  const uint64_t F0 = 0xf0f0f0f0f0f0f0f0ULL;
  const uint64_t ZeroF = 0x0f0f0f0f0f0f0f0fULL;
  const uint64_t AllOnes = ~0ULL;

  WasmEdge::uint128 A(F0, ZeroF);
  WasmEdge::uint128 B(ZeroF, F0);

  // AND (&, &=)
  EXPECT_EQ(A & B, WasmEdge::uint128(0, 0));
  WasmEdge::uint128 AndVal = A;
  AndVal &= WasmEdge::uint128(AllOnes, 0);
  EXPECT_EQ(AndVal, WasmEdge::uint128(F0, 0));

  // OR (|, |=)
  EXPECT_EQ(A | B, WasmEdge::uint128(AllOnes, AllOnes));
  WasmEdge::uint128 OrVal = A;
  OrVal |= WasmEdge::uint128(0, F0);
  EXPECT_EQ(OrVal, WasmEdge::uint128(F0, AllOnes));

  // XOR (^, ^=)
  EXPECT_EQ(A ^ A, WasmEdge::uint128(0, 0));
  EXPECT_EQ(A ^ WasmEdge::uint128(0, 0), A);
  EXPECT_EQ(A ^ B, WasmEdge::uint128(AllOnes, AllOnes));
  WasmEdge::uint128 XorVal = A;
  XorVal ^= A;
  EXPECT_EQ(XorVal, WasmEdge::uint128(0, 0));

  // NOT (~)
  EXPECT_EQ(~WasmEdge::uint128(0, 0), WasmEdge::uint128(AllOnes, AllOnes));
  EXPECT_EQ(~WasmEdge::uint128(AllOnes, AllOnes), WasmEdge::uint128(0, 0));
  EXPECT_EQ(~A, WasmEdge::uint128(ZeroF, F0));
}

TEST(Int128Test, ShiftOperations) {
  const uint64_t One = 1ULL;
  const WasmEdge::uint128 Val(0, One);

  // Shift left (<<, <<=)
  EXPECT_EQ(Val << 0U, Val);
  EXPECT_EQ(Val << 1U, WasmEdge::uint128(0, 2ULL));
  EXPECT_EQ(Val << 32U, WasmEdge::uint128(0, 1ULL << 32));
  EXPECT_EQ(Val << 63U, WasmEdge::uint128(0, 1ULL << 63));
  EXPECT_EQ(Val << 64U, WasmEdge::uint128(One, 0));
  EXPECT_EQ(Val << 65U, WasmEdge::uint128(2ULL, 0));
  EXPECT_EQ(Val << 127U, WasmEdge::uint128(1ULL << 63, 0));
  EXPECT_EQ(Val << 128U, WasmEdge::uint128(0, 0));
  EXPECT_EQ(Val << 200U, WasmEdge::uint128(0, 0));

  WasmEdge::uint128 ShiftLVal = Val;
  ShiftLVal <<= 64U;
  EXPECT_EQ(ShiftLVal, WasmEdge::uint128(One, 0));

  // Shift right (>>, >>=)
  const WasmEdge::uint128 HighVal(One, 0);
  EXPECT_EQ(HighVal >> 0U, HighVal);
  EXPECT_EQ(HighVal >> 1U, WasmEdge::uint128(0, 1ULL << 63));
  EXPECT_EQ(HighVal >> 64U, WasmEdge::uint128(0, One));
  EXPECT_EQ(HighVal >> 65U, WasmEdge::uint128(0, 0));
  EXPECT_EQ(HighVal >> 128U, WasmEdge::uint128(0, 0));

  const WasmEdge::uint128 FullVal(1ULL << 63, 0);
  EXPECT_EQ(FullVal >> 127U, WasmEdge::uint128(0, One));

  WasmEdge::uint128 ShiftRVal = HighVal;
  ShiftRVal >>= 64U;
  EXPECT_EQ(ShiftRVal, WasmEdge::uint128(0, One));
}

TEST(Int128Test, ComparisonOperators) {
  WasmEdge::uint128 A(1U, 100U);
  WasmEdge::uint128 B(1U, 200U);
  WasmEdge::uint128 C(2U, 50U);

  // Equality and inequality
  EXPECT_TRUE(A == A);
  EXPECT_FALSE(A == B);
  EXPECT_TRUE(A != B);
  EXPECT_FALSE(A != A);

  // Less than / Greater than with same High limb
  EXPECT_TRUE(A < B);
  EXPECT_TRUE(A <= B);
  EXPECT_FALSE(A > B);
  EXPECT_FALSE(A >= B);

  // Less than / Greater than with different High limb
  EXPECT_TRUE(B < C);
  EXPECT_TRUE(B <= C);
  EXPECT_FALSE(B > C);
  EXPECT_FALSE(B >= C);

  // Reflexive <= and >=
  EXPECT_TRUE(A <= A);
  EXPECT_TRUE(A >= A);
}

TEST(Int128Test, SignedInt128) {
  // Construction from negative int
  WasmEdge::int128 NegOne(-1);
  EXPECT_EQ(NegOne.high(), -1);
  EXPECT_EQ(NegOne.low(), std::numeric_limits<uint64_t>::max());

  WasmEdge::int128 PosTen(10);
  EXPECT_EQ(PosTen.high(), 0);
  EXPECT_EQ(PosTen.low(), 10U);

  // Numeric min and max
  EXPECT_EQ(WasmEdge::int128::numericMin().high(),
            std::numeric_limits<int64_t>::min());
  EXPECT_EQ(WasmEdge::int128::numericMin().low(), 0U);
  EXPECT_EQ(WasmEdge::int128::numericMax().high(),
            std::numeric_limits<int64_t>::max());
  EXPECT_EQ(WasmEdge::int128::numericMax().low(),
            std::numeric_limits<uint64_t>::max());

  // Conversion between int128 and uint128
  WasmEdge::uint128 UVal(NegOne);
  EXPECT_EQ(UVal.high(), std::numeric_limits<uint64_t>::max());
  EXPECT_EQ(UVal.low(), std::numeric_limits<uint64_t>::max());

  WasmEdge::int128 SVal(UVal);
  EXPECT_EQ(SVal.high(), -1);
  EXPECT_EQ(SVal.low(), std::numeric_limits<uint64_t>::max());
}

TEST(Int128Test, Uint128ClzMemberTest) {
  EXPECT_EQ(WasmEdge::uint128(0, 0).clz(), 128U);
  EXPECT_EQ(WasmEdge::uint128(0, 1U).clz(), 127U);
  EXPECT_EQ(WasmEdge::uint128(0, 0x8000000000000000ULL).clz(), 64U);
  EXPECT_EQ(WasmEdge::uint128(1U, 0).clz(), 63U);
  EXPECT_EQ(WasmEdge::uint128(0x8000000000000000ULL, 0).clz(), 0U);
  EXPECT_EQ(WasmEdge::uint128(~0ULL, ~0ULL).clz(), 0U);
}

} // namespace
