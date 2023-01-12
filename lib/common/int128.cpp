// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "common/int128.h"

#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <ostream>
#include <sstream>

namespace WasmEdge {
static inline uint128_t mod(uint128_t LHS, uint128_t RHS) noexcept;
static inline uint128_t div(uint128_t LHS, uint128_t RHS) noexcept;

// https://developercommunity.visualstudio.com/t/no-symbol-udivti3-in-clang-rtbuiltins-x86-64lib/1510286
// uint128_t on MSVC clang has bugs. We can not use div and mod with uint128_t.

#if defined(__clang__) && defined(_MSC_VER)
constexpr static inline uint64_t low(uint128_t Value) noexcept {
  const uint128_t Mask = std::numeric_limits<uint64_t>::max();
  return Value & Mask;
}
constexpr static inline uint64_t high(uint128_t Value) noexcept {
  return Value >> 64;
}
constexpr static inline uint32_t clz(uint128_t Value) noexcept {
  uint64_t Hi = high(Value);
  uint64_t Lo = low(Value);

  if (Hi) {
    return uint32_t(__builtin_clzll(Hi));
  }
  if (Lo) {
    return uint32_t(__builtin_clzll(Lo) + 64);
  }
  return 128;
}

static inline uint128_t mod(uint128_t LHS, uint128_t RHS) noexcept {
  if (RHS > LHS) {
    return LHS;
  }
  if (RHS == LHS) {
    return 0;
  }
  uint128_t Denominator = RHS;
  const unsigned int Shift = clz(RHS) - clz(LHS);
  Denominator <<= Shift;
  for (unsigned int I = 0; I <= Shift; ++I) {
    if (LHS >= Denominator) {
      LHS -= Denominator;
    }
    Denominator >>= 1U;
  }
  return LHS;
}

static inline uint128_t div(uint128_t LHS, uint128_t RHS) noexcept {
  if (RHS > LHS) {
    return 0;
  }
  if (RHS == LHS) {
    return 1;
  }
  uint128_t Denominator = RHS;
  uint128_t Quotient = 0;
  const unsigned int Shift = clz(RHS) - clz(LHS);
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

#else
static inline uint128_t mod(uint128_t LHS, uint128_t RHS) noexcept {
  return LHS % RHS;
}
static inline uint128_t div(uint128_t LHS, uint128_t RHS) noexcept {
  return LHS / RHS;
}
#endif

std::ostream &operator<<(std::ostream &OS, uint128_t Value) {
  std::ostringstream Buf;

  if (Value <= uint128_t(std::numeric_limits<uint64_t>::max())) {
    OS << uint64_t(Value);
    return OS;
  }

  // 2**128 < 3.5e38 < (1e13) ^3
  const uint64_t P10 = 10000000000000;
  const uint64_t LEN = 13; /*Zeros in P10*/

  uint64_t Lo, Mi, Hi;
  Lo = uint64_t(mod(Value, P10));
  Value = div(Value, P10);
  Mi = uint64_t(mod(Value, P10));
  Value = div(Value, P10);
  Hi = uint64_t(Value);

  bool NeedLeadingZero = false;

  if (Hi) {
    Buf << Hi;
    NeedLeadingZero = true;
  }

  if (Mi || NeedLeadingZero) {
    if (NeedLeadingZero) {
      Buf << std::setw(LEN) << std::setfill('0');
    }
    Buf << Mi;
    NeedLeadingZero = true;
  }

  if (Lo || NeedLeadingZero) {
    if (NeedLeadingZero) {
      Buf << std::setw(LEN) << std::setfill('0');
    }
    Buf << Lo;
    NeedLeadingZero = true;
  }

  return OS << Buf.str();
}
} // namespace WasmEdge
