#include "executor/common.h"
#include "executor/entry/value.h"
#include "executor/worker.h"
#include "executor/worker/util.h"
#include <cmath>

namespace SSVM {
namespace Executor {

template <typename T>
ErrCode Worker::runTAddOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  T I1 = retrieveValue<T>(*Val1), I2 = retrieveValue<T>(*Val2);
  /// Integer case: Return the result of (i1 + i2) modulo 2^N.
  /// Floating case: NaN, inf, and zeros are handled.
  return StackMgr.pushValue(I1 + I2);
}

template <typename T>
ErrCode Worker::runTSubOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  T I1 = retrieveValue<T>(*Val1), I2 = retrieveValue<T>(*Val2);
  /// Integer case: Return the result of (i1 - i2) modulo 2^N.
  /// Floating case: NaN, inf, and zeros are handled.
  return StackMgr.pushValue(I1 - I2);
}

template <typename T>
ErrCode Worker::runTMulOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  T I1 = retrieveValue<T>(*Val1), I2 = retrieveValue<T>(*Val2);
  /// Integer case: Return the result of (i1 * i2) modulo 2^N.
  /// Floating case: NaN, inf, and zeros are handled.
  return StackMgr.pushValue(I1 * I2);
}

template <typename T>
ErrCode Worker::runIDivUOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  T I1 = retrieveValue<T>(*Val1), I2 = retrieveValue<T>(*Val2);
  /// If i2 is 0, then the result is undefined.
  if (I2 == 0) {
    return ErrCode::DivideByZero;
  }
  /// Else, return the result of i1 / i2, truncated toward zero.
  return StackMgr.pushValue(I1 / I2);
}

template <typename T>
ErrCode Worker::runIDivSOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  T I1 = retrieveValue<T>(*Val1), I2 = retrieveValue<T>(*Val2);
  /// If i2 is 0, then the result is undefined.
  if (I2 == 0) {
    return ErrCode::DivideByZero;
  }
  /// If signed(i1) / signed(i2) is 2^(N − 1), then the result is undefined.
  if (I1 == ((T)1U << (sizeof(T) * 8 - 1)) && I2 == (~(T)0U)) {
    return ErrCode::FloatPointException;
  }
  /// Else, return the result of signed(i1) / signed(i2), truncated toward zero.
  return StackMgr.pushValue(static_cast<T>(toSigned(I1) / toSigned(I2)));
}

template <typename T>
ErrCode Worker::runFDivOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  T Z1 = retrieveValue<T>(*Val1), Z2 = retrieveValue<T>(*Val2);
  /// Return the result of z1 / z2. (+-0.0, NaN, and Inf case are handled.)
  return StackMgr.pushValue(Z1 / Z2);
}

template <typename T>
ErrCode Worker::runIRemUOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  T I1 = retrieveValue<T>(*Val1), I2 = retrieveValue<T>(*Val2);
  /// If i2 is 0, then the result is undefined.
  if (I2 == 0) {
    return ErrCode::DivideByZero;
  }
  /// Else, return the i1 % i2.
  return StackMgr.pushValue(I1 % I2);
}

template <typename T>
ErrCode Worker::runIRemSOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  T I1 = retrieveValue<T>(*Val1), I2 = retrieveValue<T>(*Val2);
  /// If i2 is 0, then the result is undefined.
  if (I2 == 0) {
    return ErrCode::DivideByZero;
  }
  /// Else, return the signed(i1) % signed(i2).
  return StackMgr.pushValue(static_cast<T>(toSigned(I1) % toSigned(I2)));
}

template <typename T>
ErrCode Worker::runIAndOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  T I1 = retrieveValue<T>(*Val1), I2 = retrieveValue<T>(*Val2);
  /// Return the bitwise conjunction of i1 and i2.
  return StackMgr.pushValue(I1 & I2);
}

template <typename T>
ErrCode Worker::runIOrOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  T I1 = retrieveValue<T>(*Val1), I2 = retrieveValue<T>(*Val2);
  /// Return the bitwise disjunction of i1 and i2.
  return StackMgr.pushValue(I1 | I2);
}

template <typename T>
ErrCode Worker::runIXorOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  T I1 = retrieveValue<T>(*Val1), I2 = retrieveValue<T>(*Val2);
  /// Return the bitwise exclusive disjunction of i1 and i2.
  return StackMgr.pushValue(I1 ^ I2);
}

template <typename T>
ErrCode Worker::runIShlOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  T I1 = retrieveValue<T>(*Val1), I2 = retrieveValue<T>(*Val2);
  /// Let k be i2 modulo N.
  I2 %= sizeof(T) * 8;
  /// Return the result of i1 << k, modulo 2^N.
  return StackMgr.pushValue(I1 << I2);
}

template <typename T>
ErrCode Worker::runIShrUOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  T I1 = retrieveValue<T>(*Val1), I2 = retrieveValue<T>(*Val2);
  /// Let k be i2 modulo N.
  I2 %= sizeof(T) * 8;
  /// Return the result of i1 >> k, extended with 0 bits.
  return StackMgr.pushValue(I1 >> I2);
}

template <typename T>
ErrCode Worker::runIShrSOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  T I1 = retrieveValue<T>(*Val1), I2 = retrieveValue<T>(*Val2);
  /// Let k be i2 modulo N.
  I2 %= sizeof(T) * 8;
  /// Return the result of i1 >> k, extended with the sign bit of i1.
  return StackMgr.pushValue(static_cast<T>(toSigned(I1) >> I2));
}

template <typename T>
ErrCode Worker::runIRotlOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  T I1 = retrieveValue<T>(*Val1), I2 = retrieveValue<T>(*Val2);
  /// Let k be i2 modulo N.
  I2 %= sizeof(T) * 8;
  /// Return the result of rotating i1 left by k bits.
  return StackMgr.pushValue((I1 << I2) | (I1 >> (sizeof(T) * 8 - I2)));
}

template <typename T>
ErrCode Worker::runIRotrOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  T I1 = retrieveValue<T>(*Val1), I2 = retrieveValue<T>(*Val2);
  /// Let k be i2 modulo N.
  I2 %= sizeof(T) * 8;
  /// Return the result of rotating i1 left by k bits.
  return StackMgr.pushValue((I1 >> I2) | (I1 << (sizeof(T) * 8 - I2)));
}

template <typename T>
ErrCode Worker::runFMinOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  T Z1 = retrieveValue<T>(*Val1), Z2 = retrieveValue<T>(*Val2);
  /// If both z1 and z2 are zeroes of opposite signs, then return negative zero.
  if (Z1 == 0.0 && Z2 == 0.0 && std::signbit(Z1) != std::signbit(Z2))
    return StackMgr.pushValue((T)-0.0);
  /// Else return the min of z1 and z2. (NaN and Inf case are handled.)
  return StackMgr.pushValue(std::min(Z1, Z2));
}

template <typename T>
ErrCode Worker::runFMaxOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  T Z1 = retrieveValue<T>(*Val1), Z2 = retrieveValue<T>(*Val2);
  /// Return the min of z1 and z2. (+-0.0, NaN, and Inf case are handled.)
  return StackMgr.pushValue(std::max(Z1, Z2));
}

template <typename T>
ErrCode Worker::runFCopysignOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  T Z1 = retrieveValue<T>(*Val1), Z2 = retrieveValue<T>(*Val2);
  /// Return z1 with the same sign with z2.
  return StackMgr.pushValue(std::copysign(Z1, Z2));
}

} // namespace Executor
} // namespace SSVM
