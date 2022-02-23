// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "executor/engine/vector_helper.h"
#include "executor/executor.h"

#include <cmath>

namespace WasmEdge {
namespace Executor {

template <typename T>
TypeN<T> Executor::runAddOp(Runtime::StackManager &StackMgr) const noexcept {
  // Integer case: Return the result of (v1 + v2) modulo 2^N.
  // Floating case: NaN, inf, and zeros are handled.
  const T V2 = StackMgr.pop<T>();
  const T V1 = StackMgr.pop<T>();
  StackMgr.push<T>(V1 + V2);
  return {};
}

template <typename T>
TypeN<T> Executor::runSubOp(Runtime::StackManager &StackMgr) const noexcept {
  // Integer case: Return the result of (v1 - v2) modulo 2^N.
  // Floating case: NaN, inf, and zeros are handled.
  const T V2 = StackMgr.pop<T>();
  const T V1 = StackMgr.pop<T>();
  StackMgr.push<T>(V1 - V2);
  return {};
}

template <typename T>
TypeN<T> Executor::runMulOp(Runtime::StackManager &StackMgr) const noexcept {
  // Integer case: Return the result of (v1 * v2) modulo 2^N.
  // Floating case: NaN, inf, and zeros are handled.
  const T V2 = StackMgr.pop<T>();
  const T V1 = StackMgr.pop<T>();
  StackMgr.push<T>(V1 * V2);
  return {};
}

template <typename T>
TypeT<T> Executor::runDivOp(Runtime::StackManager &StackMgr,
                            const AST::Instruction &Instr) const noexcept {
  const T V2 = StackMgr.pop<T>();
  const T V1 = StackMgr.pop<T>();
  if constexpr (!std::is_floating_point_v<T>) {
    if (unlikely(V2 == 0)) {
      // Integer case: If v2 is 0, then the result is undefined.
      spdlog::error(ErrCode::DivideByZero);
      spdlog::error(ErrInfo::InfoInstruction(
          Instr.getOpCode(), Instr.getOffset(), {V1, V2},
          {ValTypeFromType<T>(), ValTypeFromType<T>()}, std::is_signed_v<T>));
      return Unexpect(ErrCode::DivideByZero);
    }
    if constexpr (std::is_signed_v<T>) {
      if (unlikely(V1 == std::numeric_limits<T>::min()) &&
          unlikely(V2 == static_cast<T>(-1))) {
        // Signed Integer case: If signed(v1) / signed(v2) is 2^(N − 1), then
        // the result is undefined.
        spdlog::error(ErrCode::IntegerOverflow);
        spdlog::error(ErrInfo::InfoInstruction(
            Instr.getOpCode(), Instr.getOffset(), {V1, V2},
            {ValTypeFromType<T>(), ValTypeFromType<T>()}, true));
        return Unexpect(ErrCode::IntegerOverflow);
      }
    }
  } else {
    static_assert(std::numeric_limits<T>::is_iec559, "Unsupported platform!");
  }
  // Else, return the result of v1 / v2.
  // Integer case: truncated toward zero.
  // Floating case: +-0.0, NaN, and Inf case are handled.
  StackMgr.push<T>(V1 / V2);
  return {};
}

template <typename T>
TypeI<T> Executor::runRemOp(Runtime::StackManager &StackMgr,
                            const AST::Instruction &Instr) const noexcept {
  const T I2 = StackMgr.pop<T>();
  const T I1 = StackMgr.pop<T>();
  // If i2 is 0, then the result is undefined.
  if (I2 == 0) {
    spdlog::error(ErrCode::DivideByZero);
    spdlog::error(ErrInfo::InfoInstruction(
        Instr.getOpCode(), Instr.getOffset(), {I1, I2},
        {ValTypeFromType<T>(), ValTypeFromType<T>()}, std::is_signed_v<T>));
    return Unexpect(ErrCode::DivideByZero);
  }
  T R;
  // Else, return the i1 % i2. Signed case is handled.
  if constexpr (std::is_signed_v<T>) {
    if (I2 == static_cast<T>(-1)) {
      // Signed Integer case: If signed(v2) is -1, then the result 0.
      R = 0;
    } else {
      R = I1 % I2;
    }
  } else {
    R = I1 % I2;
  }
  StackMgr.push<T>(R);
  return {};
}

template <typename T>
TypeU<T> Executor::runAndOp(Runtime::StackManager &StackMgr) const noexcept {
  // Return the bitwise conjunction of i1 and i2.
  const T I2 = StackMgr.pop<T>();
  const T I1 = StackMgr.pop<T>();
  StackMgr.push<T>(I1 & I2);
  return {};
}

template <typename T>
TypeU<T> Executor::runOrOp(Runtime::StackManager &StackMgr) const noexcept {
  // Return the bitwise disjunction of i1 and i2.
  const T I2 = StackMgr.pop<T>();
  const T I1 = StackMgr.pop<T>();
  StackMgr.push<T>(I1 | I2);
  return {};
}

template <typename T>
TypeU<T> Executor::runXorOp(Runtime::StackManager &StackMgr) const noexcept {
  // Return the bitwise exclusive disjunction of i1 and i2.
  const T I2 = StackMgr.pop<T>();
  const T I1 = StackMgr.pop<T>();
  StackMgr.push<T>(I1 ^ I2);
  return {};
}

template <typename T>
TypeU<T> Executor::runShlOp(Runtime::StackManager &StackMgr) const noexcept {
  // Return the result of i1 << (i2 modulo N), modulo 2^N.
  const T I2 = StackMgr.pop<T>() % static_cast<T>(sizeof(T) * 8);
  const T I1 = StackMgr.pop<T>();
  StackMgr.push<T>(I1 << I2);
  return {};
}

template <typename T>
TypeI<T> Executor::runShrOp(Runtime::StackManager &StackMgr) const noexcept {
  // Return the result of i1 >> (i2 modulo N).
  // In signed case, extended with the sign bit of i1.
  // In unsigned case, extended with 0 bits.
  using UT = std::make_unsigned_t<T>;
  const UT I2 = StackMgr.pop<UT>() % static_cast<UT>(sizeof(T) * 8);
  const T I1 = StackMgr.pop<T>();
  StackMgr.push<T>(I1 >> I2);
  return {};
}

template <typename T>
TypeU<T> Executor::runRotlOp(Runtime::StackManager &StackMgr) const noexcept {
  const T I2 = StackMgr.pop<T>() % static_cast<T>(sizeof(T) * 8);
  const T I1 = StackMgr.pop<T>();
  T R;
  // Return the result of rotating i1 left by i2 bits.
  if (likely(I2 != 0)) {
    R = (I1 << I2) | (I1 >> static_cast<T>(sizeof(T) * 8 - I2));
  } else {
    R = I1;
  }
  StackMgr.push<T>(R);
  return {};
}

template <typename T>
TypeU<T> Executor::runRotrOp(Runtime::StackManager &StackMgr) const noexcept {
  const T I2 = StackMgr.pop<T>() % static_cast<T>(sizeof(T) * 8);
  const T I1 = StackMgr.pop<T>();
  T R;
  // Return the result of rotating i1 left by i2 bits.
  if (likely(I2 != 0)) {
    R = (I1 >> I2) | (I1 << static_cast<T>(sizeof(T) * 8 - I2));
  } else {
    R = I1;
  }
  StackMgr.push<T>(R);
  return {};
}

template <typename T>
TypeF<T> Executor::runMinOp(Runtime::StackManager &StackMgr) const noexcept {
  constexpr const T Zero = 0.0;
  const T Z2 = StackMgr.pop<T>();
  const T Z1 = StackMgr.pop<T>();
  T R;
  // TODO: canonical and arithmetical NaN
  if (unlikely(std::isnan(Z2))) {
    R = Z2;
  } else if (unlikely(std::isnan(Z1))) {
    R = Z1;
  } else if (Z1 == Zero && Z2 == Zero && std::signbit(Z1) != std::signbit(Z2)) {
    // If both z1 and z2 are zeroes of opposite signs, then return -0.0.
    R = -Zero;
  } else {
    // Else return the min of z1 and z2. (Inf case are handled.)
    R = std::min(Z1, Z2);
  }
  StackMgr.push<T>(R);
  return {};
}

template <typename T>
TypeF<T> Executor::runMaxOp(Runtime::StackManager &StackMgr) const noexcept {
  constexpr const T Zero = 0.0;
  const T Z2 = StackMgr.pop<T>();
  const T Z1 = StackMgr.pop<T>();
  T R;
  // TODO: canonical and arithmetical NaN
  if (unlikely(std::isnan(Z2))) {
    R = Z2;
  } else if (unlikely(std::isnan(Z1))) {
    R = Z1;
  } else if (Z1 == Zero && Z2 == Zero && std::signbit(Z1) != std::signbit(Z2)) {
    // If both z1 and z2 are zeroes of opposite signs, then return +0.0.
    R = Zero;
  } else {
    // Else return the max of z1 and z2. (Inf case are handled.)
    R = std::max(Z1, Z2);
  }
  StackMgr.push<T>(R);
  return {};
}

template <typename T>
TypeF<T>
Executor::runCopysignOp(Runtime::StackManager &StackMgr) const noexcept {
  const T Z2 = StackMgr.pop<T>();
  const T Z1 = StackMgr.pop<T>();
  // Return z1 with the same sign with z2.
  StackMgr.push<T>(std::copysign(Z1, Z2));
  return {};
}

template <typename TIn, typename TOut>
Expect<void> Executor::runReplaceLaneOp(Runtime::StackManager &StackMgr,
                                        const uint8_t Index) const noexcept {
  using VTOut [[gnu::vector_size(16)]] = TOut;
  const TIn V2 = StackMgr.pop<TIn>();
  const VTOut V1 = StackMgr.pop<VTOut>();
  VTOut R = V1;
  R[Index] = static_cast<TOut>(V2);
  StackMgr.push<VTOut>(R);
  return {};
}

template <typename T>
Expect<void>
Executor::runVectorEqOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT [[gnu::vector_size(16)]] = T;
  const VT V2 = StackMgr.pop<VT>();
  const VT V1 = StackMgr.pop<VT>();
  StackMgr.push<VT>(V1 == V2);
  return {};
}

template <typename T>
Expect<void>
Executor::runVectorNeOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT [[gnu::vector_size(16)]] = T;
  const VT V2 = StackMgr.pop<VT>();
  const VT V1 = StackMgr.pop<VT>();
  StackMgr.push<VT>(V1 != V2);
  return {};
}

template <typename T>
Expect<void>
Executor::runVectorLtOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT [[gnu::vector_size(16)]] = T;
  const VT V2 = StackMgr.pop<VT>();
  const VT V1 = StackMgr.pop<VT>();
  StackMgr.push<VT>(V1 < V2);
  return {};
}

template <typename T>
Expect<void>
Executor::runVectorGtOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT [[gnu::vector_size(16)]] = T;
  const VT V2 = StackMgr.pop<VT>();
  const VT V1 = StackMgr.pop<VT>();
  StackMgr.push<VT>(V1 > V2);
  return {};
}

template <typename T>
Expect<void>
Executor::runVectorLeOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT [[gnu::vector_size(16)]] = T;
  const VT V2 = StackMgr.pop<VT>();
  const VT V1 = StackMgr.pop<VT>();
  StackMgr.push<VT>(V1 <= V2);
  return {};
}

template <typename T>
Expect<void>
Executor::runVectorGeOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT [[gnu::vector_size(16)]] = T;
  const VT V2 = StackMgr.pop<VT>();
  const VT V1 = StackMgr.pop<VT>();
  StackMgr.push<VT>(V1 >= V2);
  return {};
}

template <typename TIn, typename TOut>
Expect<void>
Executor::runVectorNarrowOp(Runtime::StackManager &StackMgr) const noexcept {
  static_assert(sizeof(TOut) * 2 == sizeof(TIn));
  static_assert(sizeof(TOut) == 1 || sizeof(TOut) == 2);
  using VTIn [[gnu::vector_size(16)]] = TIn;
  using HVTOut [[gnu::vector_size(8)]] = TOut;
  using VTOut [[gnu::vector_size(16)]] = TOut;

  constexpr const VTIn Min =
      VTIn{} + static_cast<TIn>(std::numeric_limits<TOut>::min());
  constexpr const VTIn Max =
      VTIn{} + static_cast<TIn>(std::numeric_limits<TOut>::max());
  VTIn V2 = StackMgr.pop<VTIn>();
  VTIn V1 = StackMgr.pop<VTIn>();
  V1 = detail::vectorSelect(V1 < Min, Min, V1);
  V1 = detail::vectorSelect(V1 > Max, Max, V1);
  V2 = detail::vectorSelect(V2 < Min, Min, V2);
  V2 = detail::vectorSelect(V2 > Max, Max, V2);
  const HVTOut HV1 = __builtin_convertvector(V1, HVTOut);
  const HVTOut HV2 = __builtin_convertvector(V2, HVTOut);
  VTOut Result;
  if constexpr (sizeof(TOut) == 1) {
    Result =
        VTOut{HV1[0], HV1[1], HV1[2], HV1[3], HV1[4], HV1[5], HV1[6], HV1[7],
              HV2[0], HV2[1], HV2[2], HV2[3], HV2[4], HV2[5], HV2[6], HV2[7]};
  } else if constexpr (sizeof(TOut) == 2) {
    Result =
        VTOut{HV1[0], HV1[1], HV1[2], HV1[3], HV2[0], HV2[1], HV2[2], HV2[3]};
  }
  StackMgr.push<VTOut>(Result);
  return {};
}

template <typename T>
Expect<void>
Executor::runVectorShlOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT [[gnu::vector_size(16)]] = T;
  using U32 = uint32_t;
  const U32 I2 = StackMgr.pop<U32>() % static_cast<U32>(sizeof(T) * 8);
  const VT V1 = StackMgr.pop<VT>();
  StackMgr.push<VT>(V1 << I2);
  return {};
}

template <typename T>
Expect<void>
Executor::runVectorShrOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT [[gnu::vector_size(16)]] = T;
  using U32 = uint32_t;
  const U32 I2 = StackMgr.pop<U32>() % static_cast<U32>(sizeof(T) * 8);
  const VT V1 = StackMgr.pop<VT>();
  StackMgr.push<VT>(V1 >> I2);
  return {};
}

template <typename T>
Expect<void>
Executor::runVectorAddOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT [[gnu::vector_size(16)]] = T;
  const VT V2 = StackMgr.pop<VT>();
  const VT V1 = StackMgr.pop<VT>();
  StackMgr.push<VT>(V1 + V2);
  return {};
}

template <typename T>
Expect<void>
Executor::runVectorAddSatOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT [[gnu::vector_size(16)]] = T;
  using UVT [[gnu::vector_size(16)]] = std::make_unsigned_t<T>;
  const UVT V2 = StackMgr.pop<UVT>();
  const UVT V1 = StackMgr.pop<UVT>();
  const UVT Result = V1 + V2;
  UVT R;

  if constexpr (std::is_signed_v<T>) {
    const UVT Limit =
        (V1 >> (sizeof(T) * 8 - 1)) + std::numeric_limits<T>::max();
    const VT Over = reinterpret_cast<VT>((V1 ^ V2) | ~(V2 ^ Result));
    R = detail::vectorSelect(Over >= 0, Limit, Result);
  } else {
    R = Result | (Result < V1);
  }
  StackMgr.push<UVT>(R);
  return {};
}

template <typename T>
Expect<void>
Executor::runVectorSubOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT [[gnu::vector_size(16)]] = T;
  const VT V2 = StackMgr.pop<VT>();
  const VT V1 = StackMgr.pop<VT>();
  StackMgr.push<VT>(V1 - V2);
  return {};
}

template <typename T>
Expect<void>
Executor::runVectorSubSatOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT [[gnu::vector_size(16)]] = T;
  using UVT [[gnu::vector_size(16)]] = std::make_unsigned_t<T>;
  const UVT V2 = StackMgr.pop<UVT>();
  const UVT V1 = StackMgr.pop<UVT>();
  const UVT Result = V1 - V2;
  UVT R;

  if constexpr (std::is_signed_v<T>) {
    const UVT Limit =
        (V1 >> (sizeof(T) * 8 - 1)) + std::numeric_limits<T>::max();
    const VT Under = reinterpret_cast<VT>((V1 ^ V2) & (V1 ^ Result));
    R = detail::vectorSelect(Under < 0, Limit, Result);
  } else {
    R = Result & (Result <= V1);
  }
  StackMgr.push<UVT>(R);
  return {};
}

template <typename T>
Expect<void>
Executor::runVectorMulOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT [[gnu::vector_size(16)]] = T;
  const VT V2 = StackMgr.pop<VT>();
  const VT V1 = StackMgr.pop<VT>();
  StackMgr.push<VT>(V1 * V2);
  return {};
}

template <typename T>
Expect<void>
Executor::runVectorDivOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT [[gnu::vector_size(16)]] = T;
  const VT V2 = StackMgr.pop<VT>();
  const VT V1 = StackMgr.pop<VT>();
  StackMgr.push<VT>(V1 / V2);
  return {};
}

template <typename T>
Expect<void>
Executor::runVectorMinOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT [[gnu::vector_size(16)]] = T;
  const VT V2 = StackMgr.pop<VT>();
  const VT V1 = StackMgr.pop<VT>();
  StackMgr.push<VT>(detail::vectorSelect(V1 > V2, V2, V1));
  return {};
}

template <typename T>
Expect<void>
Executor::runVectorMaxOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT [[gnu::vector_size(16)]] = T;
  const VT V2 = StackMgr.pop<VT>();
  const VT V1 = StackMgr.pop<VT>();
  StackMgr.push<VT>(detail::vectorSelect(V1 < V2, V2, V1));
  return {};
}

template <typename T>
Expect<void>
Executor::runVectorFMinOp(Runtime::StackManager &StackMgr) const noexcept {
  static_assert(std::is_floating_point_v<T>);
  using VT [[gnu::vector_size(16)]] = T;
  const VT V2 = StackMgr.pop<VT>();
  const VT V1 = StackMgr.pop<VT>();
  VT R = reinterpret_cast<VT>(reinterpret_cast<uint64x2_t>(V1) |
                              reinterpret_cast<uint64x2_t>(V2));
  R = detail::vectorSelect(V1 < V2, V1, R);
  R = detail::vectorSelect(V1 > V2, V2, R);
  R = detail::vectorSelect(V1 == V1, R, V1);
  R = detail::vectorSelect(V2 == V2, R, V2);
  StackMgr.push<VT>(R);
  return {};
}

template <typename T>
Expect<void>
Executor::runVectorFMaxOp(Runtime::StackManager &StackMgr) const noexcept {
  using VT [[gnu::vector_size(16)]] = T;
  const VT V2 = StackMgr.pop<VT>();
  const VT V1 = StackMgr.pop<VT>();
  VT R = reinterpret_cast<VT>(reinterpret_cast<uint64x2_t>(V1) &
                              reinterpret_cast<uint64x2_t>(V2));
  R = detail::vectorSelect(V1 < V2, V2, R);
  R = detail::vectorSelect(V1 > V2, V1, R);
  R = detail::vectorSelect(V1 == V1, R, V1);
  R = detail::vectorSelect(V2 == V2, R, V2);
  StackMgr.push<VT>(R);
  return {};
}

template <typename T, typename ET>
Expect<void>
Executor::runVectorAvgrOp(Runtime::StackManager &StackMgr) const noexcept {
  static_assert(sizeof(T) * 2 == sizeof(ET));
  using VT [[gnu::vector_size(16)]] = T;
  using EVT [[gnu::vector_size(32)]] = ET;
  const VT V2 = StackMgr.pop<VT>();
  const VT V1 = StackMgr.pop<VT>();
  const EVT EV1 = __builtin_convertvector(V1, EVT);
  const EVT EV2 = __builtin_convertvector(V2, EVT);
  // Add 1 for rounding up .5
  VT R = __builtin_convertvector((EV1 + EV2 + 1) / 2, VT);
  StackMgr.push<VT>(R);
  return {};
}

template <typename TIn, typename TOut>
Expect<void>
Executor::runVectorExtMulLowOp(Runtime::StackManager &StackMgr) const noexcept {
  static_assert(sizeof(TIn) * 2 == sizeof(TOut));
  static_assert(sizeof(TIn) == 1 || sizeof(TIn) == 2 || sizeof(TIn) == 4);
  using VTIn [[gnu::vector_size(16)]] = TIn;
  using HVTIn [[gnu::vector_size(8)]] = TIn;
  using VTOut [[gnu::vector_size(16)]] = TOut;
  const VTIn V2 = StackMgr.pop<VTIn>();
  const VTIn V1 = StackMgr.pop<VTIn>();
  VTOut R;
  if constexpr (sizeof(TIn) == 1) {
    const VTOut E1 = __builtin_convertvector(
        HVTIn{V1[0], V1[1], V1[2], V1[3], V1[4], V1[5], V1[6], V1[7]}, VTOut);
    const VTOut E2 = __builtin_convertvector(
        HVTIn{V2[0], V2[1], V2[2], V2[3], V2[4], V2[5], V2[6], V2[7]}, VTOut);
    R = E1 * E2;
  } else if constexpr (sizeof(TIn) == 2) {
    const VTOut E1 =
        __builtin_convertvector(HVTIn{V1[0], V1[1], V1[2], V1[3]}, VTOut);
    const VTOut E2 =
        __builtin_convertvector(HVTIn{V2[0], V2[1], V2[2], V2[3]}, VTOut);
    R = E1 * E2;
  } else if constexpr (sizeof(TIn) == 4) {
    const VTOut E1 = __builtin_convertvector(HVTIn{V1[0], V1[1]}, VTOut);
    const VTOut E2 = __builtin_convertvector(HVTIn{V2[0], V2[1]}, VTOut);
    R = E1 * E2;
  }
  StackMgr.push<VTOut>(R);
  return {};
}

template <typename TIn, typename TOut>
Expect<void> Executor::runVectorExtMulHighOp(
    Runtime::StackManager &StackMgr) const noexcept {
  static_assert(sizeof(TIn) * 2 == sizeof(TOut));
  static_assert(sizeof(TIn) == 1 || sizeof(TIn) == 2 || sizeof(TIn) == 4);
  using VTIn [[gnu::vector_size(16)]] = TIn;
  using HVTIn [[gnu::vector_size(8)]] = TIn;
  using VTOut [[gnu::vector_size(16)]] = TOut;
  const VTIn V2 = StackMgr.pop<VTIn>();
  const VTIn V1 = StackMgr.pop<VTIn>();
  VTOut R;
  if constexpr (sizeof(TIn) == 1) {
    const VTOut E1 = __builtin_convertvector(
        HVTIn{V1[8], V1[9], V1[10], V1[11], V1[12], V1[13], V1[14], V1[15]},
        VTOut);
    const VTOut E2 = __builtin_convertvector(
        HVTIn{V2[8], V2[9], V2[10], V2[11], V2[12], V2[13], V2[14], V2[15]},
        VTOut);
    R = E1 * E2;
  } else if constexpr (sizeof(TIn) == 2) {
    const VTOut E1 =
        __builtin_convertvector(HVTIn{V1[4], V1[5], V1[6], V1[7]}, VTOut);
    const VTOut E2 =
        __builtin_convertvector(HVTIn{V2[4], V2[5], V2[6], V2[7]}, VTOut);
    R = E1 * E2;
  } else if constexpr (sizeof(TIn) == 4) {
    const VTOut E1 = __builtin_convertvector(HVTIn{V1[2], V1[3]}, VTOut);
    const VTOut E2 = __builtin_convertvector(HVTIn{V2[2], V2[3]}, VTOut);
    R = E1 * E2;
  }
  StackMgr.push<VTOut>(R);
  return {};
}

inline Expect<void>
Executor::runVectorQ15MulSatOp(Runtime::StackManager &StackMgr) const noexcept {
  using int32x8_t [[gnu::vector_size(32)]] = int32_t;
  const auto V2 = StackMgr.pop<int16x8_t>();
  const auto V1 = StackMgr.pop<int16x8_t>();
  const auto EV1 = __builtin_convertvector(V1, int32x8_t);
  const auto EV2 = __builtin_convertvector(V2, int32x8_t);
  const auto ER = (EV1 * EV2 + INT32_C(0x4000)) >> INT32_C(15);
  const int32x8_t Cap = int32x8_t{} + INT32_C(0x7fff);
  const auto ERSat = detail::vectorSelect(ER > Cap, Cap, ER);
  StackMgr.push<int16x8_t>(__builtin_convertvector(ERSat, int16x8_t));
  return {};
}

} // namespace Executor
} // namespace WasmEdge
