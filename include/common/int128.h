// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/common/int128.h - 128-bit integer type -------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the 128-bit integer type.
///
//===----------------------------------------------------------------------===//
#pragma once

#if defined(_MSC_VER) && !defined(__clang__)
#pragma intrinsic(_BitScanReverse)
#pragma intrinsic(_BitScanReverse64)
#include <immintrin.h>
#endif
// We have to detect for those environments who don't support __int128 type
// natively.
#include "endian.h"

#include <cstdint>
#include <limits>
#include <stdexcept>

namespace WasmEdge {

inline constexpr int clz(uint32_t V) noexcept {
#if defined(_MSC_VER) && !defined(__clang__)
  if (V) {
    unsigned long LeadingZero = 0;
    _BitScanReverse(&LeadingZero, V);
    return 31 ^ static_cast<int>(LeadingZero);
  }
  return 32;
#else
  return __builtin_clz(V);
#endif
}

inline constexpr int clz(uint64_t V) noexcept {
#if defined(_MSC_VER) && !defined(__clang__)
  if (V) {
    unsigned long LeadingZero = 0;
    _BitScanReverse64(&LeadingZero, V);
    return 63 ^ static_cast<int>(LeadingZero);
  }
  return 64;
#else
  return __builtin_clzll(V);
#endif
}

inline auto udiv128by64to64(uint64_t U1, uint64_t U0, uint64_t V,
                            uint64_t &R) noexcept {
  {
#if defined(_M_X64) && defined(_MSC_VER) && !defined(__clang__)
    return _udiv128(U1, U0, V, &R);
#elif defined(__x86_64__)
    uint64_t Result = 0;
    __asm__("divq %[v]" : "=a"(Result), "=d"(R) : [v] "r"(V), "a"(U0), "d"(U1));
    return Result;
#endif
  }
  const uint32_t V0 = static_cast<uint32_t>(V);
  const uint32_t V1 = static_cast<uint32_t>(V >> 32);
  if (V1 == 0) {
    auto Rem = (U1 << 32) | (U0 >> 32);
    auto Result = Rem / V0;
    Rem = ((Rem % V0) << 32) | static_cast<uint32_t>(U0);
    Result = (Result << 32) | (Rem / V0);
    R = Rem % V0;
    return Result;
  }
  uint64_t Un64 = 0, Un10 = 0;
  const auto s = clz(V1);
  if (s > 0) {
    V <<= s;
    Un64 = (U1 << s) | (U0 >> (64 - s));
    Un10 = U0 << s;
  } else {
    Un64 = U1;
    Un10 = U0;
  }
  uint64_t Vn1 = static_cast<uint32_t>(V >> 32);
  uint64_t Vn0 = static_cast<uint32_t>(V);
  uint64_t Un1 = static_cast<uint32_t>(Un10 >> 32);
  uint64_t Un0 = static_cast<uint32_t>(Un10);
  uint64_t Q1 = Un64 / Vn1;
  uint64_t Rhat = Un64 - Q1 * Vn1;
  while ((Q1 >> 32) >= 1 || Q1 * Vn0 > (Rhat << 32) + Un1) {
    --Q1;
    Rhat += Vn1;
    if ((Rhat >> 32) >= 1) {
      break;
    }
  }

  uint64_t Un21 = (Un64 << 32) + Un1 - Q1 * V;
  uint64_t Q0 = Un21 / Vn1;
  Rhat = Un21 - Q0 * Vn1;
  while ((Q0 >> 32) >= 1 || Q0 * Vn0 > (Rhat << 32) + Un0) {
    --Q0;
    Rhat += Vn1;
    if ((Rhat >> 32) >= 1) {
      break;
    }
  }
  R = ((Un21 << 32) + Un0 - Q0 * V) >> s;
  return (Q1 << 32) + Q0;
}

class int128;
class uint128;

class uint128 {
public:
  uint128() noexcept = default;
  constexpr uint128(const uint128 &) noexcept = default;
  constexpr uint128(uint128 &&) noexcept = default;
  constexpr uint128 &operator=(const uint128 &V) noexcept = default;
  constexpr uint128 &operator=(uint128 &&V) noexcept = default;

  constexpr uint128(unsigned int V) noexcept : Low(V), High(0) {}
  constexpr uint128(unsigned long V) noexcept : Low(V), High(0) {}
  constexpr uint128(unsigned long long V) noexcept : Low(V), High(0) {}
  constexpr uint128(int128 V) noexcept;
  constexpr uint128(uint64_t H, uint64_t L) noexcept
      : Low(L), High(H){}

#if defined(__x86_64__) || defined(__aarch64__) ||                             \
    (defined(__riscv) && __riscv_xlen == 64) || defined(__s390x__)
        constexpr uint128(unsigned __int128 V) noexcept
      : Low(static_cast<uint64_t>(V)), High(static_cast<uint64_t>(V >> 64)) {
  }
#endif

  constexpr operator bool() const noexcept {
    return static_cast<bool>(Low) || static_cast<bool>(High);
  }
  constexpr operator uint8_t() const noexcept {
    return static_cast<uint8_t>(Low);
  }
  constexpr operator uint16_t() const noexcept {
    return static_cast<uint16_t>(Low);
  }
  constexpr operator uint32_t() const noexcept {
    return static_cast<uint32_t>(Low);
  }
  constexpr operator uint64_t() const noexcept {
    return static_cast<uint64_t>(Low);
  }

  constexpr uint128 &operator=(unsigned int V) noexcept {
    return *this = uint128(V);
  }
  constexpr uint128 &operator=(unsigned long V) noexcept {
    return *this = uint128(V);
  }
  constexpr uint128 &operator=(unsigned long long V) noexcept {
    return *this = uint128(V);
  }
  constexpr uint128 &operator+=(uint128 Other) noexcept {
    return *this = *this + Other;
  }
  constexpr uint128 &operator-=(uint128 Other) noexcept {
    return *this = *this - Other;
  }
  constexpr uint128 &operator*=(uint128 Other) noexcept {
    return *this = *this * Other;
  }
  constexpr uint128 &operator/=(uint128 Other) noexcept {
    return *this = *this / Other;
  }
  constexpr uint128 &operator%=(uint128 Other) noexcept {
    return *this = *this % Other;
  }
  constexpr uint128 &operator&=(uint128 Other) noexcept {
    return *this = *this & Other;
  }
  constexpr uint128 &operator|=(uint128 Other) noexcept {
    return *this = *this | Other;
  }
  constexpr uint128 &operator^=(uint128 Other) noexcept {
    return *this = *this ^ Other;
  }
  constexpr uint128 &operator<<=(unsigned int Other) noexcept {
    return *this = *this << Other;
  }
  constexpr uint128 &operator>>=(unsigned int Other) noexcept {
    return *this = *this >> Other;
  }
  constexpr uint128 &operator<<=(int Other) noexcept {
    return *this = *this << Other;
  }
  constexpr uint128 &operator>>=(int Other) noexcept {
    return *this = *this >> Other;
  }

  constexpr uint128 &operator=(int128 V) noexcept;

  friend constexpr bool operator==(uint128 LHS, uint128 RHS) noexcept {
    return LHS.Low == RHS.Low && LHS.High == RHS.High;
  }
  friend constexpr bool operator<(uint128 LHS, uint128 RHS) noexcept {
    return LHS.High == RHS.High ? LHS.Low < RHS.Low : LHS.High < RHS.High;
  }
  friend constexpr bool operator>(uint128 LHS, uint128 RHS) noexcept {
    return RHS < LHS;
  }
  friend constexpr bool operator!=(uint128 LHS, uint128 RHS) noexcept {
    return !(LHS == RHS);
  }
  friend constexpr bool operator<=(uint128 LHS, uint128 RHS) noexcept {
    return !(LHS > RHS);
  }
  friend constexpr bool operator>=(uint128 LHS, uint128 RHS) noexcept {
    return !(LHS < RHS);
  }

  friend constexpr uint128 operator+(uint128 LHS, uint128 RHS) noexcept {
    uint64_t Carry =
        (std::numeric_limits<uint64_t>::max() - LHS.Low) < RHS.Low ? 1 : 0;
    return uint128(LHS.High + RHS.High + Carry, LHS.Low + RHS.Low);
  }
  friend constexpr uint128 operator-(uint128 LHS, uint128 RHS) noexcept {
    uint64_t Carry = LHS.Low < RHS.Low ? 1 : 0;
    return uint128(LHS.High - RHS.High - Carry, LHS.Low - RHS.Low);
  }
  friend constexpr uint128 operator*(uint128 LHS, uint128 RHS) noexcept {
    uint64_t A32 = LHS.Low >> 32;
    uint64_t A00 = LHS.Low & UINT64_C(0xffffffff);
    uint64_t B32 = RHS.Low >> 32;
    uint64_t B00 = RHS.Low & UINT64_C(0xffffffff);
    uint128 Result =
        uint128(LHS.High * RHS.Low + LHS.Low * RHS.High + A32 * B32, A00 * B00);
    Result += uint128(A32 * B00) << 32U;
    Result += uint128(A00 * B32) << 32U;
    return Result;
  }
  friend uint128 operator/(uint128 LHS, uint64_t RHS) noexcept {
    if (LHS.High == 0 && LHS.Low < RHS) {
      LHS.Low = 0;
      return LHS;
    }
    if (LHS.High < RHS) {
      uint64_t Rem = 0;
      uint64_t QLow = udiv128by64to64(LHS.High, LHS.Low, RHS, Rem);
      LHS.Low = QLow;
      LHS.High = 0;
      return LHS;
    }
    uint64_t QHigh = LHS.High / RHS;
    uint64_t Rem = 0;
    uint64_t QLow = udiv128by64to64(LHS.High % RHS, LHS.Low, RHS, Rem);
    LHS.Low = QLow;
    LHS.High = QHigh;
    return LHS;
  }
  friend constexpr uint128 operator/(uint128 LHS, uint128 RHS) noexcept {
    if (RHS > LHS) {
      return 0U;
    }
    if (RHS == LHS) {
      return 1U;
    }
    if (RHS.High == 0) {
      return LHS / RHS.Low;
    }
    uint128 Denominator = RHS;
    uint128 Quotient = 0U;
    const unsigned int Shift = RHS.clz() - LHS.clz();
    Denominator <<= Shift;
    for (unsigned int I = 0U; I <= Shift; ++I) {
      Quotient <<= 1U;
      if (LHS >= Denominator) {
        LHS -= Denominator;
        Quotient |= 1U;
      }
      Denominator >>= 1U;
    }
    return Quotient;
  }
  friend constexpr uint128 operator%(uint128 LHS, uint128 RHS) noexcept {
    if (RHS > LHS) {
      return LHS;
    }
    if (RHS == LHS) {
      return 0U;
    }
    uint128 Denominator = RHS;
    const unsigned int Shift = RHS.clz() - LHS.clz();
    Denominator <<= Shift;
    for (unsigned int I = 0; I <= Shift; ++I) {
      if (LHS >= Denominator) {
        LHS -= Denominator;
      }
      Denominator >>= 1U;
    }
    return LHS;
  }
  friend constexpr uint128 operator&(uint128 LHS, uint128 RHS) noexcept {
    return uint128(LHS.High & RHS.High, LHS.Low & RHS.Low);
  }
  friend constexpr uint128 operator|(uint128 LHS, uint128 RHS) noexcept {
    return uint128(LHS.High | RHS.High, LHS.Low | RHS.Low);
  }
  friend constexpr uint128 operator^(uint128 LHS, uint128 RHS) noexcept {
    return uint128(LHS.High ^ RHS.High, LHS.Low ^ RHS.Low);
  }
  friend constexpr uint128 operator~(uint128 Value) noexcept {
    return uint128(~Value.High, ~Value.Low);
  }
  friend constexpr uint128 operator<<(uint128 Value,
                                      unsigned int Shift) noexcept {
    if (Shift < 64) {
      if (Shift != 0) {
        return uint128((Value.High << Shift) | (Value.Low >> (64 - Shift)),
                       Value.Low << Shift);
      }
      return Value;
    }
    return uint128(Value.Low << (Shift - 64), 0);
  }
  friend constexpr uint128 operator>>(uint128 Value,
                                      unsigned int Shift) noexcept {
    if (Shift < 64) {
      if (Shift != 0) {
        return uint128((Value.High >> Shift),
                       Value.Low >> Shift | (Value.High << (64 - Shift)));
      }
      return Value;
    }
    return uint128(0, Value.High >> (Shift - 64));
  }
  friend constexpr uint128 operator<<(uint128 Value, int Shift) noexcept {
    return Value << static_cast<unsigned int>(Shift);
  }
  friend constexpr uint128 operator>>(uint128 Value, int Shift) noexcept {
    return Value >> static_cast<unsigned int>(Shift);
  }
  friend constexpr uint128 operator<<(uint128 Value,
                                      unsigned long long Shift) noexcept {
    return Value << static_cast<unsigned int>(Shift);
  }
  friend constexpr uint128 operator>>(uint128 Value,
                                      unsigned long long Shift) noexcept {
    return Value >> static_cast<unsigned int>(Shift);
  }

  static constexpr uint128 numericMin() noexcept {
    return uint128(std::numeric_limits<uint64_t>::min(),
                   std::numeric_limits<uint64_t>::min());
  }
  static constexpr uint128 numericMax() noexcept {
    return uint128(std::numeric_limits<uint64_t>::max(),
                   std::numeric_limits<uint64_t>::max());
  }

  constexpr uint64_t low() const noexcept { return Low; }
  constexpr uint64_t high() const noexcept { return High; }
  constexpr unsigned int clz() const noexcept {
    if (High) {
      return static_cast<unsigned int>(WasmEdge::clz(High));
    }
    if (Low) {
      return static_cast<unsigned int>(WasmEdge::clz(Low)) + 64U;
    }
    return 128U;
  }

private:
  uint64_t Low;
  uint64_t High;
};

class int128 {
public:
  int128() noexcept = default;
  constexpr int128(const int128 &) noexcept = default;
  constexpr int128(int128 &&) noexcept = default;
  constexpr int128 &operator=(const int128 &V) noexcept = default;
  constexpr int128 &operator=(int128 &&V) noexcept = default;

  constexpr int128(int V) noexcept
      : Low(static_cast<uint64_t>(V)), High(V < 0 ? INT64_C(-1) : INT64_C(0)) {}
  constexpr int128(long V) noexcept
      : Low(static_cast<uint64_t>(V)), High(V < 0 ? INT64_C(-1) : INT64_C(0)) {}
  constexpr int128(long long V) noexcept
      : Low(static_cast<uint64_t>(V)), High(V < 0 ? INT64_C(-1) : INT64_C(0)) {}
  constexpr int128(unsigned int V) noexcept : Low(V), High(INT64_C(0)) {}
  constexpr int128(unsigned long V) noexcept : Low(V), High(INT64_C(0)) {}
  constexpr int128(unsigned long long V) noexcept : Low(V), High(INT64_C(0)) {}
  constexpr int128(uint128 V) noexcept;
  constexpr int128(int64_t H, uint64_t L) noexcept
      : Low(L), High(H){}
#if defined(__x86_64__) || defined(__aarch64__) ||                             \
    (defined(__riscv) && __riscv_xlen == 64) || defined(__s390x__)
        constexpr int128(__int128 V) noexcept
      : Low(static_cast<uint64_t>(V)), High(V >> 64) {
  }
#endif

  constexpr int128 &operator=(int V) noexcept { return *this = int128(V); }
  constexpr int128 &operator=(long V) noexcept { return *this = int128(V); }
  constexpr int128 &operator=(long long V) noexcept {
    return *this = int128(V);
  }
  constexpr int128 &operator=(unsigned int V) noexcept {
    return *this = int128(V);
  }
  constexpr int128 &operator=(unsigned long V) noexcept {
    return *this = int128(V);
  }
  constexpr int128 &operator=(unsigned long long V) noexcept {
    return *this = int128(V);
  }

  constexpr int128 &operator=(uint128 V) noexcept;

  static constexpr int128 numericMin() noexcept {
    return int128(std::numeric_limits<int64_t>::min(), 0);
  }
  static constexpr int128 numericMax() noexcept {
    return int128(std::numeric_limits<int64_t>::max(),
                  std::numeric_limits<uint64_t>::max());
  }

  constexpr uint64_t low() const noexcept { return Low; }
  constexpr int64_t high() const noexcept { return High; }

private:
  uint64_t Low;
  int64_t High;
};

inline constexpr uint128::uint128(int128 V) noexcept
    : Low(V.low()), High(static_cast<uint64_t>(V.high())) {}

inline constexpr uint128 &uint128::operator=(int128 V) noexcept {
  return *this = uint128(V);
}

inline constexpr int128::int128(uint128 V) noexcept
    : Low(V.low()), High(static_cast<int64_t>(V.high())) {}

inline constexpr int128 &int128::operator=(uint128 V) noexcept {
  return *this = int128(V);
}

} // namespace WasmEdge

namespace std {
template <> class numeric_limits<WasmEdge::uint128> {
public:
  static constexpr bool is_specialized = true;
  static constexpr bool is_signed = false;
  static constexpr bool is_integer = true;
  static constexpr bool is_exact = true;
  static constexpr bool has_infinity = false;
  static constexpr bool has_quiet_NaN = false;
  static constexpr bool has_signaling_NaN = false;
  static constexpr float_denorm_style has_denorm = denorm_absent;
  static constexpr bool has_denorm_loss = false;
  static constexpr float_round_style round_style = round_toward_zero;
  static constexpr bool is_iec559 = false;
  static constexpr bool is_bounded = true;
  static constexpr bool is_modulo = false;
  static constexpr int digits = 127;
  static constexpr int digits10 = 38;
  static constexpr int max_digits10 = 0;
  static constexpr int radix = 2;
  static constexpr int min_exponent = 0;
  static constexpr int min_exponent10 = 0;
  static constexpr int max_exponent = 0;
  static constexpr int max_exponent10 = 0;
  static constexpr bool traps = numeric_limits<uint64_t>::traps;
  static constexpr bool tinyness_before = false;

  static constexpr WasmEdge::uint128 min() {
    return WasmEdge::uint128::numericMin();
  }
  static constexpr WasmEdge::uint128 lowest() {
    return WasmEdge::uint128::numericMin();
  }
  static constexpr WasmEdge::uint128 max() {
    return WasmEdge::uint128::numericMax();
  }
  static constexpr WasmEdge::uint128 epsilon() { return 0U; }
  static constexpr WasmEdge::uint128 round_error() { return 0U; }
  static constexpr WasmEdge::uint128 infinity() { return 0U; }
  static constexpr WasmEdge::uint128 quiet_NaN() { return 0U; }
  static constexpr WasmEdge::uint128 signaling_NaN() { return 0U; }
  static constexpr WasmEdge::uint128 denorm_min() { return 0U; }
};
template <> class numeric_limits<WasmEdge::int128> {
public:
  static constexpr bool is_specialized = true;
  static constexpr bool is_signed = true;
  static constexpr bool is_integer = true;
  static constexpr bool is_exact = true;
  static constexpr bool has_infinity = false;
  static constexpr bool has_quiet_NaN = false;
  static constexpr bool has_signaling_NaN = false;
  static constexpr float_denorm_style has_denorm = denorm_absent;
  static constexpr bool has_denorm_loss = false;
  static constexpr float_round_style round_style = round_toward_zero;
  static constexpr bool is_iec559 = false;
  static constexpr bool is_bounded = true;
  static constexpr bool is_modulo = false;
  static constexpr int digits = 127;
  static constexpr int digits10 = 38;
  static constexpr int max_digits10 = 0;
  static constexpr int radix = 2;
  static constexpr int min_exponent = 0;
  static constexpr int min_exponent10 = 0;
  static constexpr int max_exponent = 0;
  static constexpr int max_exponent10 = 0;
  static constexpr bool traps = numeric_limits<uint64_t>::traps;
  static constexpr bool tinyness_before = false;

  static constexpr WasmEdge::int128 min() {
    return WasmEdge::int128::numericMin();
  }
  static constexpr WasmEdge::int128 lowest() {
    return WasmEdge::int128::numericMin();
  }
  static constexpr WasmEdge::int128 max() {
    return WasmEdge::int128::numericMax();
  }
  static constexpr WasmEdge::int128 epsilon() { return 0; }
  static constexpr WasmEdge::int128 round_error() { return 0; }
  static constexpr WasmEdge::int128 infinity() { return 0; }
  static constexpr WasmEdge::int128 quiet_NaN() { return 0; }
  static constexpr WasmEdge::int128 signaling_NaN() { return 0; }
  static constexpr WasmEdge::int128 denorm_min() { return 0; }
};
} // namespace std

#include <type_traits>
namespace std {
template <> struct is_class<WasmEdge::uint128> : std::true_type {};
} // namespace std

namespace WasmEdge {
// If there is a built-in type __int128, then use it directly
#if defined(__x86_64__) || defined(__aarch64__) ||                             \
    (defined(__riscv) && __riscv_xlen == 64) || defined(__s390x__)
using int128_t = __int128;
using uint128_t = unsigned __int128;
#else
using int128_t = int128;
using uint128_t = uint128;
#endif
} // namespace WasmEdge

#include <fmt/format.h>

FMT_BEGIN_NAMESPACE
namespace detail {
inline constexpr bool operator>=(detail::uint128_fallback LHS,
                                 unsigned int RHS) {
  return LHS.high() != 0 || LHS.low() >= static_cast<uint64_t>(RHS);
}

inline constexpr bool operator<(detail::uint128_fallback LHS,
                                unsigned int RHS) {
  return LHS.high() == 0 && LHS.low() < static_cast<uint64_t>(RHS);
}

inline constexpr bool operator<(detail::uint128_fallback LHS,
                                detail::uint128_fallback RHS) {
  return LHS.high() < RHS.high() ||
         (LHS.high() == RHS.high() && LHS.low() < RHS.low());
}

inline constexpr detail::uint128_fallback
operator+(detail::uint128_fallback LHS, unsigned int RHS) {
  uint64_t NewLow = LHS.low() + RHS;
  uint64_t NewHigh = LHS.high() + (NewLow < LHS.low());
  return {NewHigh, NewLow};
}

inline constexpr detail::uint128_fallback
operator-(unsigned int LHS, detail::uint128_fallback RHS) {
  uint128_fallback Result = RHS;
  return (~Result) + 1 + LHS;
}

inline constexpr detail::uint128_fallback &
operator/=(detail::uint128_fallback &LHS, unsigned int RHSi) {
  const uint64_t RHS = static_cast<uint64_t>(RHSi);
  if (LHS.high() == 0 && LHS.low() < RHS) {
    LHS = 0;
    return LHS;
  }
  if (LHS.high() < RHS) {
    uint64_t Rem = 0;
    uint64_t QLo = WasmEdge::udiv128by64to64(LHS.high(), LHS.low(), RHS, Rem);
    LHS = QLo;
    return LHS;
  }
  uint64_t QHi = LHS.high() / RHS;
  uint64_t Rem = 0;
  uint64_t QLo =
      WasmEdge::udiv128by64to64(LHS.high() % RHS, LHS.low(), RHS, Rem);
  LHS = (detail::uint128_t{QHi} << 64u) | QLo;
  return LHS;
}

inline constexpr detail::uint128_fallback
operator%(detail::uint128_fallback LHS, unsigned int RHSi) {
  const uint64_t RHS = static_cast<uint64_t>(RHSi);
  if (LHS.high() == 0 && LHS.low() < RHS) {
    return LHS;
  }
  uint64_t Rem = 0;
  WasmEdge::udiv128by64to64(LHS.high() % RHS, LHS.low(), RHS, Rem);
  return Rem;
}

inline int do_count_digits(detail::uint128_fallback N) {
  const uint64_t Low = static_cast<uint64_t>(N);
  const uint64_t High = static_cast<uint64_t>(N >> 64);
  if (High == 0) {
    return detail::count_digits(Low);
  }
  // Maps bsr(n) to ceil(log10(pow(2, bsr(n) + 1) - 1)).
  static constexpr uint8_t Bsr2Log10[] = {
      20, 20, 21, 21, 21, 22, 22, 22, 22, 23, 23, 23, 24, 24, 24, 25,
      25, 25, 25, 26, 26, 26, 27, 27, 27, 28, 28, 28, 28, 29, 29, 29,
      30, 30, 30, 31, 31, 31, 32, 32, 32, 32, 33, 33, 33, 34, 34, 34,
      35, 35, 35, 35, 36, 36, 36, 37, 37, 37, 38, 38, 38, 38, 39, 39};
  auto C = WasmEdge::clz(High);
  auto T = Bsr2Log10[C ^ 63];
  static constexpr const uint128_t PowersOf10[] = {
      0,
      (uint128_t(5ULL) << 64) | uint128_t(7766279631452241920ULL),
      (uint128_t(54ULL) << 64) | uint128_t(3875820019684212736ULL),
      (uint128_t(542ULL) << 64) | uint128_t(1864712049423024128ULL),
      (uint128_t(5421ULL) << 64) | uint128_t(200376420520689664ULL),
      (uint128_t(54210ULL) << 64) | uint128_t(2003764205206896640ULL),
      (uint128_t(542101ULL) << 64) | uint128_t(1590897978359414784ULL),
      (uint128_t(5421010ULL) << 64) | uint128_t(15908979783594147840ULL),
      (uint128_t(54210108ULL) << 64) | uint128_t(11515845246265065472ULL),
      (uint128_t(542101086ULL) << 64) | uint128_t(4477988020393345024ULL),
      (uint128_t(5421010862ULL) << 64) | uint128_t(7886392056514347008ULL),
      (uint128_t(54210108624ULL) << 64) | uint128_t(5076944270305263616ULL),
      (uint128_t(542101086242ULL) << 64) | uint128_t(13875954555633532928ULL),
      (uint128_t(5421010862427ULL) << 64) | uint128_t(9632337040368467968ULL),
      (uint128_t(54210108624275ULL) << 64) | uint128_t(4089650035136921600ULL),
      (uint128_t(542101086242752ULL) << 64) | uint128_t(4003012203950112768ULL),
      (uint128_t(5421010862427522ULL) << 64) |
          uint128_t(3136633892082024448ULL),
      (uint128_t(54210108624275221ULL) << 64) |
          uint128_t(12919594847110692864ULL),
      (uint128_t(542101086242752217ULL) << 64) |
          uint128_t(68739955140067328ULL),
      (uint128_t(5421010862427522170ULL) << 64) |
          uint128_t(687399551400673280ULL),
  };
  return T - (N < PowersOf10[T - 20]);
}

FMT_CONSTEXPR20 inline int count_digits(detail::uint128_fallback N) {
  if (!is_constant_evaluated()) {
    return do_count_digits(N);
  }
  return count_digits_fallback(N);
}

} // namespace detail

template <typename Char> struct formatter<WasmEdge::uint128, Char> {
private:
  detail::dynamic_format_specs<Char> Specs;

public:
  template <typename ParseContext> constexpr auto parse(ParseContext &Ctx) {
#if FMT_VERSION >= 100000
    return parse_format_specs(Ctx.begin(), Ctx.end(), Specs, Ctx,
                              detail::type::uint_type);
#else
    using HandlerType = detail::dynamic_specs_handler<ParseContext>;
    detail::specs_checker<HandlerType> Handler(HandlerType(Specs, Ctx),
                                               detail::type::uint_type);
    return parse_format_specs(Ctx.begin(), Ctx.end(), Handler);
#endif
  }

  template <typename FormatContext>
  auto format(WasmEdge::uint128 V, FormatContext &Ctx) const {
    auto Out = Ctx.out();
    auto S = Specs;
#if FMT_VERSION >= 110100
    detail::handle_dynamic_spec(S.dynamic_width(), S.width, S.width_ref, Ctx);
    detail::handle_dynamic_spec(S.dynamic_precision(), S.precision,
                                S.precision_ref, Ctx);
#else
    detail::handle_dynamic_spec<detail::width_checker>(S.width, S.width_ref,
                                                       Ctx);
    detail::handle_dynamic_spec<detail::precision_checker>(
        S.precision, S.precision_ref, Ctx);
#endif

    const detail::uint128_t U =
        (detail::uint128_t{static_cast<uint64_t>(V >> 64)} << 64) |
        detail::uint128_t{static_cast<uint64_t>(V)};
#if FMT_VERSION >= 110100
    return detail::write_int<Char>(Out, detail::make_write_int_arg(U, S.sign()),
                                   S);
#else
    return detail::write_int<Char>(Out, detail::make_write_int_arg(U, S.sign),
                                   S, Ctx.locale());
#endif
  }
};
FMT_END_NAMESPACE
