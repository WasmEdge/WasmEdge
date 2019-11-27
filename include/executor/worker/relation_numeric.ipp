#include "executor/common.h"
#include "executor/entry/value.h"
#include "executor/worker.h"
#include "executor/worker/util.h"
#include "support/casting.h"
#include <cmath>

namespace SSVM {
namespace Executor {

template <typename T> TypeU<T, ErrCode> Worker::runEqzOp(Value &Val) const {
  /// Return 1 if i is zero, 0 otherwise.
  Val = static_cast<uint32_t>(retrieveValue<T>(Val) == 0 ? 1U : 0U);
  return ErrCode::Success;
}

template <typename T>
TypeT<T, ErrCode> Worker::runEqOp(Value &Val1, const Value &Val2) const {
  /// Return 1 if v1 == v2, 0 otherwise. NaN, inf, and +-0.0 cases handled.
  Val1 = static_cast<uint32_t>(
      retrieveValue<T>(Val1) == retrieveValue<T>(Val2) ? 1U : 0U);
  return ErrCode::Success;
}

template <typename T>
TypeT<T, ErrCode> Worker::runNeOp(Value &Val1, const Value &Val2) const {
  /// Return 1 if v1 != v2, 0 otherwise. NaN, inf, and +-0.0 cases handled.
  Val1 = static_cast<uint32_t>(
      retrieveValue<T>(Val1) != retrieveValue<T>(Val2) ? 1U : 0U);
  return ErrCode::Success;
}

template <typename T>
TypeT<T, ErrCode> Worker::runLtOp(Value &Val1, const Value &Val2) const {
  /// Return 1 if v1 < v2, 0 otherwise. Signed, NaN, inf, +-0.0 cases handled.
  Val1 = static_cast<uint32_t>(
      retrieveValue<T>(Val1) < retrieveValue<T>(Val2) ? 1U : 0U);
  return ErrCode::Success;
}

template <typename T>
TypeT<T, ErrCode> Worker::runGtOp(Value &Val1, const Value &Val2) const {
  /// Return 1 if v1 > v2, 0 otherwise. Signed, NaN, inf, +-0.0 cases handled.
  Val1 = static_cast<uint32_t>(
      retrieveValue<T>(Val1) > retrieveValue<T>(Val2) ? 1U : 0U);
  return ErrCode::Success;
}

template <typename T>
TypeT<T, ErrCode> Worker::runLeOp(Value &Val1, const Value &Val2) const {
  /// Return 1 if v1 <= v2, 0 otherwise. Signed, NaN, inf, +-0.0 cases handled.
  Val1 = static_cast<uint32_t>(
      retrieveValue<T>(Val1) <= retrieveValue<T>(Val2) ? 1U : 0U);
  return ErrCode::Success;
}

template <typename T>
TypeT<T, ErrCode> Worker::runGeOp(Value &Val1, const Value &Val2) const {
  /// Return 1 if v1 >= v2, 0 otherwise. Signed, NaN, inf, +-0.0 cases handled.
  Val1 = static_cast<uint32_t>(
      retrieveValue<T>(Val1) >= retrieveValue<T>(Val2) ? 1U : 0U);
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
