// SPDX-License-Identifier: Apache-2.0
#include "common/log.h"
#include "common/value.h"
#include "interpreter/interpreter.h"

#include <cmath>

namespace SSVM {
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
TypeT<T> Interpreter::runDivOp(const AST::BinaryNumericInstruction &Instr,
                               ValVariant &Val1, const ValVariant &Val2) const {
  T &V1 = retrieveValue<T>(Val1);
  const T &V2 = retrieveValue<T>(Val2);
  if (!std::is_floating_point_v<T>) {
    if (V2 == 0) {
      /// Integer case: If v2 is 0, then the result is undefined.
      LOG(ERROR) << ErrCode::DivideByZero;
      LOG(ERROR) << ErrInfo::InfoInstruction(
          Instr.getOpCode(), Instr.getOffset(), {Val1, Val2},
          {ValTypeFromType<T>(), ValTypeFromType<T>()}, std::is_signed_v<T>);
      return Unexpect(ErrCode::DivideByZero);
    }
    if (std::is_signed_v<T> && V1 == std::numeric_limits<T>::min() &&
        V2 == static_cast<T>(-1)) {
      /// Signed Integer case: If signed(v1) / signed(v2) is 2^(N − 1), then the
      /// result is undefined.
      LOG(ERROR) << ErrCode::IntegerOverflow;
      LOG(ERROR) << ErrInfo::InfoInstruction(
          Instr.getOpCode(), Instr.getOffset(), {Val1, Val2},
          {ValTypeFromType<T>(), ValTypeFromType<T>()}, true);
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
TypeI<T> Interpreter::runRemOp(const AST::BinaryNumericInstruction &Instr,
                               ValVariant &Val1, const ValVariant &Val2) const {
  T &I1 = retrieveValue<T>(Val1);
  const T &I2 = retrieveValue<T>(Val2);
  /// If i2 is 0, then the result is undefined.
  if (I2 == 0) {
    LOG(ERROR) << ErrCode::DivideByZero;
    LOG(ERROR) << ErrInfo::InfoInstruction(
        Instr.getOpCode(), Instr.getOffset(), {Val1, Val2},
        {ValTypeFromType<T>(), ValTypeFromType<T>()}, std::is_signed_v<T>);
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

} // namespace Interpreter
} // namespace SSVM
