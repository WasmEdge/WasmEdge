#include "executor/common.h"
#include "executor/entry/value.h"
#include "executor/worker.h"
#include "executor/worker/util.h"
#include "support/casting.h"
#include <cmath>

namespace SSVM {
namespace Executor {

template <typename T> ErrCode Worker::runEqzOp(const ValueEntry *Val) {
  /// Return 1 if i is zero, 0 otherwise.
  return StackMgr.pushValue((retrieveValue<T>(*Val) == 0) ? 1U : 0U);
}

template <typename T>
ErrCode Worker::runTEqOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  T I1 = retrieveValue<T>(*Val1), I2 = retrieveValue<T>(*Val2);
  /// Return 1 if i1 == i2, 0 otherwise. (NaN, inf, and +-0.0 cases handled.)
  return StackMgr.pushValue((I1 == I2) ? 1U : 0U);
}

template <typename T>
ErrCode Worker::runTNeOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  T I1 = retrieveValue<T>(*Val1), I2 = retrieveValue<T>(*Val2);
  /// Return 1 if i1 != i2, 0 otherwise. (NaN, inf, and +-0.0 cases handled.)
  return StackMgr.pushValue((I1 != I2) ? 1U : 0U);
}

template <typename T>
ErrCode Worker::runILtUOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  T I1 = retrieveValue<T>(*Val1), I2 = retrieveValue<T>(*Val2);
  /// Return 1 if i1 < i2, 0 otherwise.
  return StackMgr.pushValue((I1 < I2) ? 1U : 0U);
}

template <typename T>
ErrCode Worker::runILtSOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  T I1 = retrieveValue<T>(*Val1), I2 = retrieveValue<T>(*Val2);
  /// Return 1 if signed(i1) < signed(i2), 0 otherwise.
  return StackMgr.pushValue((toSigned(I1) < toSigned(I2)) ? 1U : 0U);
}

template <typename T>
ErrCode Worker::runFLtOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  /// NaN, inf, +-0.0 cases handled.
  return runILtUOp<T>(Val1, Val2);
}

template <typename T>
ErrCode Worker::runIGtUOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  T I1 = retrieveValue<T>(*Val1), I2 = retrieveValue<T>(*Val2);
  /// Return 1 if i1 > i2, 0 otherwise.
  return StackMgr.pushValue((I1 > I2) ? 1U : 0U);
}

template <typename T>
ErrCode Worker::runIGtSOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  T I1 = retrieveValue<T>(*Val1), I2 = retrieveValue<T>(*Val2);
  /// Return 1 if signed(i1) > signed(i2), 0 otherwise.
  return StackMgr.pushValue((toSigned(I1) > toSigned(I2)) ? 1U : 0U);
}

template <typename T>
ErrCode Worker::runFGtOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  /// NaN, inf, +-0.0 cases handled.
  return runIGtUOp<T>(Val1, Val2);
}

template <typename T>
ErrCode Worker::runILeUOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  T I1 = retrieveValue<T>(*Val1), I2 = retrieveValue<T>(*Val2);
  /// Return 1 if i1 <= i2, 0 otherwise.
  return StackMgr.pushValue((I1 <= I2) ? 1U : 0U);
}

template <typename T>
ErrCode Worker::runILeSOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  T I1 = retrieveValue<T>(*Val1), I2 = retrieveValue<T>(*Val2);
  /// Return 1 if signed(i1) <= signed(i2), 0 otherwise.
  return StackMgr.pushValue((toSigned(I1) <= toSigned(I2)) ? 1U : 0U);
}

template <typename T>
ErrCode Worker::runFLeOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  /// NaN, inf, +-0.0 cases handled.
  return runILeUOp<T>(Val1, Val2);
}

template <typename T>
ErrCode Worker::runIGeUOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  T I1 = retrieveValue<T>(*Val1), I2 = retrieveValue<T>(*Val2);
  /// Return 1 if i1 >= i2, 0 otherwise.
  return StackMgr.pushValue((I1 >= I2) ? 1U : 0U);
}

template <typename T>
ErrCode Worker::runIGeSOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  T I1 = retrieveValue<T>(*Val1), I2 = retrieveValue<T>(*Val2);
  /// Return 1 if signed(i1) >= signed(i2), 0 otherwise.
  return StackMgr.pushValue((toSigned(I1) >= toSigned(I2)) ? 1U : 0U);
}

template <typename T>
ErrCode Worker::runFGeOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  /// NaN, inf, +-0.0 cases handled.
  return runIGeUOp<T>(Val1, Val2);
}

} // namespace Executor
} // namespace SSVM
