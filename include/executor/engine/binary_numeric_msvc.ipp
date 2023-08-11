// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

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
  using VTOut = SIMDArray<TOut, 16>;
  VTOut &Result = Val1.get<VTOut>();
  Result[Index] = static_cast<TOut>(Val2.get<TIn>());
  return {};
}

template <typename T>
Expect<void> Executor::runVectorEqOp(ValVariant &Val1,
                                     const ValVariant &Val2) const {
  using VT = SIMDArray<T, 16>;

  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();

  uint64_t IAllOnes = ~UINT64_C(0);
  const T AllOnes = reinterpret_cast<T&>(IAllOnes);

  // unrolling V1 = (V1 == V2);
  VT VOut;
  for(size_t I = 0; I < (16 / sizeof(T)); I++) {
    if (V1[I] == V2[I]) {
      // all ones
      VOut[I] = AllOnes;
    } else {
      VOut[I] = 0;
    }
  }
  Val1.emplace<VT>(VOut);

  return {};
}

template <typename T>
Expect<void> Executor::runVectorNeOp(ValVariant &Val1,
                                     const ValVariant &Val2) const {
  using VT = SIMDArray<T, 16>;

  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();

  uint64_t IAllOnes = ~UINT64_C(0);
  const T AllOnes = reinterpret_cast<T&>(IAllOnes);

  // unrolling V1 = (V1 != V2);
  VT VOut;
  for(size_t I = 0; I < (16 / sizeof(T)); I++) {
    if (V1[I] != V2[I]) {
      // all ones
      VOut[I] = AllOnes;
    } else {
      VOut[I] = 0;
    }
  }
  Val1.emplace<VT>(VOut);

  return {};
}

template <typename T>
Expect<void> Executor::runVectorLtOp(ValVariant &Val1,
                                     const ValVariant &Val2) const {
  using VT = SIMDArray<T, 16>;

  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();

  uint64_t IAllOnes = ~UINT64_C(0);
  const T AllOnes = reinterpret_cast<T&>(IAllOnes);

  // unrolling V1 = (V1 < V2);
  VT VOut;
  for(size_t I = 0; I < (16 / sizeof(T)); I++) {
    if (V1[I] < V2[I]) {
      // all ones
      VOut[I] = AllOnes;
    } else {
      VOut[I] = 0;
    }
  }
  Val1.emplace<VT>(VOut);

  return {};
}

template <typename T>
Expect<void> Executor::runVectorGtOp(ValVariant &Val1,
                                     const ValVariant &Val2) const {
  using VT = SIMDArray<T, 16>;

  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();

  uint64_t IAllOnes = ~UINT64_C(0);
  const T AllOnes = reinterpret_cast<T&>(IAllOnes);

  // unrolling V1 = (V1 > V2);
  VT VOut;
  for(size_t I = 0; I < (16 / sizeof(T)); I++) {
    if (V1[I] > V2[I]) {
      // all ones
      VOut[I] = AllOnes;
    } else {
      VOut[I] = 0;
    }
  }
  Val1.emplace<VT>(VOut);

  return {};
}

template <typename T>
Expect<void> Executor::runVectorLeOp(ValVariant &Val1,
                                     const ValVariant &Val2) const {
  using VT = SIMDArray<T, 16>;

  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();

  uint64_t IAllOnes = ~UINT64_C(0);
  const T AllOnes = reinterpret_cast<T&>(IAllOnes);

  // unrolling V1 = (V1 <= V2);
  VT VOut;
  for(size_t I = 0; I < (16 / sizeof(T)); I++) {
    if (V1[I] <= V2[I]) {
      // all ones
      VOut[I] = AllOnes;
    } else {
      VOut[I] = 0;
    }
  }
  Val1.emplace<VT>(VOut);

  return {};
}

template <typename T>
Expect<void> Executor::runVectorGeOp(ValVariant &Val1,
                                     const ValVariant &Val2) const {
  using VT = SIMDArray<T, 16>;

  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();

  uint64_t IAllOnes = ~UINT64_C(0);
  const T AllOnes = reinterpret_cast<T&>(IAllOnes);

  // unrolling V1 = (V1 >= V2);
  VT VOut;
  for(size_t I = 0; I < (16 / sizeof(T)); I++) {
    if (V1[I] >= V2[I]) {
      // all ones
      VOut[I] = AllOnes;
    } else {
      VOut[I] = 0;
    }
  }
  Val1.emplace<VT>(VOut);

  return {};
}

template <typename TIn, typename TOut>
Expect<void> Executor::runVectorNarrowOp(ValVariant &Val1,
                                         const ValVariant &Val2) const {
  static_assert(sizeof(TOut) * 2 == sizeof(TIn));
  static_assert(sizeof(TOut) == 1 || sizeof(TOut) == 2);
  using VTIn = SIMDArray<TIn, 16>;
  using HVTOut = SIMDArray<TOut, 8>;
  using VTOut = SIMDArray<TOut, 16>;
  constexpr size_t HSize = (8 / sizeof(TOut));

  VTOut Result;
  VTIn V1 = Val1.get<VTIn>();
  for(size_t I = 0; I < HSize; ++I) {
    if (V1[I] > std::numeric_limits<TOut>::max()) {
      Result[I] = std::numeric_limits<TOut>::max();
    } else if (V1[I] < std::numeric_limits<TOut>::min()) {
      Result[I] = std::numeric_limits<TOut>::min();
    } else {
      Result[I] = static_cast<TOut>(V1[I]);
    }
  }
  const VTIn &V2 = Val2.get<VTIn>();
  for(size_t I = 0; I < HSize; ++I) {
    if (V2[I] > std::numeric_limits<TOut>::max()) {
      Result[HSize + I] = std::numeric_limits<TOut>::max();
    } else if (V2[I] < std::numeric_limits<TOut>::min()) {
      Result[HSize + I] = std::numeric_limits<TOut>::min();
    } else {
      Result[HSize + I] = static_cast<TOut>(V2[I]);
    }
  }
  Val1.emplace<VTOut>(Result);
  return {};
}

template <typename T>
Expect<void> Executor::runVectorShlOp(ValVariant &Val1,
                                      const ValVariant &Val2) const {
  using VT = SIMDArray<T, 16>;
  const uint32_t Mask = static_cast<uint32_t>(sizeof(T) * 8 - 1);
  const uint32_t Count = Val2.get<uint32_t>() & Mask;
  VT &V1 = Val1.get<VT>();
  for(size_t I = 0; I < (16 / sizeof(T)); ++I) {
    V1[I] <<= Count;
  }

  return {};
}

template <typename T>
Expect<void> Executor::runVectorShrOp(ValVariant &Val1,
                                      const ValVariant &Val2) const {
  using VT = SIMDArray<T, 16>;
  const uint32_t Mask = static_cast<uint32_t>(sizeof(T) * 8 - 1);
  const uint32_t Count = Val2.get<uint32_t>() & Mask;
  VT &V1 = Val1.get<VT>();
  for(size_t I = 0; I < (16 / sizeof(T)); ++I) {
    V1[I] >>= Count;
  }

  return {};
}

template <typename T>
Expect<void> Executor::runVectorAddOp(ValVariant &Val1,
                                      const ValVariant &Val2) const {
  using VT = SIMDArray<T, 16>;
  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();
  for (size_t I = 0; I < (16 / sizeof(T)); ++I) {
    V1[I] += V2[I];
  }

  return {};
}

template <typename T>
Expect<void> Executor::runVectorAddSatOp(ValVariant &Val1,
                                         const ValVariant &Val2) const {
  // use int32 to check for overflow.
  static_assert(sizeof(T) < 4);
  using VT = SIMDArray<T, 16>;
  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();

  for (size_t I = 0; I < (16 / sizeof(T)); ++I) {
    int32_t Result = ((int32_t)V1[I]) + ((int32_t)V2[I]);
    if (Result > std::numeric_limits<T>::max()) {
      V1[I] = std::numeric_limits<T>::max();
    } else if (Result < std::numeric_limits<T>::min()) {
      V1[I] = std::numeric_limits<T>::min();
    } else {
      V1[I] = static_cast<T>(Result);
    }
  }
  return {};
}

template <typename T>
Expect<void> Executor::runVectorSubOp(ValVariant &Val1,
                                      const ValVariant &Val2) const {
  using VT = SIMDArray<T, 16>;
  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();
  for (size_t I = 0; I < (16 / sizeof(T)); ++I) {
    V1[I] -= V2[I];
  }

  return {};
}

template <typename T>
Expect<void> Executor::runVectorSubSatOp(ValVariant &Val1,
                                         const ValVariant &Val2) const {
  // use int32 to check for overflow.
  static_assert(sizeof(T) < 4);
  using VT = SIMDArray<T, 16>;
  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();

  for (size_t I = 0; I < (16 / sizeof(T)); ++I) {
    int32_t Result = ((int32_t)V1[I]) - ((int32_t)V2[I]);
    if (Result > std::numeric_limits<T>::max()) {
      V1[I] = std::numeric_limits<T>::max();
    } else if (Result < std::numeric_limits<T>::min()) {
      V1[I] = std::numeric_limits<T>::min();
    } else {
      V1[I] = static_cast<T>(Result);
    }
  }
  return {};
}

template <typename T>
Expect<void> Executor::runVectorMulOp(ValVariant &Val1,
                                      const ValVariant &Val2) const {
  using VT = SIMDArray<T, 16>;
  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();
  for (size_t I = 0; I < (16 / sizeof(T)); ++I) {
    V1[I] *= V2[I];
  }

  return {};
}

template <typename T>
Expect<void> Executor::runVectorDivOp(ValVariant &Val1,
                                      const ValVariant &Val2) const {
  using VT = SIMDArray<T, 16>;
  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();
  for (size_t I = 0; I < (16 / sizeof(T)); ++I) {
    V1[I] /= V2[I];
  }

  return {};
}

template <typename T>
Expect<void> Executor::runVectorMinOp(ValVariant &Val1,
                                      const ValVariant &Val2) const {
  using VT = SIMDArray<T, 16>;
  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();
  for (size_t I = 0; I < (16 / sizeof(T)); ++I) {
    V1[I] = V1[I] > V2[I] ? V2[I] : V1[I];
  }

  return {};
}

template <typename T>
Expect<void> Executor::runVectorMaxOp(ValVariant &Val1,
                                      const ValVariant &Val2) const {
  using VT = SIMDArray<T, 16>;
  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();
  for (size_t I = 0; I < (16 / sizeof(T)); ++I) {
    V1[I] = V1[I] < V2[I] ? V2[I] : V1[I];
  }

  return {};
}

template <typename T>
Expect<void> Executor::runVectorFMinOp(ValVariant &Val1,
                                       const ValVariant &Val2) const {
  static_assert(std::is_floating_point_v<T>);
  using VT = SIMDArray<T, 16>;
  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();
  for (size_t I = 0; I < (16 / sizeof(T)); ++I) {
    if (V1[I] > V2[I]) {
      V1[I] = V2[I];
    } else if (V1[I] < V2[I]) {
      // do nothing
    } else if (std::isnan(V2[I]) && !std::isnan(V1[I])) {
      V1[I] = V2[I];
    } else if (V1[I] == ((T)0.0)) {
      // prefer negative zero
      if (std::signbit(V2[I]) && !std::signbit(V1[I])) {
        V1[I] = V2[I];
      }
    }
  }

  return {};
}

template <typename T>
Expect<void> Executor::runVectorFMaxOp(ValVariant &Val1,
                                       const ValVariant &Val2) const {
  using VT = SIMDArray<T, 16>;
  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();
  for (size_t I = 0; I < (16 / sizeof(T)); ++I) {
    if (V1[I] < V2[I]) {
      V1[I] = V2[I];
    } else if (V1[I] > V2[I]) {
      // do nothing
    } else if (std::isnan(V2[I]) && !std::isnan(V1[I])) {
      V1[I] = V2[I];
    } else if (V1[I] == ((T)0.0)) {
      // prefer positive zero
      if (!std::signbit(V2[I]) && std::signbit(V1[I])) {
        V1[I] = V2[I];
      }
    }
  }

  return {};
}

template <typename T, typename ET>
Expect<void> Executor::runVectorAvgrOp(ValVariant &Val1,
                                       const ValVariant &Val2) const {
  static_assert(sizeof(T) * 2 == sizeof(ET));
  using VT = SIMDArray<T, 16>;
  VT &V1 = Val1.get<VT>();
  const VT &V2 = Val2.get<VT>();
  for (size_t I = 0; I < (16 / sizeof(T)); ++I) {
    // Add 1 for rounding up .5
    V1[I] = (((ET)V1[I]) + ((ET)V2[I]) + 1) / 2;
  }

  return {};
}

template <typename TIn, typename TOut>
Expect<void> Executor::runVectorExtMulLowOp(ValVariant &Val1,
                                            const ValVariant &Val2) const {
  static_assert(sizeof(TIn) * 2 == sizeof(TOut));
  static_assert(sizeof(TIn) == 1 || sizeof(TIn) == 2 || sizeof(TIn) == 4);
  using VTIn = SIMDArray<TIn, 16>;
  using VTOut = SIMDArray<TOut, 16>;
  const VTIn &V1 = Val1.get<VTIn>();
  const VTIn &V2 = Val2.get<VTIn>();
  VTOut Result;
  for (size_t I = 0; I < (8 / sizeof(TIn)); ++I) {
    Result[I] = ((TOut)V1[I]) * ((TOut)V2[I]);
  }
  Val1.emplace<VTOut>(Result);
  return {};
}

template <typename TIn, typename TOut>
Expect<void> Executor::runVectorExtMulHighOp(ValVariant &Val1,
                                             const ValVariant &Val2) const {
  static_assert(sizeof(TIn) * 2 == sizeof(TOut));
  static_assert(sizeof(TIn) == 1 || sizeof(TIn) == 2 || sizeof(TIn) == 4);
  using VTIn = SIMDArray<TIn, 16>;
  using VTOut = SIMDArray<TOut, 16>;
  constexpr size_t HSize = (8 / sizeof(TIn));
  const VTIn &V1 = Val1.get<VTIn>();
  const VTIn &V2 = Val2.get<VTIn>();
  VTOut Result;
  for (size_t I = 0; I < HSize; ++I) {
    Result[I] = ((TOut)V1[HSize + I]) * ((TOut)V2[HSize + I]);
  }
  Val1.emplace<VTOut>(Result);
  return {};
}

inline Expect<void>
Executor::runVectorQ15MulSatOp(ValVariant &Val1, const ValVariant &Val2) const {
  using int32x8_t = SIMDArray<int32_t, 32>;
  const auto &V1 = Val1.get<int16x8_t>();
  const auto &V2 = Val2.get<int16x8_t>();
  int16x8_t VOut;
  int32x8_t ER;
  for(size_t I = 0; I < 8; I++) {
    ER[I] = (static_cast<int32_t>(V1[I]) * static_cast<int32_t>(V2[I]) + INT32_C(0x4000)) >> INT32_C(15);
    if (ER[I] > 0x7fff) {
      ER[I] = 0x7fff;
    }
    VOut[I] = static_cast<int16_t>(ER[I]);
  }
  Val1.emplace<int16x8_t>(VOut);
  return {};
}

} // namespace Executor
} // namespace WasmEdge
