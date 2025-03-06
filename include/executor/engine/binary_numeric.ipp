// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/executor.h"

#include <cmath>

namespace WasmEdge {
namespace Executor {

template <typename T>
TypeN<T> Executor::runAddOp(Runtime::StackManager &StackMgr) const noexcept {
  const auto [Val2, Val1] = StackMgr.popsPeekTop<T, T>();
  // Integer case: Return the result of (v1 + v2) modulo 2^N.
  // Floating case: NaN, inf, and zeros are handled.
  StackMgr.emplaceTop(Val1 + Val2);
  return {};
}

template <typename T>
TypeN<T> Executor::runSubOp(Runtime::StackManager &StackMgr) const noexcept {
  const auto [Val2, Val1] = StackMgr.popsPeekTop<T, T>();
  // Integer case: Return the result of (v1 - v2) modulo 2^N.
  // Floating case: NaN, inf, and zeros are handled.
  StackMgr.emplaceTop(Val1 - Val2);
  return {};
}

template <typename T>
TypeN<T> Executor::runMulOp(Runtime::StackManager &StackMgr) const noexcept {
  const auto [Val2, Val1] = StackMgr.popsPeekTop<T, T>();
  // Integer case: Return the result of (v1 * v2) modulo 2^N.
  // Floating case: NaN, inf, and zeros are handled.
  StackMgr.emplaceTop(Val1 * Val2);
  return {};
}

template <typename T>
TypeT<T> Executor::runDivOp(Runtime::StackManager &StackMgr,
                            const AST::Instruction &Instr) const noexcept {
  const auto [Val2, Val1] = StackMgr.popsPeekTop<T, T>();
  if constexpr (!std::is_floating_point_v<T>) {
    if (Val2 == 0) {
      // Integer case: If v2 is 0, then the result is undefined.
      spdlog::error(ErrCode::Value::DivideByZero);
      spdlog::error(ErrInfo::InfoInstruction(
          Instr.getOpCode(), Instr.getOffset(), {Val1, Val2},
          {ValTypeFromType<T>(), ValTypeFromType<T>()}, std::is_signed_v<T>));
      return Unexpect(ErrCode::Value::DivideByZero);
    }
    if (std::is_signed_v<T> && Val1 == std::numeric_limits<T>::min() &&
        Val2 == static_cast<T>(-1)) {
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
  StackMgr.emplaceTop<T>(Val1 / Val2);
  return {};
}

template <typename T>
TypeI<T> Executor::runRemOp(Runtime::StackManager &StackMgr,
                            const AST::Instruction &Instr) const noexcept {
  auto [Val2, Val1] = StackMgr.popsPeekTop<T, T>();
  // If i2 is 0, then the result is undefined.
  if (Val2 == 0) {
    spdlog::error(ErrCode::Value::DivideByZero);
    spdlog::error(ErrInfo::InfoInstruction(
        Instr.getOpCode(), Instr.getOffset(), {Val1, Val2},
        {ValTypeFromType<T>(), ValTypeFromType<T>()}, std::is_signed_v<T>));
    return Unexpect(ErrCode::Value::DivideByZero);
  }
  // Else, return the i1 % i2. Signed case is handled.
  if (std::is_signed_v<T> && Val2 == static_cast<T>(-1)) {
    // Signed Integer case: If signed(v2) is -1, then the result 0.
    T R = 0;
    StackMgr.emplaceTop(R);
  } else {
    T R = Val1 % Val2;
    StackMgr.emplaceTop(R);
  }
  return {};
}

template <typename T>
TypeU<T> Executor::runAndOp(Runtime::StackManager &StackMgr) const noexcept {
  const auto [Val2, Val1] = StackMgr.popsPeekTop<T, T>();
  // Return the bitwise conjunction of i1 and i2.
  StackMgr.emplaceTop(Val1 & Val2);
  return {};
}

template <typename T>
TypeU<T> Executor::runOrOp(Runtime::StackManager &StackMgr) const noexcept {
  const auto [Val2, Val1] = StackMgr.popsPeekTop<T, T>();
  // Return the bitwise disjunction of i1 and i2.
  StackMgr.emplaceTop(Val1 | Val2);
  return {};
}

template <typename T>
TypeU<T> Executor::runXorOp(Runtime::StackManager &StackMgr) const noexcept {
  const auto [Val2, Val1] = StackMgr.popsPeekTop<T, T>();
  // Return the bitwise exclusive disjunction of i1 and i2.
  StackMgr.emplaceTop(Val1 ^ Val2);
  return {};
}

template <typename T>
TypeU<T> Executor::runShlOp(Runtime::StackManager &StackMgr) const noexcept {
  const auto [Val2, Val1] = StackMgr.popsPeekTop<T, T>();
  // Return the result of i1 << (i2 modulo N), modulo 2^N.
  const T Shift = Val2 % static_cast<T>(sizeof(T) * 8);
  StackMgr.emplaceTop(Val1 << Shift);
  return {};
}

template <typename T>
TypeI<T> Executor::runShrOp(Runtime::StackManager &StackMgr) const noexcept {
  // Return the result of i1 >> (i2 modulo N).
  // In signed case, extended with the sign bit of i1.
  // In unsigned case, extended with 0 bits.
  using UT = std::make_unsigned_t<T>;
  const auto [Val2, Val1] = StackMgr.popsPeekTop<UT, T>();
  const UT Shift = Val2 % static_cast<T>(sizeof(T) * 8);
  StackMgr.emplaceTop(Val1 >> Shift);
  return {};
}

template <typename T>
TypeU<T> Executor::runRotlOp(Runtime::StackManager &StackMgr) const noexcept {
  const auto [Val2, Val1] = StackMgr.popsPeekTop<T, T>();
  // Let Shift be Val2 modulo N.
  const T Shift = Val2 % static_cast<T>(sizeof(T) * 8);
  const T ReverseShift = static_cast<T>(sizeof(T) * 8) - Shift;
  // Return the result of rotating i1 left by Shift bits.
  if (likely(Shift != 0)) {
    StackMgr.emplaceTop((Val1 << Shift) | (Val1 >> ReverseShift));
  }
  return {};
}

template <typename T>
TypeU<T> Executor::runRotrOp(Runtime::StackManager &StackMgr) const noexcept {
  const auto [Val2, Val1] = StackMgr.popsPeekTop<T, T>();
  // Let Shift be i2 modulo N.
  const T Shift = Val2 % static_cast<T>(sizeof(T) * 8);
  const T ReverseShift = static_cast<T>(sizeof(T) * 8) - Shift;
  // Return the result of rotating i1 right by Shift bits.
  if (likely(Shift != 0)) {
    StackMgr.emplaceTop((Val1 >> Shift) | (Val1 << ReverseShift));
  }
  return {};
}

template <typename T>
TypeF<T> Executor::runMinOp(Runtime::StackManager &StackMgr) const noexcept {
  const auto [Val2, Val1] = StackMgr.popsPeekTop<T, T>();
  const T kZero = 0.0;
  if (unlikely(std::isnan(Val1) || std::isnan(Val2))) {
    T Result = Val1;
    if (!std::isnan(Val1)) {
      Result = Val2;
    }
    // Set the most significant bit of the payload to 1.
    if constexpr (sizeof(T) == sizeof(uint32_t)) {
      uint32_t I32;
      std::memcpy(&I32, &Result, sizeof(T));
      I32 |= static_cast<uint32_t>(0x01U) << 22;
      std::memcpy(&Result, &I32, sizeof(T));
    } else if constexpr (sizeof(T) == sizeof(uint64_t)) {
      uint64_t I64;
      std::memcpy(&I64, &Result, sizeof(T));
      I64 |= static_cast<uint64_t>(0x01U) << 51;
      std::memcpy(&Result, &I64, sizeof(T));
    }
    StackMgr.emplaceTop(Result);
  } else if (Val1 == kZero && Val2 == kZero &&
             std::signbit(Val1) != std::signbit(Val2)) {
    // If both z1 and z2 are zeroes of opposite signs, then return -0.0.
    StackMgr.emplaceTop(-kZero);
  } else {
    // Else return the min of z1 and z2. (Inf case are handled.)
    StackMgr.emplaceTop(std::min(Val1, Val2));
  }
  return {};
}

template <typename T>
TypeF<T> Executor::runMaxOp(Runtime::StackManager &StackMgr) const noexcept {
  const auto [Val2, Val1] = StackMgr.popsPeekTop<T, T>();
  const T kZero = 0.0;
  if (unlikely(std::isnan(Val1) || std::isnan(Val2))) {
    T Result = Val1;
    if (!std::isnan(Val1)) {
      Result = Val2;
    }
    // Set the most significant bit of the payload to 1.
    if constexpr (sizeof(T) == sizeof(uint32_t)) {
      uint32_t I32;
      std::memcpy(&I32, &Result, sizeof(T));
      I32 |= static_cast<uint32_t>(0x01U) << 22;
      std::memcpy(&Result, &I32, sizeof(T));
    } else if constexpr (sizeof(T) == sizeof(uint64_t)) {
      uint64_t I64;
      std::memcpy(&I64, &Result, sizeof(T));
      I64 |= static_cast<uint64_t>(0x01U) << 51;
      std::memcpy(&Result, &I64, sizeof(T));
    }
    StackMgr.emplaceTop(Result);
  } else if (Val1 == kZero && Val2 == kZero &&
             std::signbit(Val1) != std::signbit(Val2)) {
    // If both z1 and z2 are zeroes of opposite signs, then return +0.0.
    StackMgr.emplaceTop(kZero);
  } else {
    // Else return the max of z1 and z2. (Inf case are handled.)
    StackMgr.emplaceTop(std::max(Val1, Val2));
  }
  return {};
}

template <typename T>
TypeF<T>
Executor::runCopysignOp(Runtime::StackManager &StackMgr) const noexcept {
  const auto [Val2, Val1] = StackMgr.popsPeekTop<T, T>();
  // Return z1 with the same sign with z2.
  StackMgr.emplaceTop(std::copysign(Val1, Val2));
  return {};
}

} // namespace Executor
} // namespace WasmEdge

#if defined(_MSC_VER) && !defined(__clang__) // MSVC
#include "executor/engine/binary_numeric_vector_msvc.ipp"
#else
#include "executor/engine/binary_numeric_vector.ipp"
#endif // MSVC
