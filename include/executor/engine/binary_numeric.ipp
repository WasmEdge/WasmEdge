// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

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

} // namespace Executor
} // namespace WasmEdge

#if defined(_MSC_VER) && !defined(__clang__) // MSVC
#include "executor/engine/binary_numeric_vector_msvc.ipp"
#else
#include "executor/engine/binary_numeric_vector.ipp"
#endif // MSVC
