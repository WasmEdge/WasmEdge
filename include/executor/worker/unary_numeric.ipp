#include "executor/common.h"
#include "executor/entry/value.h"
#include "executor/worker.h"
#include "executor/worker/util.h"
#include <cmath>

namespace SSVM {
namespace Executor {

template <typename T> TypeU<T, ErrCode> Worker::runClzOp(const Value &Val) {
  T I = retrieveValue<T>(Val);
  /// Return the count of leading zero bits in i.
  if (I == 0U) {
    return StackMgr.push(static_cast<T>(sizeof(T) * 8));
  }
  T Cnt = 0;
  T Mask = (T)0x1U << (sizeof(T) * 8 - 1);
  while ((I & Mask) == 0U) {
    Cnt++;
    I <<= 1;
  }
  return StackMgr.push(Cnt);
}

template <typename T> TypeU<T, ErrCode> Worker::runCtzOp(const Value &Val) {
  T I = retrieveValue<T>(Val);
  /// Return the count of trailing zero bits in i.
  if (I == 0U) {
    return StackMgr.push(static_cast<T>(sizeof(T) * 8));
  }
  T Cnt = 0;
  T Mask = (T)0x1U;
  while ((I & Mask) == 0U) {
    Cnt++;
    I >>= 1;
  }
  return StackMgr.push(Cnt);
}

template <typename T> TypeU<T, ErrCode> Worker::runPopcntOp(const Value &Val) {
  T I = retrieveValue<T>(Val);
  /// Return the count of non-zero bits in i.
  if (I == 0U) {
    return StackMgr.push(static_cast<T>(0));
  }
  T Cnt = 0;
  T Mask = (T)0x1U;
  while (I != 0U) {
    if (I & Mask) {
      Cnt++;
    }
    I >>= 1;
  }
  return StackMgr.push(Cnt);
}

template <typename T> TypeF<T, ErrCode> Worker::runAbsOp(const Value &Val) {
  return StackMgr.push(std::fabs(retrieveValue<T>(Val)));
}

template <typename T> TypeF<T, ErrCode> Worker::runNegOp(const Value &Val) {
  return StackMgr.push(-retrieveValue<T>(Val));
}

template <typename T> TypeF<T, ErrCode> Worker::runCeilOp(const Value &Val) {
  return StackMgr.push(std::ceil(retrieveValue<T>(Val)));
}

template <typename T> TypeF<T, ErrCode> Worker::runFloorOp(const Value &Val) {
  return StackMgr.push(std::floor(retrieveValue<T>(Val)));
}

template <typename T> TypeF<T, ErrCode> Worker::runTruncOp(const Value &Val) {
  return StackMgr.push(std::trunc(retrieveValue<T>(Val)));
}

template <typename T> TypeF<T, ErrCode> Worker::runNearestOp(const Value &Val) {
  return StackMgr.push(std::nearbyint(retrieveValue<T>(Val)));
}

template <typename T> TypeF<T, ErrCode> Worker::runSqrtOp(const Value &Val) {
  return StackMgr.push(std::sqrt(retrieveValue<T>(Val)));
}

} // namespace Executor
} // namespace SSVM
