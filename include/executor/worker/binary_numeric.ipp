#include "executor/common.h"
#include "executor/entry/value.h"
#include "executor/worker.h"
#include "executor/worker/util.h"
#include "support/casting.h"
#include <cmath>

namespace SSVM {
namespace Executor {

template <typename T>
TypeB<T, ErrCode> Worker::runAddOp(Value &Val1, const Value &Val2) const {
  /// Integer case: Return the result of (v1 + v2) modulo 2^N.
  /// Floating case: NaN, inf, and zeros are handled.
  retrieveValue<T>(Val1) += retrieveValue<T>(Val2);
  return ErrCode::Success;
}

template <typename T>
TypeB<T, ErrCode> Worker::runSubOp(Value &Val1, const Value &Val2) const {
  /// Integer case: Return the result of (v1 - v2) modulo 2^N.
  /// Floating case: NaN, inf, and zeros are handled.
  retrieveValue<T>(Val1) -= retrieveValue<T>(Val2);
  return ErrCode::Success;
}

template <typename T>
TypeB<T, ErrCode> Worker::runMulOp(Value &Val1, const Value &Val2) const {
  /// Integer case: Return the result of (v1 * v2) modulo 2^N.
  /// Floating case: NaN, inf, and zeros are handled.
  retrieveValue<T>(Val1) *= retrieveValue<T>(Val2);
  return ErrCode::Success;
}

template <typename T>
TypeT<T, ErrCode> Worker::runDivOp(Value &Val1, const Value &Val2) const {
  T &V1 = retrieveValue<T>(Val1);
  const T &V2 = retrieveValue<T>(Val2);
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
  V1 /= V2;
  return ErrCode::Success;
}

template <typename T>
TypeI<T, ErrCode> Worker::runRemOp(Value &Val1, const Value &Val2) const {
  T &I1 = retrieveValue<T>(Val1);
  const T &I2 = retrieveValue<T>(Val2);
  /// If i2 is 0, then the result is undefined.
  if (I2 == 0) {
    return ErrCode::DivideByZero;
  }
  /// Else, return the i1 % i2. Signed case is handled.
  if (std::is_signed_v<T> && I2 == -1) {
    /// Signed Integer case: If signed(v2) is -1, then the result 0.
    I1 = 0;
  } else {
    I1 %= I2;
  }
  return ErrCode::Success;
}

template <typename T>
TypeU<T, ErrCode> Worker::runAndOp(Value &Val1, const Value &Val2) const {
  /// Return the bitwise conjunction of i1 and i2.
  retrieveValue<T>(Val1) &= retrieveValue<T>(Val2);
  return ErrCode::Success;
}

template <typename T>
TypeU<T, ErrCode> Worker::runOrOp(Value &Val1, const Value &Val2) const {
  /// Return the bitwise disjunction of i1 and i2.
  retrieveValue<T>(Val1) |= retrieveValue<T>(Val2);
  return ErrCode::Success;
}

template <typename T>
TypeU<T, ErrCode> Worker::runXorOp(Value &Val1, const Value &Val2) const {
  /// Return the bitwise exclusive disjunction of i1 and i2.
  retrieveValue<T>(Val1) ^= retrieveValue<T>(Val2);
  return ErrCode::Success;
}

template <typename T>
TypeU<T, ErrCode> Worker::runShlOp(Value &Val1, const Value &Val2) const {
  /// Return the result of i1 << (i2 modulo N), modulo 2^N.
  retrieveValue<T>(Val1) <<= (retrieveValue<T>(Val2) % (sizeof(T) * 8));
  return ErrCode::Success;
}

template <typename T>
TypeI<T, ErrCode> Worker::runShrOp(Value &Val1, const Value &Val2) const {
  /// Return the result of i1 >> (i2 modulo N).
  /// In signed case, extended with the sign bit of i1.
  /// In unsigned case, extended with 0 bits.
  retrieveValue<T>(Val1) >>= (retrieveValue<T>(Val2) % (sizeof(T) * 8));
  return ErrCode::Success;
}

template <typename T>
TypeU<T, ErrCode> Worker::runRotlOp(Value &Val1, const Value &Val2) const {
  T &I1 = retrieveValue<T>(Val1);
  /// Let k be i2 modulo N.
  const T K = retrieveValue<T>(Val2) % (sizeof(T) * 8);
  /// Return the result of rotating i1 left by k bits.
  I1 = (I1 << K) | (I1 >> (sizeof(T) * 8 - K));
  return ErrCode::Success;
}

template <typename T>
TypeU<T, ErrCode> Worker::runRotrOp(Value &Val1, const Value &Val2) const {
  T &I1 = retrieveValue<T>(Val1);
  /// Let k be i2 modulo N.
  const T K = retrieveValue<T>(Val2) % (sizeof(T) * 8);
  /// Return the result of rotating i1 left by k bits.
  I1 = (I1 >> K) | (I1 << (sizeof(T) * 8 - K));
  return ErrCode::Success;
}

template <typename T>
TypeF<T, ErrCode> Worker::runMinOp(Value &Val1, const Value &Val2) const {
  T &Z1 = retrieveValue<T>(Val1);
  const T &Z2 = retrieveValue<T>(Val2);
  /// If both z1 and z2 are zeroes of opposite signs, then return negative zero.
  if (Z1 == 0.0 && Z2 == 0.0 && std::signbit(Z1) != std::signbit(Z2)) {
    Z1 = -0.0;
  } else {
    /// Else return the min of z1 and z2. (NaN and Inf case are handled.)
    Z1 = std::min(Z1, Z2);
  }
  return ErrCode::Success;
}

template <typename T>
TypeF<T, ErrCode> Worker::runMaxOp(Value &Val1, const Value &Val2) const {
  T &Z1 = retrieveValue<T>(Val1);
  const T &Z2 = retrieveValue<T>(Val2);
  /// Return the max of z1 and z2. (+-0.0, NaN, and Inf case are handled.)
  Z1 = std::max(Z1, Z2);
  return ErrCode::Success;
}

template <typename T>
TypeF<T, ErrCode> Worker::runCopysignOp(Value &Val1, const Value &Val2) const {
  T &Z1 = retrieveValue<T>(Val1);
  const T &Z2 = retrieveValue<T>(Val2);
  /// Return z1 with the same sign with z2.
  Z1 = std::copysign(Z1, Z2);
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
