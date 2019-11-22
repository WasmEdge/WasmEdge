#include "executor/common.h"
#include "executor/entry/value.h"
#include "executor/worker.h"
#include "executor/worker/util.h"
#include "support/casting.h"
#include <cmath>

namespace SSVM {
namespace Executor {

template <typename T> TypeU<T, ErrCode> Worker::runEqzOp(const Value &Val) {
  /// Return 1 if i is zero, 0 otherwise.
  return StackMgr.pushValue((retrieveValue<T>(Val) == 0) ? 1U : 0U);
}

template <typename T>
TypeT<T, ErrCode> Worker::runEqOp(const Value &Val1, const Value &Val2) {
  T V1 = retrieveValue<T>(Val1), V2 = retrieveValue<T>(Val2);
  /// Return 1 if v1 == v2, 0 otherwise. NaN, inf, and +-0.0 cases handled.
  return StackMgr.pushValue((V1 == V2) ? 1U : 0U);
}

template <typename T>
TypeT<T, ErrCode> Worker::runNeOp(const Value &Val1, const Value &Val2) {
  T V1 = retrieveValue<T>(Val1), V2 = retrieveValue<T>(Val2);
  /// Return 1 if v1 != v2, 0 otherwise. NaN, inf, and +-0.0 cases handled.
  return StackMgr.pushValue((V1 != V2) ? 1U : 0U);
}

template <typename T>
TypeT<T, ErrCode> Worker::runLtOp(const Value &Val1, const Value &Val2) {
  T V1 = retrieveValue<T>(Val1), V2 = retrieveValue<T>(Val2);
  /// Return 1 if v1 < v2, 0 otherwise. Signed, NaN, inf, +-0.0 cases handled.
  return StackMgr.pushValue((V1 < V2) ? 1U : 0U);
}

template <typename T>
TypeT<T, ErrCode> Worker::runGtOp(const Value &Val1, const Value &Val2) {
  T V1 = retrieveValue<T>(Val1), V2 = retrieveValue<T>(Val2);
  /// Return 1 if v1 > v2, 0 otherwise. Signed, NaN, inf, +-0.0 cases handled.
  return StackMgr.pushValue((V1 > V2) ? 1U : 0U);
}

template <typename T>
TypeT<T, ErrCode> Worker::runLeOp(const Value &Val1, const Value &Val2) {
  T V1 = retrieveValue<T>(Val1), V2 = retrieveValue<T>(Val2);
  /// Return 1 if v1 <= v2, 0 otherwise. Signed, NaN, inf, +-0.0 cases handled.
  return StackMgr.pushValue((V1 <= V2) ? 1U : 0U);
}

template <typename T>
TypeT<T, ErrCode> Worker::runGeOp(const Value &Val1, const Value &Val2) {
  T V1 = retrieveValue<T>(Val1), V2 = retrieveValue<T>(Val2);
  /// Return 1 if v1 >= v2, 0 otherwise. Signed, NaN, inf, +-0.0 cases handled.
  return StackMgr.pushValue((V1 >= V2) ? 1U : 0U);
}

} // namespace Executor
} // namespace SSVM
