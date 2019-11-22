#include "executor/common.h"
#include "executor/entry/value.h"
#include "executor/worker.h"
#include "executor/worker/util.h"
#include "support/casting.h"
#include <cmath>

namespace SSVM {
namespace Executor {

template <typename T>
TypeB<T, ErrCode> Worker::runAddOp(const Value &Val1, const Value &Val2) {
  T V1 = retrieveValue<T>(Val1), V2 = retrieveValue<T>(Val2);
  /// Integer case: Return the result of (v1 + v2) modulo 2^N.
  /// Floating case: NaN, inf, and zeros are handled.
  return StackMgr.pushValue(V1 + V2);
}

template <typename T>
TypeB<T, ErrCode> Worker::runSubOp(const Value &Val1, const Value &Val2) {
  T V1 = retrieveValue<T>(Val1), V2 = retrieveValue<T>(Val2);
  /// Integer case: Return the result of (v1 - v2) modulo 2^N.
  /// Floating case: NaN, inf, and zeros are handled.
  return StackMgr.pushValue(V1 - V2);
}

template <typename T>
TypeB<T, ErrCode> Worker::runMulOp(const Value &Val1, const Value &Val2) {
  T V1 = retrieveValue<T>(Val1), V2 = retrieveValue<T>(Val2);
  /// Integer case: Return the result of (v1 * v2) modulo 2^N.
  /// Floating case: NaN, inf, and zeros are handled.
  return StackMgr.pushValue(V1 * V2);
}

template <typename T>
TypeT<T, ErrCode> Worker::runDivOp(const Value &Val1, const Value &Val2) {
  T V1 = retrieveValue<T>(Val1), V2 = retrieveValue<T>(Val2);
  if (!std::is_floating_point_v<T> && V2 == 0) {
    /// Integer case: If v2 is 0, then the result is undefined.
    return ErrCode::DivideByZero;
  }
  if (std::is_signed_v<T> && V1 == std::numeric_limits<T>::min() && V2 == -1) {
    /// Signed Integer case: If signed(v1) / signed(v2) is 2^(N − 1), then the
    /// result is undefined.
    return ErrCode::FloatPointException;
  }
  /// Else, return the result of v1 / v2.
  /// Integer case: truncated toward zero.
  /// Floating case: +-0.0, NaN, and Inf case are handled.
  return StackMgr.pushValue(Support::toUnsigned(V1 / V2));
}

template <typename T>
TypeI<T, ErrCode> Worker::runRemOp(const Value &Val1, const Value &Val2) {
  T I1 = retrieveValue<T>(Val1), I2 = retrieveValue<T>(Val2);
  /// If i2 is 0, then the result is undefined.
  if (I2 == 0) {
    return ErrCode::DivideByZero;
  }
  /// Else, return the i1 % i2. Signed case is handled.
  if (std::is_signed_v<T> && I2 == -1) {
    /// Signed Integer case: If signed(v2) is -1, then the result 0.
    return StackMgr.pushValue(Support::toUnsigned((T)0U));
  }
  return StackMgr.pushValue(Support::toUnsigned(I1 % I2));
}

template <typename T>
TypeU<T, ErrCode> Worker::runAndOp(const Value &Val1, const Value &Val2) {
  T I1 = retrieveValue<T>(Val1), I2 = retrieveValue<T>(Val2);
  /// Return the bitwise conjunction of i1 and i2.
  return StackMgr.pushValue(I1 & I2);
}

template <typename T>
TypeU<T, ErrCode> Worker::runOrOp(const Value &Val1, const Value &Val2) {
  T I1 = retrieveValue<T>(Val1), I2 = retrieveValue<T>(Val2);
  /// Return the bitwise disjunction of i1 and i2.
  return StackMgr.pushValue(I1 | I2);
}

template <typename T>
TypeU<T, ErrCode> Worker::runXorOp(const Value &Val1, const Value &Val2) {
  T I1 = retrieveValue<T>(Val1), I2 = retrieveValue<T>(Val2);
  /// Return the bitwise exclusive disjunction of i1 and i2.
  return StackMgr.pushValue(I1 ^ I2);
}

template <typename T>
TypeU<T, ErrCode> Worker::runShlOp(const Value &Val1, const Value &Val2) {
  T I1 = retrieveValue<T>(Val1), I2 = retrieveValue<T>(Val2);
  /// Let k be i2 modulo N.
  I2 %= sizeof(T) * 8;
  /// Return the result of i1 << k, modulo 2^N.
  return StackMgr.pushValue(I1 << I2);
}

template <typename T>
TypeI<T, ErrCode> Worker::runShrOp(const Value &Val1, const Value &Val2) {
  T I1 = retrieveValue<T>(Val1), I2 = retrieveValue<T>(Val2);
  /// Let k be i2 modulo N.
  I2 %= sizeof(T) * 8;
  /// Return the result of i1 >> k.
  /// In signed case, extended with the sign bit of i1.
  /// In unsigned case, extended with 0 bits.
  return StackMgr.pushValue(Support::toUnsigned(I1 >> I2));
}

template <typename T>
TypeU<T, ErrCode> Worker::runRotlOp(const Value &Val1, const Value &Val2) {
  T I1 = retrieveValue<T>(Val1), I2 = retrieveValue<T>(Val2);
  /// Let k be i2 modulo N.
  I2 %= sizeof(T) * 8;
  /// Return the result of rotating i1 left by k bits.
  return StackMgr.pushValue((I1 << I2) | (I1 >> (sizeof(T) * 8 - I2)));
}

template <typename T>
TypeU<T, ErrCode> Worker::runRotrOp(const Value &Val1, const Value &Val2) {
  T I1 = retrieveValue<T>(Val1), I2 = retrieveValue<T>(Val2);
  /// Let k be i2 modulo N.
  I2 %= sizeof(T) * 8;
  /// Return the result of rotating i1 left by k bits.
  return StackMgr.pushValue((I1 >> I2) | (I1 << (sizeof(T) * 8 - I2)));
}

template <typename T>
TypeF<T, ErrCode> Worker::runMinOp(const Value &Val1, const Value &Val2) {
  T Z1 = retrieveValue<T>(Val1), Z2 = retrieveValue<T>(Val2);
  /// If both z1 and z2 are zeroes of opposite signs, then return negative zero.
  if (Z1 == 0.0 && Z2 == 0.0 && std::signbit(Z1) != std::signbit(Z2))
    return StackMgr.pushValue((T)-0.0);
  /// Else return the min of z1 and z2. (NaN and Inf case are handled.)
  return StackMgr.pushValue(std::min(Z1, Z2));
}

template <typename T>
TypeF<T, ErrCode> Worker::runMaxOp(const Value &Val1, const Value &Val2) {
  T Z1 = retrieveValue<T>(Val1), Z2 = retrieveValue<T>(Val2);
  /// Return the min of z1 and z2. (+-0.0, NaN, and Inf case are handled.)
  return StackMgr.pushValue(std::max(Z1, Z2));
}

template <typename T>
TypeF<T, ErrCode> Worker::runCopysignOp(const Value &Val1, const Value &Val2) {
  T Z1 = retrieveValue<T>(Val1), Z2 = retrieveValue<T>(Val2);
  /// Return z1 with the same sign with z2.
  return StackMgr.pushValue(std::copysign(Z1, Z2));
}

} // namespace Executor
} // namespace SSVM
