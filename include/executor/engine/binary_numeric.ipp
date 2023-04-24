// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "executor/engine/vector_helper.h"
#include "executor/executor.h"

#include <cmath>

namespace WasmEdge {
namespace Executor {

template <typename T>
TypeN<T> Executor::runAddOp(ValVariant &Val1, const ValVariant &Val2) const {
  // Integer case: Return the result of (v1 + v2) modulo 2^N.
  // Floating case: NaN, inf, and zeros are handled.
  Val1.get<T>() += Val2.get<T>();
  return {};
}

template <typename T>
TypeN<T> Executor::runSubOp(ValVariant &Val1, const ValVariant &Val2) const {
  // Integer case: Return the result of (v1 - v2) modulo 2^N.
  // Floating case: NaN, inf, and zeros are handled.
  Val1.get<T>() -= Val2.get<T>();
  return {};
}

template <typename T>
TypeN<T> Executor::runMulOp(ValVariant &Val1, const ValVariant &Val2) const {
  // Integer case: Return the result of (v1 * v2) modulo 2^N.
  // Floating case: NaN, inf, and zeros are handled.
  Val1.get<T>() *= Val2.get<T>();
  return {};
}

template <typename T>
TypeT<T> Executor::runDivOp(const AST::Instruction &Instr, ValVariant &Val1,
                            const ValVariant &Val2) const {
  T &V1 = Val1.get<T>();
  const T &V2 = Val2.get<T>();
  if constexpr (!std::is_floating_point_v<T>) {
    if (V2 == 0) {
      // Integer case: If v2 is 0, then the result is undefined.
      spdlog::error(ErrCode::Value::DivideByZero);
      spdlog::error(ErrInfo::InfoInstruction(
          Instr.getOpCode(), Instr.getOffset(), {Val1, Val2},
          {ValTypeFromType<T>(), ValTypeFromType<T>()}, std::is_signed_v<T>));
      return Unexpect(ErrCode::Value::DivideByZero);
    }
    if (std::is_signed_v<T> && V1 == std::numeric_limits<T>::min() &&
        V2 == static_cast<T>(-1)) {
      // Signed Integer case: If signed(v1) / signed(v2) is 2^(N − 1), then the
      // result is undefined.
      spdlog::error(ErrCode::Value::IntegerOverflow);
      spdlog::error(ErrInfo::InfoInstruction(
          Instr.getOpCode(), Instr.getOffset(), {Val1, Val2},
          {ValTypeFromType<T>(), ValTypeFromType<T>()}, true));
      return Unexpect(ErrCode::Value::IntegerOverflow);
    }
  } else {
    static_assert(std::numeric_limits<T>::is_iec559, "Unsupported platform!");
  }
  // Else, return the result of v1 / v2.
  // Integer case: truncated toward zero.
  // Floating case: +-0.0, NaN, and Inf case are handled.
  V1 /= V2;
  return {};
}

template <typename T>
TypeI<T> Executor::runRemOp(const AST::Instruction &Instr, ValVariant &Val1,
                            const ValVariant &Val2) const {
  T &I1 = Val1.get<T>();
  const T &I2 = Val2.get<T>();
  // If i2 is 0, then the result is undefined.
  if (I2 == 0) {
    spdlog::error(ErrCode::Value::DivideByZero);
    spdlog::error(ErrInfo::InfoInstruction(
        Instr.getOpCode(), Instr.getOffset(), {Val1, Val2},
        {ValTypeFromType<T>(), ValTypeFromType<T>()}, std::is_signed_v<T>));
    return Unexpect(ErrCode::Value::DivideByZero);
  }
  // Else, return the i1 % i2. Signed case is handled.
  if (std::is_signed_v<T> && I2 == static_cast<T>(-1)) {
    // Signed Integer case: If signed(v2) is -1, then the result 0.
    I1 = 0;
  } else {
    I1 %= I2;
  }
  return {};
}

template <typename T>
TypeU<T> Executor::runAndOp(ValVariant &Val1, const ValVariant &Val2) const {
  // Return the bitwise conjunction of i1 and i2.
  Val1.get<T>() &= Val2.get<T>();
  return {};
}

template <typename T>
TypeU<T> Executor::runOrOp(ValVariant &Val1, const ValVariant &Val2) const {
  // Return the bitwise disjunction of i1 and i2.
  Val1.get<T>() |= Val2.get<T>();
  return {};
}

template <typename T>
TypeU<T> Executor::runXorOp(ValVariant &Val1, const ValVariant &Val2) const {
  // Return the bitwise exclusive disjunction of i1 and i2.
  Val1.get<T>() ^= Val2.get<T>();
  return {};
}

template <typename T>
TypeU<T> Executor::runShlOp(ValVariant &Val1, const ValVariant &Val2) const {
  // Return the result of i1 << (i2 modulo N), modulo 2^N.
  Val1.get<T>() <<= (Val2.get<T>() % static_cast<T>(sizeof(T) * 8));
  return {};
}

template <typename T>
TypeI<T> Executor::runShrOp(ValVariant &Val1, const ValVariant &Val2) const {
  // Return the result of i1 >> (i2 modulo N).
  // In signed case, extended with the sign bit of i1.
  // In unsigned case, extended with 0 bits.
  using UT = std::make_unsigned_t<T>;
  Val1.get<T>() >>= (Val2.get<UT>() % static_cast<T>(sizeof(T) * 8));
  return {};
}

template <typename T>
TypeU<T> Executor::runRotlOp(ValVariant &Val1, const ValVariant &Val2) const {
  T &I1 = Val1.get<T>();
  // Let k be i2 modulo N.
  const T K = Val2.get<T>() % static_cast<T>(sizeof(T) * 8);
  // Return the result of rotating i1 left by k bits.
  if (likely(K != 0)) {
    I1 = (I1 << K) | (I1 >> static_cast<T>(sizeof(T) * 8 - K));
  }
  return {};
}

template <typename T>
TypeU<T> Executor::runRotrOp(ValVariant &Val1, const ValVariant &Val2) const {
  T &I1 = Val1.get<T>();
  // Let k be i2 modulo N.
  const T K = Val2.get<T>() % static_cast<T>(sizeof(T) * 8);
  // Return the result of rotating i1 left by k bits.
  if (likely(K != 0)) {
    I1 = (I1 >> K) | (I1 << static_cast<T>(sizeof(T) * 8 - K));
  }
  return {};
}

template <typename T>
TypeF<T> Executor::runMinOp(ValVariant &Val1, const ValVariant &Val2) const {
  T &Z1 = Val1.get<T>();
  const T &Z2 = Val2.get<T>();
  const T kZero = 0.0;
  if (std::isnan(Z1) || std::isnan(Z2)) {
    if (std::isnan(Z2)) {
      Z1 = Z2;
    }
    // Set the most significant bit of the payload to 1.
    if constexpr (sizeof(T) == sizeof(uint32_t)) {
      uint32_t I32;
      std::memcpy(&I32, &Z1, sizeof(T));
      I32 |= static_cast<uint32_t>(0x01U) << 22;
      std::memcpy(&Z1, &I32, sizeof(T));
    } else if constexpr (sizeof(T) == sizeof(uint64_t)) {
      uint64_t I64;
      std::memcpy(&I64, &Z1, sizeof(T));
      I64 |= static_cast<uint64_t>(0x01U) << 51;
      std::memcpy(&Z1, &I64, sizeof(T));
    }
  } else if (Z1 == kZero && Z2 == kZero &&
             std::signbit(Z1) != std::signbit(Z2)) {
    // If both z1 and z2 are zeroes of opposite signs, then return -0.0.
    Z1 = -kZero;
  } else {
    // Else return the min of z1 and z2. (Inf case are handled.)
    Z1 = std::min(Z1, Z2);
  }
  return {};
}

template <typename T>
TypeF<T> Executor::runMaxOp(ValVariant &Val1, const ValVariant &Val2) const {
  T &Z1 = Val1.get<T>();
  const T &Z2 = Val2.get<T>();
  const T kZero = 0.0;
  if (std::isnan(Z1) || std::isnan(Z2)) {
    if (std::isnan(Z2)) {
      Z1 = Z2;
    }
    // Set the most significant bit of the payload to 1.
    if constexpr (sizeof(T) == sizeof(uint32_t)) {
      uint32_t I32;
      std::memcpy(&I32, &Z1, sizeof(T));
      I32 |= static_cast<uint32_t>(0x01U) << 22;
      std::memcpy(&Z1, &I32, sizeof(T));
    } else if constexpr (sizeof(T) == sizeof(uint64_t)) {
      uint64_t I64;
      std::memcpy(&I64, &Z1, sizeof(T));
      I64 |= static_cast<uint64_t>(0x01U) << 51;
      std::memcpy(&Z1, &I64, sizeof(T));
    }
  } else if (Z1 == kZero && Z2 == kZero &&
             std::signbit(Z1) != std::signbit(Z2)) {
    // If both z1 and z2 are zeroes of opposite signs, then return +0.0.
    Z1 = kZero;
  } else {
    // Else return the max of z1 and z2. (Inf case are handled.)
    Z1 = std::max(Z1, Z2);
  }
  return {};
}

template <typename T>
TypeF<T> Executor::runCopysignOp(ValVariant &Val1,
                                 const ValVariant &Val2) const {
  T &Z1 = Val1.get<T>();
  const T &Z2 = Val2.get<T>();
  // Return z1 with the same sign with z2.
  Z1 = std::copysign(Z1, Z2);
  return {};
}

template <typename TIn, typename TOut>
Expect<void> Executor::runReplaceLaneOp(ValVariant &Val1,
                                        const ValVariant &Val2,
                                        const uint8_t Index) const {
  using VTOut [[gnu::vector_size(16)]] = TOut;
  VTOut &Result = Val1.get<VTOut>();
  Result[Index] = static_cast<TOut>(Val2.get<TIn>());
  return {};
}

template <typename T>
Expect<void> Executor::runVectorEqOp(ValVariant &Val1,
                                     const ValVariant &Val2) const {
  using VT [[gnu::vector_size(16)]] = T;

  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();

  V1 = (V1 == V2);
  return {};
}

template <typename T>
Expect<void> Executor::runVectorNeOp(ValVariant &Val1,
                                     const ValVariant &Val2) const {
  using VT [[gnu::vector_size(16)]] = T;

  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();

  V1 = (V1 != V2);
  return {};
}

template <typename T>
Expect<void> Executor::runVectorLtOp(ValVariant &Val1,
                                     const ValVariant &Val2) const {
  using VT [[gnu::vector_size(16)]] = T;

  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();

  V1 = (V1 < V2);
  return {};
}

template <typename T>
Expect<void> Executor::runVectorGtOp(ValVariant &Val1,
                                     const ValVariant &Val2) const {
  using VT [[gnu::vector_size(16)]] = T;

  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();

  V1 = (V1 > V2);
  return {};
}

template <typename T>
Expect<void> Executor::runVectorLeOp(ValVariant &Val1,
                                     const ValVariant &Val2) const {
  using VT [[gnu::vector_size(16)]] = T;

  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();

  V1 = (V1 <= V2);
  return {};
}

template <typename T>
Expect<void> Executor::runVectorGeOp(ValVariant &Val1,
                                     const ValVariant &Val2) const {
  using VT [[gnu::vector_size(16)]] = T;

  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();

  V1 = (V1 >= V2);
  return {};
}

template <typename TIn, typename TOut>
Expect<void> Executor::runVectorNarrowOp(ValVariant &Val1,
                                         const ValVariant &Val2) const {
  static_assert(sizeof(TOut) * 2 == sizeof(TIn));
  static_assert(sizeof(TOut) == 1 || sizeof(TOut) == 2);
  using VTIn [[gnu::vector_size(16)]] = TIn;
  using HVTOut [[gnu::vector_size(8)]] = TOut;
  using VTOut [[gnu::vector_size(16)]] = TOut;

  const VTIn Min = VTIn{} + static_cast<TIn>(std::numeric_limits<TOut>::min());
  const VTIn Max = VTIn{} + static_cast<TIn>(std::numeric_limits<TOut>::max());
  VTIn V1 = Val1.get<VTIn>();
  VTIn V2 = Val2.get<VTIn>();
  V1 = detail::vectorSelect(V1 < Min, Min, V1);
  V1 = detail::vectorSelect(V1 > Max, Max, V1);
  V2 = detail::vectorSelect(V2 < Min, Min, V2);
  V2 = detail::vectorSelect(V2 > Max, Max, V2);
  const HVTOut HV1 = __builtin_convertvector(V1, HVTOut);
  const HVTOut HV2 = __builtin_convertvector(V2, HVTOut);
  if constexpr (sizeof(TOut) == 1) {
    Val1.emplace<VTOut>(VTOut{HV1[0], HV1[1], HV1[2], HV1[3], HV1[4], HV1[5],
                              HV1[6], HV1[7], HV2[0], HV2[1], HV2[2], HV2[3],
                              HV2[4], HV2[5], HV2[6], HV2[7]});
  } else if constexpr (sizeof(TOut) == 2) {
    Val1.emplace<VTOut>(
        VTOut{HV1[0], HV1[1], HV1[2], HV1[3], HV2[0], HV2[1], HV2[2], HV2[3]});
  }

  return {};
}

template <typename T>
Expect<void> Executor::runVectorShlOp(ValVariant &Val1,
                                      const ValVariant &Val2) const {
  using VT [[gnu::vector_size(16)]] = T;
  const uint32_t Mask = static_cast<uint32_t>(sizeof(T) * 8 - 1);
  VT &V1 = Val1.get<VT>();
  V1 <<= Val2.get<uint32_t>() & Mask;

  return {};
}

template <typename T>
Expect<void> Executor::runVectorShrOp(ValVariant &Val1,
                                      const ValVariant &Val2) const {
  using VT [[gnu::vector_size(16)]] = T;
  const uint32_t Mask = static_cast<uint32_t>(sizeof(T) * 8 - 1);
  VT &V1 = Val1.get<VT>();
  V1 >>= Val2.get<uint32_t>() & Mask;

  return {};
}

template <typename T>
Expect<void> Executor::runVectorAddOp(ValVariant &Val1,
                                      const ValVariant &Val2) const {
  using VT [[gnu::vector_size(16)]] = T;
  VT &V1 = Val1.get<VT>();
  V1 += Val2.get<VT>();

  return {};
}

template <typename T>
Expect<void> Executor::runVectorAddSatOp(ValVariant &Val1,
                                         const ValVariant &Val2) const {
  using VT [[gnu::vector_size(16)]] = T;
  using UVT [[gnu::vector_size(16)]] = std::make_unsigned_t<T>;
  UVT &V1 = Val1.get<UVT>();
  const UVT &V2 = Val2.get<UVT>();
  const UVT Result = V1 + V2;

  if constexpr (std::is_signed_v<T>) {
    const UVT Limit =
        (V1 >> (sizeof(T) * 8 - 1)) + std::numeric_limits<T>::max();
    const VT Over = reinterpret_cast<VT>((V1 ^ V2) | ~(V2 ^ Result));
    V1 = detail::vectorSelect(Over >= 0, Limit, Result);
  } else {
    V1 = Result | (Result < V1);
  }

  return {};
}

template <typename T>
Expect<void> Executor::runVectorSubOp(ValVariant &Val1,
                                      const ValVariant &Val2) const {
  using VT [[gnu::vector_size(16)]] = T;
  VT &V1 = Val1.get<VT>();
  V1 -= Val2.get<VT>();

  return {};
}

template <typename T>
Expect<void> Executor::runVectorSubSatOp(ValVariant &Val1,
                                         const ValVariant &Val2) const {
  using VT [[gnu::vector_size(16)]] = T;
  using UVT [[gnu::vector_size(16)]] = std::make_unsigned_t<T>;
  UVT &V1 = Val1.get<UVT>();
  const UVT &V2 = Val2.get<UVT>();
  const UVT Result = V1 - V2;

  if constexpr (std::is_signed_v<T>) {
    const UVT Limit =
        (V1 >> (sizeof(T) * 8 - 1)) + std::numeric_limits<T>::max();
    const VT Under = reinterpret_cast<VT>((V1 ^ V2) & (V1 ^ Result));
    V1 = detail::vectorSelect(Under < 0, Limit, Result);
  } else {
    V1 = Result & (Result <= V1);
  }

  return {};
}

template <typename T>
Expect<void> Executor::runVectorMulOp(ValVariant &Val1,
                                      const ValVariant &Val2) const {
  using VT [[gnu::vector_size(16)]] = T;
  VT &V1 = Val1.get<VT>();
  V1 *= Val2.get<VT>();

  return {};
}

template <typename T>
Expect<void> Executor::runVectorDivOp(ValVariant &Val1,
                                      const ValVariant &Val2) const {
  using VT [[gnu::vector_size(16)]] = T;
  VT &V1 = Val1.get<VT>();
  V1 /= Val2.get<VT>();

  return {};
}

template <typename T>
Expect<void> Executor::runVectorMinOp(ValVariant &Val1,
                                      const ValVariant &Val2) const {
  using VT [[gnu::vector_size(16)]] = T;
  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();
  V1 = detail::vectorSelect(V1 > V2, V2, V1);

  return {};
}

template <typename T>
Expect<void> Executor::runVectorMaxOp(ValVariant &Val1,
                                      const ValVariant &Val2) const {
  using VT [[gnu::vector_size(16)]] = T;
  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();
  V1 = detail::vectorSelect(V2 > V1, V2, V1);

  return {};
}

template <typename T>
Expect<void> Executor::runVectorFMinOp(ValVariant &Val1,
                                       const ValVariant &Val2) const {
  static_assert(std::is_floating_point_v<T>);
  using VT [[gnu::vector_size(16)]] = T;
  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();
  VT R = reinterpret_cast<VT>(reinterpret_cast<uint64x2_t>(V1) |
                              reinterpret_cast<uint64x2_t>(V2));
  R = detail::vectorSelect(V1 < V2, V1, R);
  R = detail::vectorSelect(V1 > V2, V2, R);
  R = detail::vectorSelect(V1 == V1, R, V1);
  R = detail::vectorSelect(V2 == V2, R, V2);
  V1 = R;

  return {};
}

template <typename T>
Expect<void> Executor::runVectorFMaxOp(ValVariant &Val1,
                                       const ValVariant &Val2) const {
  using VT [[gnu::vector_size(16)]] = T;
  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();
  VT R = reinterpret_cast<VT>(reinterpret_cast<uint64x2_t>(V1) &
                              reinterpret_cast<uint64x2_t>(V2));
  R = detail::vectorSelect(V1 < V2, V2, R);
  R = detail::vectorSelect(V1 > V2, V1, R);
  R = detail::vectorSelect(V1 == V1, R, V1);
  R = detail::vectorSelect(V2 == V2, R, V2);
  V1 = R;

  return {};
}

template <typename T, typename ET>
Expect<void> Executor::runVectorAvgrOp(ValVariant &Val1,
                                       const ValVariant &Val2) const {
  static_assert(sizeof(T) * 2 == sizeof(ET));
  using VT [[gnu::vector_size(16)]] = T;
  using EVT [[gnu::vector_size(32)]] = ET;
  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();
  const EVT EV1 = __builtin_convertvector(V1, EVT);
  const EVT EV2 = __builtin_convertvector(V2, EVT);
  // Add 1 for rounding up .5
  V1 = __builtin_convertvector((EV1 + EV2 + 1) / 2, VT);

  return {};
}

template <typename TIn, typename TOut>
Expect<void> Executor::runVectorExtMulLowOp(ValVariant &Val1,
                                            const ValVariant &Val2) const {
  static_assert(sizeof(TIn) * 2 == sizeof(TOut));
  static_assert(sizeof(TIn) == 1 || sizeof(TIn) == 2 || sizeof(TIn) == 4);
  using VTIn [[gnu::vector_size(16)]] = TIn;
  using HVTIn [[gnu::vector_size(8)]] = TIn;
  using VTOut [[gnu::vector_size(16)]] = TOut;
  const VTIn &V1 = Val1.get<VTIn>();
  const VTIn &V2 = Val2.get<VTIn>();
  if constexpr (sizeof(TIn) == 1) {
    const VTOut E1 = __builtin_convertvector(
        HVTIn{V1[0], V1[1], V1[2], V1[3], V1[4], V1[5], V1[6], V1[7]}, VTOut);
    const VTOut E2 = __builtin_convertvector(
        HVTIn{V2[0], V2[1], V2[2], V2[3], V2[4], V2[5], V2[6], V2[7]}, VTOut);
    Val1.emplace<VTOut>(E1 * E2);
  } else if constexpr (sizeof(TIn) == 2) {
    const VTOut E1 =
        __builtin_convertvector(HVTIn{V1[0], V1[1], V1[2], V1[3]}, VTOut);
    const VTOut E2 =
        __builtin_convertvector(HVTIn{V2[0], V2[1], V2[2], V2[3]}, VTOut);
    Val1.emplace<VTOut>(E1 * E2);
  } else if constexpr (sizeof(TIn) == 4) {
    const VTOut E1 = __builtin_convertvector(HVTIn{V1[0], V1[1]}, VTOut);
    const VTOut E2 = __builtin_convertvector(HVTIn{V2[0], V2[1]}, VTOut);
    Val1.emplace<VTOut>(E1 * E2);
  }
  return {};
}

template <typename TIn, typename TOut>
Expect<void> Executor::runVectorExtMulHighOp(ValVariant &Val1,
                                             const ValVariant &Val2) const {
  static_assert(sizeof(TIn) * 2 == sizeof(TOut));
  static_assert(sizeof(TIn) == 1 || sizeof(TIn) == 2 || sizeof(TIn) == 4);
  using VTIn [[gnu::vector_size(16)]] = TIn;
  using HVTIn [[gnu::vector_size(8)]] = TIn;
  using VTOut [[gnu::vector_size(16)]] = TOut;
  const VTIn &V1 = Val1.get<VTIn>();
  const VTIn &V2 = Val2.get<VTIn>();
  if constexpr (sizeof(TIn) == 1) {
    const VTOut E1 = __builtin_convertvector(
        HVTIn{V1[8], V1[9], V1[10], V1[11], V1[12], V1[13], V1[14], V1[15]},
        VTOut);
    const VTOut E2 = __builtin_convertvector(
        HVTIn{V2[8], V2[9], V2[10], V2[11], V2[12], V2[13], V2[14], V2[15]},
        VTOut);
    Val1.emplace<VTOut>(E1 * E2);
  } else if constexpr (sizeof(TIn) == 2) {
    const VTOut E1 =
        __builtin_convertvector(HVTIn{V1[4], V1[5], V1[6], V1[7]}, VTOut);
    const VTOut E2 =
        __builtin_convertvector(HVTIn{V2[4], V2[5], V2[6], V2[7]}, VTOut);
    Val1.emplace<VTOut>(E1 * E2);
  } else if constexpr (sizeof(TIn) == 4) {
    const VTOut E1 = __builtin_convertvector(HVTIn{V1[2], V1[3]}, VTOut);
    const VTOut E2 = __builtin_convertvector(HVTIn{V2[2], V2[3]}, VTOut);
    Val1.emplace<VTOut>(E1 * E2);
  }
  return {};
}

inline Expect<void>
Executor::runVectorQ15MulSatOp(ValVariant &Val1, const ValVariant &Val2) const {
  using int32x8_t [[gnu::vector_size(32)]] = int32_t;
  const auto &V1 = Val1.get<int16x8_t>();
  const auto &V2 = Val2.get<int16x8_t>();
  const auto EV1 = __builtin_convertvector(V1, int32x8_t);
  const auto EV2 = __builtin_convertvector(V2, int32x8_t);
  const auto ER = (EV1 * EV2 + INT32_C(0x4000)) >> INT32_C(15);
  const int32x8_t Cap = int32x8_t{} + INT32_C(0x7fff);
  const auto ERSat = detail::vectorSelect(ER > Cap, Cap, ER);
  Val1.emplace<int16x8_t>(__builtin_convertvector(ERSat, int16x8_t));
  return {};
}

} // namespace Executor
} // namespace WasmEdge
