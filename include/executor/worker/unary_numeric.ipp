// SPDX-License-Identifier: Apache-2.0
#include "executor/common.h"
#include "executor/entry/value.h"
#include "executor/worker.h"
#include "common/value.h"

#include <cmath>

namespace SSVM {
namespace Executor {

template <typename T> TypeU<T, ErrCode> Worker::runClzOp(Value &Val) const {
  T I = retrieveValue<T>(Val);
  /// Return the count of leading zero bits in i.
  if (I != 0U) {
    T Cnt = 0;
    T Mask = (T)0x1U << (sizeof(T) * 8 - 1);
    while ((I & Mask) == 0U) {
      Cnt++;
      I <<= 1;
    }
    Val = Cnt;
  }
  return ErrCode::Success;
}

template <typename T> TypeU<T, ErrCode> Worker::runCtzOp(Value &Val) const {
  T I = retrieveValue<T>(Val);
  /// Return the count of trailing zero bits in i.
  if (I != 0U) {
    T Cnt = 0;
    T Mask = (T)0x1U;
    while ((I & Mask) == 0U) {
      Cnt++;
      I >>= 1;
    }
    Val = Cnt;
  } else {
    Val = static_cast<T>(sizeof(T) * 8);
  }
  return ErrCode::Success;
}

template <typename T> TypeU<T, ErrCode> Worker::runPopcntOp(Value &Val) const {
  T I = retrieveValue<T>(Val);
  /// Return the count of non-zero bits in i.
  if (I != 0U) {
    T Cnt = 0;
    T Mask = (T)0x1U;
    while (I != 0U) {
      if (I & Mask) {
        Cnt++;
      }
      I >>= 1;
    }
    Val = Cnt;
  }
  return ErrCode::Success;
}

template <typename T> TypeF<T, ErrCode> Worker::runAbsOp(Value &Val) const {
  Val = std::fabs(retrieveValue<T>(Val));
  return ErrCode::Success;
}

template <typename T> TypeF<T, ErrCode> Worker::runNegOp(Value &Val) const {
  Val = -retrieveValue<T>(Val);
  return ErrCode::Success;
}

template <typename T> TypeF<T, ErrCode> Worker::runCeilOp(Value &Val) const {
  Val = std::ceil(retrieveValue<T>(Val));
  return ErrCode::Success;
}

template <typename T> TypeF<T, ErrCode> Worker::runFloorOp(Value &Val) const {
  Val = std::floor(retrieveValue<T>(Val));
  return ErrCode::Success;
}

template <typename T> TypeF<T, ErrCode> Worker::runTruncOp(Value &Val) const {
  Val = std::trunc(retrieveValue<T>(Val));
  return ErrCode::Success;
}

template <typename T> TypeF<T, ErrCode> Worker::runNearestOp(Value &Val) const {
  Val = std::nearbyint(retrieveValue<T>(Val));
  return ErrCode::Success;
}

template <typename T> TypeF<T, ErrCode> Worker::runSqrtOp(Value &Val) const {
  Val = std::sqrt(retrieveValue<T>(Val));
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
