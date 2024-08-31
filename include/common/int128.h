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

// If there is a built-in type __int128, then use it directly
#if defined(__x86_64__) || defined(__aarch64__) ||                             \
    (defined(__riscv) && __riscv_xlen == 64)
namespace WasmEdge {
using int128_t = __int128;
using uint128_t = unsigned __int128;
} // namespace WasmEdge
#else

#if defined(_MSC_VER) && !defined(__clang__)
#pragma intrinsic(_BitScanReverse64)
#endif
// We have to detect for those environments who don't support __int128 type
// natively.
#include "endian.h"

#include <cstdint>
#include <limits>
#include <stdexcept>

// Currently, only byte-swapped little endian is handled.
#if !WASMEDGE_ENDIAN_LITTLE_BYTE
#error unsupported endian!
#endif

namespace WasmEdge {

class int128_t;
class uint128_t;

class uint128_t {
public:
  uint128_t() noexcept = default;
  constexpr uint128_t(const uint128_t &) noexcept = default;
  constexpr uint128_t(uint128_t &&) noexcept = default;
  constexpr uint128_t &operator=(const uint128_t &V) noexcept = default;
  constexpr uint128_t &operator=(uint128_t &&V) noexcept = default;

  constexpr uint128_t(int V) noexcept
      : Low(V), High(V < 0 ? std::numeric_limits<uint64_t>::max() : 0) {}
  constexpr uint128_t(long V) noexcept
      : Low(V), High(V < 0 ? std::numeric_limits<uint64_t>::max() : 0) {}
  constexpr uint128_t(long long V) noexcept
      : Low(V), High(V < 0 ? std::numeric_limits<uint64_t>::max() : 0) {}
  constexpr uint128_t(unsigned int V) noexcept : Low(V), High(0) {}
  constexpr uint128_t(unsigned long V) noexcept : Low(V), High(0) {}
  constexpr uint128_t(unsigned long long V) noexcept : Low(V), High(0) {}
  constexpr uint128_t(int128_t V) noexcept;
  constexpr uint128_t(uint64_t H, uint64_t L) noexcept : Low(L), High(H) {}

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

  constexpr uint128_t &operator=(int V) noexcept {
    return *this = uint128_t(V);
  }
  constexpr uint128_t &operator=(long V) noexcept {
    return *this = uint128_t(V);
  }
  constexpr uint128_t &operator=(long long V) noexcept {
    return *this = uint128_t(V);
  }
  constexpr uint128_t &operator=(unsigned int V) noexcept {
    return *this = uint128_t(V);
  }
  constexpr uint128_t &operator=(unsigned long V) noexcept {
    return *this = uint128_t(V);
  }
  constexpr uint128_t &operator=(unsigned long long V) noexcept {
    return *this = uint128_t(V);
  }
  constexpr uint128_t &operator+=(uint128_t Other) noexcept {
    return *this = *this + Other;
  }
  constexpr uint128_t &operator-=(uint128_t Other) noexcept {
    return *this = *this - Other;
  }
  constexpr uint128_t &operator*=(uint128_t Other) noexcept {
    return *this = *this * Other;
  }
  constexpr uint128_t &operator/=(uint128_t Other) noexcept {
    return *this = *this / Other;
  }
  constexpr uint128_t &operator%=(uint128_t Other) noexcept {
    return *this = *this % Other;
  }
  constexpr uint128_t &operator&=(uint128_t Other) noexcept {
    return *this = *this & Other;
  }
  constexpr uint128_t &operator|=(uint128_t Other) noexcept {
    return *this = *this | Other;
  }
  constexpr uint128_t &operator^=(uint128_t Other) noexcept {
    return *this = *this ^ Other;
  }
  constexpr uint128_t &operator<<=(unsigned int Other) noexcept {
    return *this = *this << Other;
  }
  constexpr uint128_t &operator>>=(unsigned int Other) noexcept {
    return *this = *this >> Other;
  }
  constexpr uint128_t &operator<<=(int Other) noexcept {
    return *this = *this << Other;
  }
  constexpr uint128_t &operator>>=(int Other) noexcept {
    return *this = *this >> Other;
  }

  constexpr uint128_t &operator=(int128_t V) noexcept;

  friend constexpr bool operator==(uint128_t LHS, uint128_t RHS) noexcept {
    return LHS.Low == RHS.Low && LHS.High == RHS.High;
  }
  friend constexpr bool operator<(uint128_t LHS, uint128_t RHS) noexcept {
    return LHS.High == RHS.High ? LHS.Low < RHS.Low : LHS.High < RHS.High;
  }
  friend constexpr bool operator>(uint128_t LHS, uint128_t RHS) noexcept {
    return RHS < LHS;
  }
  friend constexpr bool operator!=(uint128_t LHS, uint128_t RHS) noexcept {
    return !(LHS == RHS);
  }
  friend constexpr bool operator<=(uint128_t LHS, uint128_t RHS) noexcept {
    return !(LHS > RHS);
  }
  friend constexpr bool operator>=(uint128_t LHS, uint128_t RHS) noexcept {
    return !(LHS < RHS);
  }

  friend constexpr uint128_t operator+(uint128_t LHS, uint128_t RHS) noexcept {
    uint64_t Carry =
        (std::numeric_limits<uint64_t>::max() - LHS.Low) < RHS.Low ? 1 : 0;
    return uint128_t(LHS.High + RHS.High + Carry, LHS.Low + RHS.Low);
  }
  friend constexpr uint128_t operator-(uint128_t LHS, uint128_t RHS) noexcept {
    uint64_t Carry = LHS.Low < RHS.Low ? 1 : 0;
    return uint128_t(LHS.High - RHS.High - Carry, LHS.Low - RHS.Low);
  }
  friend constexpr uint128_t operator*(uint128_t LHS, uint128_t RHS) noexcept {
    uint64_t A32 = LHS.Low >> 32;
    uint64_t A00 = LHS.Low & UINT64_C(0xffffffff);
    uint64_t B32 = RHS.Low >> 32;
    uint64_t B00 = RHS.Low & UINT64_C(0xffffffff);
    uint128_t Result = uint128_t(
        LHS.High * RHS.Low + LHS.Low * RHS.High + A32 * B32, A00 * B00);
    Result += uint128_t(A32 * B00) << 32U;
    Result += uint128_t(A00 * B32) << 32U;
    return Result;
  }
  friend constexpr uint128_t operator/(uint128_t LHS, uint128_t RHS) noexcept {
    if (RHS > LHS) {
      return 0;
    }
    if (RHS == LHS) {
      return 1;
    }
    uint128_t Denominator = RHS;
    uint128_t Quotient = 0;
    const unsigned int Shift = RHS.clz() - LHS.clz();
    Denominator <<= Shift;
    for (unsigned int I = 0; I <= Shift; ++I) {
      Quotient <<= 1U;
      if (LHS >= Denominator) {
        LHS -= Denominator;
        Quotient |= 1U;
      }
      Denominator >>= 1U;
    }
    return Quotient;
  }
  friend constexpr uint128_t operator%(uint128_t LHS, uint128_t RHS) noexcept {
    if (RHS > LHS) {
      return LHS;
    }
    if (RHS == LHS) {
      return 0;
    }
    uint128_t Denominator = RHS;
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
  friend constexpr uint128_t operator&(uint128_t LHS, uint128_t RHS) noexcept {
    return uint128_t(LHS.High & RHS.High, LHS.Low & RHS.Low);
  }
  friend constexpr uint128_t operator|(uint128_t LHS, uint128_t RHS) noexcept {
    return uint128_t(LHS.High | RHS.High, LHS.Low | RHS.Low);
  }
  friend constexpr uint128_t operator^(uint128_t LHS, uint128_t RHS) noexcept {
    return uint128_t(LHS.High ^ RHS.High, LHS.Low ^ RHS.Low);
  }
  friend constexpr uint128_t operator~(uint128_t Value) noexcept {
    return uint128_t(~Value.High, ~Value.Low);
  }
  friend constexpr uint128_t operator<<(uint128_t Value,
                                        unsigned int Shift) noexcept {
    if (Shift < 64) {
      if (Shift != 0) {
        return uint128_t((Value.High << Shift) | (Value.Low >> (64 - Shift)),
                         Value.Low << Shift);
      }
      return Value;
    }
    return uint128_t(Value.Low << (Shift - 64), 0);
  }
  friend constexpr uint128_t operator>>(uint128_t Value,
                                        unsigned int Shift) noexcept {
    if (Shift < 64) {
      if (Shift != 0) {
        return uint128_t((Value.High >> Shift),
                         Value.Low >> Shift | (Value.High << (64 - Shift)));
      }
      return Value;
    }
    return uint128_t(0, Value.High >> (Shift - 64));
  }
  friend constexpr uint128_t operator<<(uint128_t Value, int Shift) noexcept {
    return Value << static_cast<unsigned int>(Shift);
  }
  friend constexpr uint128_t operator>>(uint128_t Value, int Shift) noexcept {
    return Value >> static_cast<unsigned int>(Shift);
  }
  friend constexpr uint128_t operator<<(uint128_t Value,
                                        unsigned long long Shift) noexcept {
    return Value << static_cast<unsigned int>(Shift);
  }
  friend constexpr uint128_t operator>>(uint128_t Value,
                                        unsigned long long Shift) noexcept {
    return Value >> static_cast<unsigned int>(Shift);
  }

  static constexpr uint128_t numericMin() noexcept {
    return uint128_t(std::numeric_limits<uint64_t>::min(),
                     std::numeric_limits<uint64_t>::min());
  }
  static constexpr uint128_t numericMax() noexcept {
    return uint128_t(std::numeric_limits<uint64_t>::max(),
                     std::numeric_limits<uint64_t>::max());
  }

  constexpr uint64_t low() const noexcept { return Low; }
  constexpr uint64_t high() const noexcept { return High; }
  constexpr unsigned int clz() const noexcept {
#if defined(_MSC_VER) && !defined(__clang__)
    unsigned long LeadingZero = 0;
    if (High) {
      _BitScanReverse64(&LeadingZero, High);
      return (63 - LeadingZero);
    }
    if (Low) {
      _BitScanReverse64(&LeadingZero, Low);
      return (63 - LeadingZero) + 64;
    }
    return 128;
#else
    if (High) {
      return __builtin_clzll(High);
    }
    if (Low) {
      return __builtin_clzll(Low) + 64;
    }
    return 128;
#endif
  }

private:
  uint64_t Low;
  uint64_t High;
};

class int128_t {
public:
  int128_t() noexcept = default;
  constexpr int128_t(const int128_t &) noexcept = default;
  constexpr int128_t(int128_t &&) noexcept = default;
  constexpr int128_t &operator=(const int128_t &V) noexcept = default;
  constexpr int128_t &operator=(int128_t &&V) noexcept = default;

  constexpr int128_t(int V) noexcept
      : Low(static_cast<uint64_t>(V)), High(V < 0 ? INT64_C(-1) : INT64_C(0)) {}
  constexpr int128_t(long V) noexcept
      : Low(static_cast<uint64_t>(V)), High(V < 0 ? INT64_C(-1) : INT64_C(0)) {}
  constexpr int128_t(long long V) noexcept
      : Low(static_cast<uint64_t>(V)), High(V < 0 ? INT64_C(-1) : INT64_C(0)) {}
  constexpr int128_t(unsigned int V) noexcept : Low(V), High(INT64_C(0)) {}
  constexpr int128_t(unsigned long V) noexcept : Low(V), High(INT64_C(0)) {}
  constexpr int128_t(unsigned long long V) noexcept
      : Low(V), High(INT64_C(0)) {}
  constexpr int128_t(uint128_t V) noexcept;
  constexpr int128_t(int64_t H, uint64_t L) noexcept : Low(L), High(H) {}

  constexpr int128_t &operator=(int V) noexcept { return *this = int128_t(V); }
  constexpr int128_t &operator=(long V) noexcept { return *this = int128_t(V); }
  constexpr int128_t &operator=(long long V) noexcept {
    return *this = int128_t(V);
  }
  constexpr int128_t &operator=(unsigned int V) noexcept {
    return *this = int128_t(V);
  }
  constexpr int128_t &operator=(unsigned long V) noexcept {
    return *this = int128_t(V);
  }
  constexpr int128_t &operator=(unsigned long long V) noexcept {
    return *this = int128_t(V);
  }

  constexpr int128_t &operator=(uint128_t V) noexcept;

  static constexpr int128_t numericMin() noexcept {
    return int128_t(std::numeric_limits<int64_t>::min(), 0);
  }
  static constexpr int128_t numericMax() noexcept {
    return int128_t(std::numeric_limits<int64_t>::max(),
                    std::numeric_limits<uint64_t>::max());
  }

  constexpr uint64_t low() const noexcept { return Low; }
  constexpr int64_t high() const noexcept { return High; }

private:
  uint64_t Low;
  int64_t High;
};

inline constexpr uint128_t::uint128_t(int128_t V) noexcept
    : Low(V.low()), High(static_cast<uint64_t>(V.high())) {}

inline constexpr uint128_t &uint128_t::operator=(int128_t V) noexcept {
  return *this = uint128_t(V);
}

inline constexpr int128_t::int128_t(uint128_t V) noexcept
    : Low(V.low()), High(static_cast<int64_t>(V.high())) {}

inline constexpr int128_t &int128_t::operator=(uint128_t V) noexcept {
  return *this = int128_t(V);
}

} // namespace WasmEdge

namespace std {
template <> class numeric_limits<WasmEdge::uint128_t> {
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

  static constexpr WasmEdge::uint128_t min() {
    return WasmEdge::uint128_t::numericMin();
  }
  static constexpr WasmEdge::uint128_t lowest() {
    return WasmEdge::uint128_t::numericMin();
  }
  static constexpr WasmEdge::uint128_t max() {
    return WasmEdge::uint128_t::numericMax();
  }
  static constexpr WasmEdge::uint128_t epsilon() { return 0; }
  static constexpr WasmEdge::uint128_t round_error() { return 0; }
  static constexpr WasmEdge::uint128_t infinity() { return 0; }
  static constexpr WasmEdge::uint128_t quiet_NaN() { return 0; }
  static constexpr WasmEdge::uint128_t signaling_NaN() { return 0; }
  static constexpr WasmEdge::uint128_t denorm_min() { return 0; }
};
template <> class numeric_limits<WasmEdge::int128_t> {
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

  static constexpr WasmEdge::int128_t min() {
    return WasmEdge::int128_t::numericMin();
  }
  static constexpr WasmEdge::int128_t lowest() {
    return WasmEdge::int128_t::numericMin();
  }
  static constexpr WasmEdge::int128_t max() {
    return WasmEdge::int128_t::numericMax();
  }
  static constexpr WasmEdge::int128_t epsilon() { return 0; }
  static constexpr WasmEdge::int128_t round_error() { return 0; }
  static constexpr WasmEdge::int128_t infinity() { return 0; }
  static constexpr WasmEdge::int128_t quiet_NaN() { return 0; }
  static constexpr WasmEdge::int128_t signaling_NaN() { return 0; }
  static constexpr WasmEdge::int128_t denorm_min() { return 0; }
};
} // namespace std

#endif

#include <type_traits>
namespace std {
template <> struct is_class<WasmEdge::uint128_t> : std::true_type {};
} // namespace std

#include <fmt/format.h>

#if !FMT_USE_INT128

FMT_BEGIN_NAMESPACE
template <typename Char> struct formatter<WasmEdge::uint128_t, Char> {
private:
  detail::dynamic_format_specs<> Specs;

public:
  template <typename ParseContext> constexpr auto parse(ParseContext &Ctx) {
    return parse_format_specs(Ctx.begin(), Ctx.end(), Specs, Ctx,
                              detail::type::uint_type);
  }

  template <typename FormatContext>
  auto format(WasmEdge::uint128_t V, FormatContext &Ctx) const {
    auto S = Specs;
    detail::handle_dynamic_spec<detail::width_checker>(S.width, S.width_ref,
                                                       Ctx);
    detail::handle_dynamic_spec<detail::precision_checker>(
        S.precision, S.precision_ref, Ctx);
    constexpr const unsigned Prefixes[4] = {0, 0, 0x1000000u | '+',
                                            0x1000000u | ' '};
    const detail::uint128_t U{static_cast<uint64_t>(V >> 64),
                              static_cast<uint64_t>(V)};
    return detail::write_int<Char>(
        Ctx.out(),
        detail::write_int_arg<detail::uint128_t>{U, Prefixes[S.sign]}, S,
        Ctx.locale());
  }
};
FMT_END_NAMESPACE

#endif
