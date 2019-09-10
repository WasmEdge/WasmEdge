#include "executor/common.h"
#include "executor/entry/value.h"
#include "executor/worker.h"
#include "executor/worker/util.h"
#include <cmath>

namespace SSVM {
namespace Executor {

template <typename T> ErrCode Worker::runIClzOp(const ValueEntry *Val) {
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

template <typename T> ErrCode Worker::runICtzOp(const ValueEntry *Val) {
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

template <typename T> ErrCode Worker::runIPopcntOp(const ValueEntry *Val) {
  T I = retrieveValue<T>(*Val);
  /// Return the count of non-zero bits in i.
  if (I == 0U) {
    return StackMgr.pushValue((T)(sizeof(T) * 8));
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

template <typename T> ErrCode Worker::runFAbsOp(const ValueEntry *Val) {
  return StackMgr.pushValue(std::fabs(retrieveValue<T>(*Val)));
}

template <typename T> ErrCode Worker::runFNegOp(const ValueEntry *Val) {
  return StackMgr.pushValue(-retrieveValue<T>(*Val));
}

template <typename T> ErrCode Worker::runFCeilOp(const ValueEntry *Val) {
  return StackMgr.pushValue(std::ceil(retrieveValue<T>(*Val)));
}

template <typename T> ErrCode Worker::runFFloorOp(const ValueEntry *Val) {
  return StackMgr.pushValue(std::floor(retrieveValue<T>(*Val)));
}

template <typename T> ErrCode Worker::runFTruncOp(const ValueEntry *Val) {
  return StackMgr.pushValue(std::trunc(retrieveValue<T>(*Val)));
}

template <typename T> ErrCode Worker::runFNearestOp(const ValueEntry *Val) {
  return StackMgr.pushValue(std::nearbyint(retrieveValue<T>(*Val)));
}

template <typename T> ErrCode Worker::runFSqrtOp(const ValueEntry *Val) {
  return StackMgr.pushValue(std::sqrt(retrieveValue<T>(*Val)));
}

} // namespace Executor
} // namespace SSVM