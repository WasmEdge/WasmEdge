#include "executor/common.h"
#include "executor/entry/value.h"
#include "executor/worker.h"
#include "executor/worker/util.h"
#include <cmath>

namespace SSVM {
namespace Executor {

template <typename T>
TypeU<T, ErrCode> Worker::runClzOp(const ValueEntry *Val) {
  T I = retrieveValue<T>(*Val);
  /// Return the count of leading zero bits in i.
  if (I == 0U) {
    return StackMgr.pushValue((T)(sizeof(T) * 8));
  }
  T Cnt = 0;
  T Mask = (T)0x1U << (sizeof(T) * 8 - 1);
  while ((I & Mask) == 0U) {
    Cnt++;
    I <<= 1;
  }
  return StackMgr.pushValue(Cnt);
}

template <typename T>
TypeU<T, ErrCode> Worker::runCtzOp(const ValueEntry *Val) {
  T I = retrieveValue<T>(*Val);
  /// Return the count of trailing zero bits in i.
  if (I == 0U) {
    return StackMgr.pushValue((T)(sizeof(T) * 8));
  }
  T Cnt = 0;
  T Mask = (T)0x1U;
  while ((I & Mask) == 0U) {
    Cnt++;
    I >>= 1;
  }
  return StackMgr.pushValue(Cnt);
}

template <typename T>
TypeU<T, ErrCode> Worker::runPopcntOp(const ValueEntry *Val) {
  T I = retrieveValue<T>(*Val);
  /// Return the count of non-zero bits in i.
  if (I == 0U) {
    return StackMgr.pushValue((T)0);
  }
  T Cnt = 0;
  T Mask = (T)0x1U;
  while (I != 0U) {
    if (I & Mask) {
      Cnt++;
    }
    I >>= 1;
  }
  return StackMgr.pushValue(Cnt);
}

template <typename T>
TypeF<T, ErrCode> Worker::runAbsOp(const ValueEntry *Val) {
  return StackMgr.pushValue(std::fabs(retrieveValue<T>(*Val)));
}

template <typename T>
TypeF<T, ErrCode> Worker::runNegOp(const ValueEntry *Val) {
  return StackMgr.pushValue(-retrieveValue<T>(*Val));
}

template <typename T>
TypeF<T, ErrCode> Worker::runCeilOp(const ValueEntry *Val) {
  return StackMgr.pushValue(std::ceil(retrieveValue<T>(*Val)));
}

template <typename T>
TypeF<T, ErrCode> Worker::runFloorOp(const ValueEntry *Val) {
  return StackMgr.pushValue(std::floor(retrieveValue<T>(*Val)));
}

template <typename T>
TypeF<T, ErrCode> Worker::runTruncOp(const ValueEntry *Val) {
  return StackMgr.pushValue(std::trunc(retrieveValue<T>(*Val)));
}

template <typename T>
TypeF<T, ErrCode> Worker::runNearestOp(const ValueEntry *Val) {
  return StackMgr.pushValue(std::nearbyint(retrieveValue<T>(*Val)));
}

template <typename T>
TypeF<T, ErrCode> Worker::runSqrtOp(const ValueEntry *Val) {
  return StackMgr.pushValue(std::sqrt(retrieveValue<T>(*Val)));
}

} // namespace Executor
} // namespace SSVM