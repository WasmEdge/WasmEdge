// SPDX-License-Identifier: Apache-2.0
#include "common/log.h"
#include "common/value.h"
#include "interpreter/interpreter.h"

#include <cmath>

namespace WasmEdge {
namespace Interpreter {

template <typename T>
TypeN<T> Interpreter::runAddOp(ValVariant &Val1, const ValVariant &Val2) const {
  /// Integer case: Return the result of (v1 + v2) modulo 2^N.
  /// Floating case: NaN, inf, and zeros are handled.
  retrieveValue<T>(Val1) += retrieveValue<T>(Val2);
  return {};
}

template <typename T>
TypeN<T> Interpreter::runSubOp(ValVariant &Val1, const ValVariant &Val2) const {
  /// Integer case: Return the result of (v1 - v2) modulo 2^N.
  /// Floating case: NaN, inf, and zeros are handled.
  retrieveValue<T>(Val1) -= retrieveValue<T>(Val2);
  return {};
}

template <typename T>
TypeN<T> Interpreter::runMulOp(ValVariant &Val1, const ValVariant &Val2) const {
  /// Integer case: Return the result of (v1 * v2) modulo 2^N.
  /// Floating case: NaN, inf, and zeros are handled.
  retrieveValue<T>(Val1) *= retrieveValue<T>(Val2);
  return {};
}

template <typename T>
TypeT<T> Interpreter::runDivOp(const AST::Instruction &Instr, ValVariant &Val1,
                               const ValVariant &Val2) const {
  T &V1 = retrieveValue<T>(Val1);
  const T &V2 = retrieveValue<T>(Val2);
  if (!std::is_floating_point_v<T>) {
    if (V2 == 0) {
      /// Integer case: If v2 is 0, then the result is undefined.
      spdlog::error(ErrCode::DivideByZero);
      spdlog::error(ErrInfo::InfoInstruction(
          Instr.getOpCode(), Instr.getOffset(), {Val1, Val2},
          {ValTypeFromType<T>(), ValTypeFromType<T>()}, std::is_signed_v<T>));
      return Unexpect(ErrCode::DivideByZero);
    }
    if (std::is_signed_v<T> && V1 == std::numeric_limits<T>::min() &&
        V2 == static_cast<T>(-1)) {
      /// Signed Integer case: If signed(v1) / signed(v2) is 2^(N − 1), then the
      /// result is undefined.
      spdlog::error(ErrCode::IntegerOverflow);
      spdlog::error(ErrInfo::InfoInstruction(
          Instr.getOpCode(), Instr.getOffset(), {Val1, Val2},
          {ValTypeFromType<T>(), ValTypeFromType<T>()}, true));
      return Unexpect(ErrCode::IntegerOverflow);
    }
  }
  /// Else, return the result of v1 / v2.
  /// Integer case: truncated toward zero.
  /// Floating case: +-0.0, NaN, and Inf case are handled.
  V1 /= V2;
  return {};
}

template <typename T>
TypeI<T> Interpreter::runRemOp(const AST::Instruction &Instr, ValVariant &Val1,
                               const ValVariant &Val2) const {
  T &I1 = retrieveValue<T>(Val1);
  const T &I2 = retrieveValue<T>(Val2);
  /// If i2 is 0, then the result is undefined.
  if (I2 == 0) {
    spdlog::error(ErrCode::DivideByZero);
    spdlog::error(ErrInfo::InfoInstruction(
        Instr.getOpCode(), Instr.getOffset(), {Val1, Val2},
        {ValTypeFromType<T>(), ValTypeFromType<T>()}, std::is_signed_v<T>));
    return Unexpect(ErrCode::DivideByZero);
  }
  /// Else, return the i1 % i2. Signed case is handled.
  if (std::is_signed_v<T> && I2 == static_cast<T>(-1)) {
    /// Signed Integer case: If signed(v2) is -1, then the result 0.
    I1 = 0;
  } else {
    I1 %= I2;
  }
  return {};
}

template <typename T>
TypeU<T> Interpreter::runAndOp(ValVariant &Val1, const ValVariant &Val2) const {
  /// Return the bitwise conjunction of i1 and i2.
  retrieveValue<T>(Val1) &= retrieveValue<T>(Val2);
  return {};
}

template <typename T>
TypeU<T> Interpreter::runOrOp(ValVariant &Val1, const ValVariant &Val2) const {
  /// Return the bitwise disjunction of i1 and i2.
  retrieveValue<T>(Val1) |= retrieveValue<T>(Val2);
  return {};
}

template <typename T>
TypeU<T> Interpreter::runXorOp(ValVariant &Val1, const ValVariant &Val2) const {
  /// Return the bitwise exclusive disjunction of i1 and i2.
  retrieveValue<T>(Val1) ^= retrieveValue<T>(Val2);
  return {};
}

template <typename T>
TypeU<T> Interpreter::runShlOp(ValVariant &Val1, const ValVariant &Val2) const {
  /// Return the result of i1 << (i2 modulo N), modulo 2^N.
  retrieveValue<T>(Val1) <<= (retrieveValue<T>(Val2) % (sizeof(T) * 8));
  return {};
}

template <typename T>
TypeI<T> Interpreter::runShrOp(ValVariant &Val1, const ValVariant &Val2) const {
  /// Return the result of i1 >> (i2 modulo N).
  /// In signed case, extended with the sign bit of i1.
  /// In unsigned case, extended with 0 bits.
  retrieveValue<T>(Val1) >>= (retrieveValue<T>(Val2) % (sizeof(T) * 8));
  return {};
}

template <typename T>
TypeU<T> Interpreter::runRotlOp(ValVariant &Val1,
                                const ValVariant &Val2) const {
  T &I1 = retrieveValue<T>(Val1);
  /// Let k be i2 modulo N.
  const T K = retrieveValue<T>(Val2) % (sizeof(T) * 8);
  /// Return the result of rotating i1 left by k bits.
  if (likely(K != 0)) {
    I1 = (I1 << K) | (I1 >> (sizeof(T) * 8 - K));
  }
  return {};
}

template <typename T>
TypeU<T> Interpreter::runRotrOp(ValVariant &Val1,
                                const ValVariant &Val2) const {
  T &I1 = retrieveValue<T>(Val1);
  /// Let k be i2 modulo N.
  const T K = retrieveValue<T>(Val2) % (sizeof(T) * 8);
  /// Return the result of rotating i1 left by k bits.
  if (likely(K != 0)) {
    I1 = (I1 >> K) | (I1 << (sizeof(T) * 8 - K));
  }
  return {};
}

template <typename T>
TypeF<T> Interpreter::runMinOp(ValVariant &Val1, const ValVariant &Val2) const {
  T &Z1 = retrieveValue<T>(Val1);
  const T &Z2 = retrieveValue<T>(Val2);
  /// TODO: canonical and arithmetical NaN
  if (std::isnan(Z2)) {
    Z1 = Z2;
  } else if (Z1 == 0.0 && Z2 == 0.0 && std::signbit(Z1) != std::signbit(Z2)) {
    /// If both z1 and z2 are zeroes of opposite signs, then return -0.0.
    Z1 = -0.0;
  } else if (!std::isnan(Z1)) {
    /// Else return the min of z1 and z2. (Inf case are handled.)
    Z1 = std::min(Z1, Z2);
  }
  return {};
}

template <typename T>
TypeF<T> Interpreter::runMaxOp(ValVariant &Val1, const ValVariant &Val2) const {
  T &Z1 = retrieveValue<T>(Val1);
  const T &Z2 = retrieveValue<T>(Val2);
  /// TODO: canonical and arithmetical NaN
  if (std::isnan(Z2)) {
    Z1 = Z2;
  } else if (Z1 == 0.0 && Z2 == 0.0 && std::signbit(Z1) != std::signbit(Z2)) {
    /// If both z1 and z2 are zeroes of opposite signs, then return +0.0.
    Z1 = 0.0;
  } else if (!std::isnan(Z1)) {
    /// Else return the max of z1 and z2. (Inf case are handled.)
    Z1 = std::max(Z1, Z2);
  }
  return {};
}

template <typename T>
TypeF<T> Interpreter::runCopysignOp(ValVariant &Val1,
                                    const ValVariant &Val2) const {
  T &Z1 = retrieveValue<T>(Val1);
  const T &Z2 = retrieveValue<T>(Val2);
  /// Return z1 with the same sign with z2.
  Z1 = std::copysign(Z1, Z2);
  return {};
}

template <typename TIn, typename TOut>
Expect<void> Interpreter::runReplaceLaneOp(ValVariant &Val1,
                                           const ValVariant &Val2,
                                           const uint32_t Index) const {
  using VTOut [[gnu::vector_size(16)]] = TOut;
  VTOut &Result = retrieveValue<VTOut>(Val1);
  Result[Index] = retrieveValue<TIn>(Val2);
  return {};
}

template <typename T>
Expect<void> Interpreter::runVectorEqOp(ValVariant &Val1,
                                        const ValVariant &Val2) const {
  using VT [[gnu::vector_size(16)]] = T;

  VT &V1 = retrieveValue<VT>(Val1);
  const VT &V2 = retrieveValue<VT>(Val2);

  V1 = (V1 == V2);
  return {};
}

template <typename T>
Expect<void> Interpreter::runVectorNeOp(ValVariant &Val1,
                                        const ValVariant &Val2) const {
  using VT [[gnu::vector_size(16)]] = T;

  VT &V1 = retrieveValue<VT>(Val1);
  const VT &V2 = retrieveValue<VT>(Val2);

  V1 = (V1 != V2);
  return {};
}

template <typename T>
Expect<void> Interpreter::runVectorLtOp(ValVariant &Val1,
                                        const ValVariant &Val2) const {
  using VT [[gnu::vector_size(16)]] = T;

  VT &V1 = retrieveValue<VT>(Val1);
  const VT &V2 = retrieveValue<VT>(Val2);

  V1 = (V1 < V2);
  return {};
}

template <typename T>
Expect<void> Interpreter::runVectorGtOp(ValVariant &Val1,
                                        const ValVariant &Val2) const {
  using VT [[gnu::vector_size(16)]] = T;

  VT &V1 = retrieveValue<VT>(Val1);
  const VT &V2 = retrieveValue<VT>(Val2);

  V1 = (V1 > V2);
  return {};
}

template <typename T>
Expect<void> Interpreter::runVectorLeOp(ValVariant &Val1,
                                        const ValVariant &Val2) const {
  using VT [[gnu::vector_size(16)]] = T;

  VT &V1 = retrieveValue<VT>(Val1);
  const VT &V2 = retrieveValue<VT>(Val2);

  V1 = (V1 <= V2);
  return {};
}

template <typename T>
Expect<void> Interpreter::runVectorGeOp(ValVariant &Val1,
                                        const ValVariant &Val2) const {
  using VT [[gnu::vector_size(16)]] = T;

  VT &V1 = retrieveValue<VT>(Val1);
  const VT &V2 = retrieveValue<VT>(Val2);

  V1 = (V1 >= V2);
  return {};
}

template <typename TIn, typename TOut>
Expect<void> Interpreter::runVectorNarrowOp(ValVariant &Val1,
                                            const ValVariant &Val2) const {
  static_assert(sizeof(TOut) * 2 == sizeof(TIn));
  static_assert(sizeof(TOut) == 1 || sizeof(TOut) == 2);
  using VTIn [[gnu::vector_size(16)]] = TIn;
  using HVTOut [[gnu::vector_size(8)]] = TOut;
  using VTOut [[gnu::vector_size(16)]] = TOut;

  const TIn Min = static_cast<TIn>(std::numeric_limits<TOut>::min());
  const TIn Max = static_cast<TIn>(std::numeric_limits<TOut>::max());
  VTIn V1 = retrieveValue<VTIn>(Val1);
  VTIn V2 = retrieveValue<VTIn>(Val2);
  V1 = V1 < Min ? Min : V1;
  V1 = V1 > Max ? Max : V1;
  V2 = V2 < Min ? Min : V2;
  V2 = V2 > Max ? Max : V2;
  const HVTOut HV1 = __builtin_convertvector(V1, HVTOut);
  const HVTOut HV2 = __builtin_convertvector(V2, HVTOut);
  VTOut &Result = retrieveValue<VTOut>(Val1);
  if constexpr (sizeof(TOut) == 1) {
    Result =
        VTOut{HV1[0], HV1[1], HV1[2], HV1[3], HV1[4], HV1[5], HV1[6], HV1[7],
              HV2[0], HV2[1], HV2[2], HV2[3], HV2[4], HV2[5], HV2[6], HV2[7]};
  } else if constexpr (sizeof(TOut) == 2) {
    Result =
        VTOut{HV1[0], HV1[1], HV1[2], HV1[3], HV2[0], HV2[1], HV2[2], HV2[3]};
  }

  return {};
}

template <typename T>
Expect<void> Interpreter::runVectorShlOp(ValVariant &Val1,
                                         const ValVariant &Val2) const {
  using VT [[gnu::vector_size(16)]] = T;
  const uint32_t Mask = static_cast<uint32_t>(sizeof(T) * 8 - 1);
  VT &V1 = retrieveValue<VT>(Val1);
  V1 <<= retrieveValue<uint32_t>(Val2) & Mask;

  return {};
}

template <typename T>
Expect<void> Interpreter::runVectorShrOp(ValVariant &Val1,
                                         const ValVariant &Val2) const {
  using VT [[gnu::vector_size(16)]] = T;
  const uint32_t Mask = static_cast<uint32_t>(sizeof(T) * 8 - 1);
  VT &V1 = retrieveValue<VT>(Val1);
  V1 >>= retrieveValue<uint32_t>(Val2) & Mask;

  return {};
}

template <typename T>
Expect<void> Interpreter::runVectorAddOp(ValVariant &Val1,
                                         const ValVariant &Val2) const {
  using VT [[gnu::vector_size(16)]] = T;
  VT &V1 = retrieveValue<VT>(Val1);
  V1 += retrieveValue<VT>(Val2);

  return {};
}

template <typename T>
Expect<void> Interpreter::runVectorAddSatOp(ValVariant &Val1,
                                            const ValVariant &Val2) const {
  using VT [[gnu::vector_size(16)]] = T;
  using UVT [[gnu::vector_size(16)]] = std::make_unsigned_t<T>;
  UVT &V1 = retrieveValue<UVT>(Val1);
  const UVT &V2 = retrieveValue<UVT>(Val2);
  const UVT Result = V1 + V2;

  if constexpr (std::is_signed_v<T>) {
    const UVT Limit =
        (V1 >> (sizeof(T) * 8 - 1)) + std::numeric_limits<T>::max();
    const VT Over = reinterpret_cast<VT>((V1 ^ V2) | ~(V2 ^ Result));
    V1 = Over >= 0 ? Limit : Result;
  } else {
    V1 = Result | (Result < V1);
  }

  return {};
}

template <typename T>
Expect<void> Interpreter::runVectorSubOp(ValVariant &Val1,
                                         const ValVariant &Val2) const {
  using VT [[gnu::vector_size(16)]] = T;
  VT &V1 = retrieveValue<VT>(Val1);
  V1 -= retrieveValue<VT>(Val2);

  return {};
}

template <typename T>
Expect<void> Interpreter::runVectorSubSatOp(ValVariant &Val1,
                                            const ValVariant &Val2) const {
  using VT [[gnu::vector_size(16)]] = T;
  using UVT [[gnu::vector_size(16)]] = std::make_unsigned_t<T>;
  UVT &V1 = retrieveValue<UVT>(Val1);
  const UVT &V2 = retrieveValue<UVT>(Val2);
  const UVT Result = V1 - V2;

  if constexpr (std::is_signed_v<T>) {
    const UVT Limit =
        (V1 >> (sizeof(T) * 8 - 1)) + std::numeric_limits<T>::max();
    const VT Under = reinterpret_cast<VT>((V1 ^ V2) & (V1 ^ Result));
    V1 = Under < 0 ? Limit : Result;
  } else {
    V1 = Result & (Result <= V1);
  }

  return {};
}

template <typename T>
Expect<void> Interpreter::runVectorMulOp(ValVariant &Val1,
                                         const ValVariant &Val2) const {
  using VT [[gnu::vector_size(16)]] = T;
  VT &V1 = retrieveValue<VT>(Val1);
  V1 *= retrieveValue<VT>(Val2);

  return {};
}

template <typename T>
Expect<void> Interpreter::runVectorDivOp(ValVariant &Val1,
                                         const ValVariant &Val2) const {
  using VT [[gnu::vector_size(16)]] = T;
  VT &V1 = retrieveValue<VT>(Val1);
  V1 /= retrieveValue<VT>(Val2);

  return {};
}

template <typename T>
Expect<void> Interpreter::runVectorMinOp(ValVariant &Val1,
                                         const ValVariant &Val2) const {
  using VT [[gnu::vector_size(16)]] = T;
  VT &V1 = retrieveValue<VT>(Val1);
  const VT &V2 = retrieveValue<VT>(Val2);
  V1 = V1 > V2 ? V2 : V1;

  return {};
}

template <typename T>
Expect<void> Interpreter::runVectorMaxOp(ValVariant &Val1,
                                         const ValVariant &Val2) const {
  using VT [[gnu::vector_size(16)]] = T;
  VT &V1 = retrieveValue<VT>(Val1);
  const VT &V2 = retrieveValue<VT>(Val2);
  V1 = V2 > V1 ? V2 : V1;

  return {};
}

template <typename T>
Expect<void> Interpreter::runVectorFMinOp(ValVariant &Val1,
                                          const ValVariant &Val2) const {
  static_assert(std::is_floating_point_v<T>);
  using VT [[gnu::vector_size(16)]] = T;
  VT &V1 = retrieveValue<VT>(Val1);
  const VT &V2 = retrieveValue<VT>(Val2);
  VT A = (V1 < V2) ? V1 : V2;
  VT B = (V2 < V1) ? V2 : V1;
  V1 = reinterpret_cast<VT>(reinterpret_cast<uint64x2_t>(A) |
                            reinterpret_cast<uint64x2_t>(B));

  return {};
}

template <typename T>
Expect<void> Interpreter::runVectorFMaxOp(ValVariant &Val1,
                                          const ValVariant &Val2) const {
  using VT [[gnu::vector_size(16)]] = T;
  VT &V1 = retrieveValue<VT>(Val1);
  const VT &V2 = retrieveValue<VT>(Val2);
  VT A = (V1 > V2) ? V1 : V2;
  VT B = (V2 > V1) ? V2 : V1;
  V1 = reinterpret_cast<VT>(reinterpret_cast<uint64x2_t>(A) &
                            reinterpret_cast<uint64x2_t>(B));

  return {};
}

template <typename T, typename ET>
Expect<void> Interpreter::runVectorAvgrOp(ValVariant &Val1,
                                          const ValVariant &Val2) const {
  static_assert(sizeof(T) * 2 == sizeof(ET));
  using VT [[gnu::vector_size(16)]] = T;
  using EVT [[gnu::vector_size(32)]] = ET;
  VT &V1 = retrieveValue<VT>(Val1);
  const VT &V2 = retrieveValue<VT>(Val2);
  const EVT EV1 = __builtin_convertvector(V1, EVT);
  const EVT EV2 = __builtin_convertvector(V2, EVT);
  /// Add 1 for rounding up .5
  V1 = __builtin_convertvector((EV1 + EV2 + 1) / 2, VT);

  return {};
}

template <typename TIn, typename TOut>
Expect<void> Interpreter::runVectorExtMulLowOp(ValVariant &Val1,
                                               const ValVariant &Val2) const {
  static_assert(sizeof(TIn) * 2 == sizeof(TOut));
  static_assert(sizeof(TIn) == 1 || sizeof(TIn) == 2 || sizeof(TIn) == 4);
  using VTIn [[gnu::vector_size(16)]] = TIn;
  using HVTIn [[gnu::vector_size(8)]] = TIn;
  using VTOut [[gnu::vector_size(16)]] = TOut;
  const VTIn &V1 = retrieveValue<VTIn>(Val1);
  const VTIn &V2 = retrieveValue<VTIn>(Val2);
  VTOut &Result = retrieveValue<VTOut>(Val1);
  if constexpr (sizeof(TIn) == 1) {
    const VTOut E1 = __builtin_convertvector(
        HVTIn{V1[0], V1[1], V1[2], V1[3], V1[4], V1[5], V1[6], V1[7]}, VTOut);
    const VTOut E2 = __builtin_convertvector(
        HVTIn{V2[0], V2[1], V2[2], V2[3], V2[4], V2[5], V2[6], V2[7]}, VTOut);
    Result = E1 * E2;
  } else if constexpr (sizeof(TIn) == 2) {
    const VTOut E1 =
        __builtin_convertvector(HVTIn{V1[0], V1[1], V1[2], V1[3]}, VTOut);
    const VTOut E2 =
        __builtin_convertvector(HVTIn{V2[0], V2[1], V2[2], V2[3]}, VTOut);
    Result = E1 * E2;
  } else if constexpr (sizeof(TIn) == 4) {
    const VTOut E1 = __builtin_convertvector(HVTIn{V1[0], V1[1]}, VTOut);
    const VTOut E2 = __builtin_convertvector(HVTIn{V2[0], V2[1]}, VTOut);
    Result = E1 * E2;
  }
  return {};
}

template <typename TIn, typename TOut>
Expect<void> Interpreter::runVectorExtMulHighOp(ValVariant &Val1,
                                                const ValVariant &Val2) const {
  static_assert(sizeof(TIn) * 2 == sizeof(TOut));
  static_assert(sizeof(TIn) == 1 || sizeof(TIn) == 2 || sizeof(TIn) == 4);
  using VTIn [[gnu::vector_size(16)]] = TIn;
  using HVTIn [[gnu::vector_size(8)]] = TIn;
  using VTOut [[gnu::vector_size(16)]] = TOut;
  const VTIn &V1 = retrieveValue<VTIn>(Val1);
  const VTIn &V2 = retrieveValue<VTIn>(Val2);
  VTOut &Result = retrieveValue<VTOut>(Val1);
  if constexpr (sizeof(TIn) == 1) {
    const VTOut E1 = __builtin_convertvector(
        HVTIn{V1[8], V1[9], V1[10], V1[11], V1[12], V1[13], V1[14], V1[15]},
        VTOut);
    const VTOut E2 = __builtin_convertvector(
        HVTIn{V2[8], V2[9], V2[10], V2[11], V2[12], V2[13], V2[14], V2[15]},
        VTOut);
    Result = E1 * E2;
  } else if constexpr (sizeof(TIn) == 2) {
    const VTOut E1 =
        __builtin_convertvector(HVTIn{V1[4], V1[5], V1[6], V1[7]}, VTOut);
    const VTOut E2 =
        __builtin_convertvector(HVTIn{V2[4], V2[5], V2[6], V2[7]}, VTOut);
    Result = E1 * E2;
  } else if constexpr (sizeof(TIn) == 4) {
    const VTOut E1 = __builtin_convertvector(HVTIn{V1[2], V1[3]}, VTOut);
    const VTOut E2 = __builtin_convertvector(HVTIn{V2[2], V2[3]}, VTOut);
    Result = E1 * E2;
  }
  return {};
}

inline Expect<void>
Interpreter::runVectorQ15MulSatOp(ValVariant &Val1,
                                  const ValVariant &Val2) const {
  using int32x8_t [[gnu::vector_size(32)]] = int32_t;
  const auto &V1 = retrieveValue<int16x8_t>(Val1);
  const auto &V2 = retrieveValue<int16x8_t>(Val2);
  auto &Result = retrieveValue<int16x8_t>(Val1);
  const auto EV1 = __builtin_convertvector(V1, int32x8_t);
  const auto EV2 = __builtin_convertvector(V2, int32x8_t);
  const auto ER = (EV1 * EV2 + INT32_C(0x4000)) >> INT32_C(15);
  const auto ERSat = ER > INT32_C(0x7fff) ? INT32_C(0x7fff) : ER;
  Result = __builtin_convertvector(ERSat, int16x8_t);
  return {};
}

} // namespace Interpreter
} // namespace WasmEdge
