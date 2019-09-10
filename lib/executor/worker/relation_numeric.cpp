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
ErrCode Worker::runEqIOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  T I1 = retrieveValue<T>(*Val1), I2 = retrieveValue<T>(*Val2);
  /// Return 1 if i1 equals i2, 0 otherwise.
  return StackMgr.pushValue((I1 == I2) ? 1U : 0U);
}

template <typename T>
ErrCode Worker::runEqFOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  T Z1 = retrieveValue<T>(*Val1), Z2 = retrieveValue<T>(*Val2);
  /// If either z1 or z2 is a NaN, then return 0.
  if (std::isnan(Z1) || std::isnan(Z2)) {
    return StackMgr.pushValue(0U);
  }
  /// If both z1 and z2 are zeroes, then return 1.
  if ((Z1 == +0.0f || Z1 == -0.0f) && (Z2 == +0.0f || Z2 == -0.0f)) {
    return StackMgr.pushValue(1U);
  }
  /// If both z1 and z2 are the same value, then return 1. Else return 0.
  return StackMgr.pushValue((Z1 == Z2) ? 1U : 0U);
}

template <typename T>
ErrCode Worker::runNeIOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  T I1 = retrieveValue<T>(*Val1), I2 = retrieveValue<T>(*Val2);
  /// Return 1 if i1 does not equal i2, 0 otherwise.
  return StackMgr.pushValue((I1 != I2) ? 1U : 0U);
}

template <typename T>
ErrCode Worker::runNeFOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  T Z1 = retrieveValue<T>(*Val1), Z2 = retrieveValue<T>(*Val2);
  /// If either z1 or z2 is a NaN, then return 1.
  if (std::isnan(Z1) || std::isnan(Z2)) {
    return StackMgr.pushValue(1U);
  }
  /// If both z1 and z2 are zeroes, then return 0.
  if ((Z1 == +0.0f || Z1 == -0.0f) && (Z2 == +0.0f || Z2 == -0.0f)) {
    return StackMgr.pushValue(0U);
  }
  /// If both z1 and z2 are the same value, then return 0. Else return 1.
  return StackMgr.pushValue((Z1 == Z2) ? 0U : 1U);
}

template <typename T>
ErrCode Worker::runLtUOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  T I1 = retrieveValue<T>(*Val1), I2 = retrieveValue<T>(*Val2);
  /// Return 1 if i1 is less than i2, 0 otherwise.
  return StackMgr.pushValue((I1 < I2) ? 1U : 0U);
}

template <typename T>
ErrCode Worker::runLtSOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  T I1 = retrieveValue<T>(*Val1), I2 = retrieveValue<T>(*Val2);
  /// Return 1 if signed_interpret(i1) is less than signed_interpret(i2), 0
  /// otherwise.
  return StackMgr.pushValue(
      (signedInterpretation(I1) < signedInterpretation(I2)) ? 1U : 0U);
}

template <typename T>
ErrCode Worker::runLtFOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  T Z1 = retrieveValue<T>(*Val1), Z2 = retrieveValue<T>(*Val2);
  /// If either z1 or z2 is a NaN, then return 0.
  if (std::isnan(Z1) || std::isnan(Z2)) {
    return StackMgr.pushValue(0U);
  }
  /// If z1 and z2 are the same value, then return 0.
  if (Z1 == Z2) {
    return StackMgr.pushValue(0U);
  }
  /// If z1 is positive infinity, then return 0.
  if (std::isinf(Z1) && Z1 > 0) {
    return StackMgr.pushValue(0U);
  }
  /// If z1 is negative infinity, then return 1.
  if (std::isinf(Z1) && Z1 < 0) {
    return StackMgr.pushValue(1U);
  }
  /// If z2 is positive infinity, then return 1.
  if (std::isinf(Z2) && Z2 > 0) {
    return StackMgr.pushValue(1U);
  }
  /// If z2 is negative infinity, then return 0.
  if (std::isinf(Z2) && Z2 < 0) {
    return StackMgr.pushValue(0U);
  }
  /// If both z1 and z2 are zeroes, then return 0.
  if ((Z1 == +0.0f || Z1 == -0.0f) && (Z2 == +0.0f || Z2 == -0.0f)) {
    return StackMgr.pushValue(0U);
  }
  /// If z1 is smaller than z2, then return 1. Else return 0.
  return StackMgr.pushValue((Z1 < Z2) ? 1U : 0U);
}

template <typename T>
ErrCode Worker::runGtUOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  T I1 = retrieveValue<T>(*Val1), I2 = retrieveValue<T>(*Val2);
  /// Return 1 if i1 is greater than i2, 0 otherwise.
  return StackMgr.pushValue((I1 > I2) ? 1U : 0U);
}

template <typename T>
ErrCode Worker::runGtSOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  T I1 = retrieveValue<T>(*Val1), I2 = retrieveValue<T>(*Val2);
  /// Return 1 if signed_interpret(i1) is greater than signed_interpret(i2), 0
  /// otherwise.
  return StackMgr.pushValue(
      (signedInterpretation(I1) > signedInterpretation(I2)) ? 1U : 0U);
}

template <typename T>
ErrCode Worker::runGtFOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  T Z1 = retrieveValue<T>(*Val1), Z2 = retrieveValue<T>(*Val2);
  /// If either z1 or z2 is a NaN, then return 0.
  if (std::isnan(Z1) || std::isnan(Z2)) {
    return StackMgr.pushValue(0U);
  }
  /// If z1 and z2 are the same value, then return 0.
  if (Z1 == Z2) {
    return StackMgr.pushValue(0U);
  }
  /// If z1 is positive infinity, then return 1.
  if (std::isinf(Z1) && Z1 > 0) {
    return StackMgr.pushValue(1U);
  }
  /// If z1 is negative infinity, then return 0.
  if (std::isinf(Z1) && Z1 < 0) {
    return StackMgr.pushValue(0U);
  }
  /// If z2 is positive infinity, then return 0.
  if (std::isinf(Z2) && Z2 > 0) {
    return StackMgr.pushValue(0U);
  }
  /// If z2 is negative infinity, then return 1.
  if (std::isinf(Z2) && Z2 < 0) {
    return StackMgr.pushValue(1U);
  }
  /// If both z1 and z2 are zeroes, then return 0.
  if ((Z1 == +0.0f || Z1 == -0.0f) && (Z2 == +0.0f || Z2 == -0.0f)) {
    return StackMgr.pushValue(0U);
  }
  /// If z1 is larger than z2, then return 1. Else return 0.
  return StackMgr.pushValue((Z1 > Z2) ? 1U : 0U);
}

template <typename T>
ErrCode Worker::runLeUOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  T I1 = retrieveValue<T>(*Val1), I2 = retrieveValue<T>(*Val2);
  /// Return 1 if i1 is less than or equal i2, 0 otherwise.
  return StackMgr.pushValue((I1 <= I2) ? 1U : 0U);
}

template <typename T>
ErrCode Worker::runLeSOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  T I1 = retrieveValue<T>(*Val1), I2 = retrieveValue<T>(*Val2);
  /// Return 1 if signed_interpret(i1) is less than or equal to
  /// signed_interpret(i2), 0 otherwise.
  return StackMgr.pushValue(
      (signedInterpretation(I1) <= signedInterpretation(I2)) ? 1U : 0U);
}

template <typename T>
ErrCode Worker::runLeFOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  T Z1 = retrieveValue<T>(*Val1), Z2 = retrieveValue<T>(*Val2);
  /// If either z1 or z2 is a NaN, then return 0.
  if (std::isnan(Z1) || std::isnan(Z2)) {
    return StackMgr.pushValue(0U);
  }
  /// If z1 and z2 are the same value, then return 1.
  if (Z1 == Z2) {
    return StackMgr.pushValue(1U);
  }
  /// If z1 is positive infinity, then return 0.
  if (std::isinf(Z1) && Z1 > 0) {
    return StackMgr.pushValue(0U);
  }
  /// If z1 is negative infinity, then return 1.
  if (std::isinf(Z1) && Z1 < 0) {
    return StackMgr.pushValue(1U);
  }
  /// If z2 is positive infinity, then return 1.
  if (std::isinf(Z2) && Z2 > 0) {
    return StackMgr.pushValue(1U);
  }
  /// If z2 is negative infinity, then return 0.
  if (std::isinf(Z2) && Z2 < 0) {
    return StackMgr.pushValue(0U);
  }
  /// If both z1 and z2 are zeroes, then return 1.
  if ((Z1 == +0.0f || Z1 == -0.0f) && (Z2 == +0.0f || Z2 == -0.0f)) {
    return StackMgr.pushValue(1U);
  }
  /// If z1 is smaller than or equal to z2, then return 1. Else return 0.
  return StackMgr.pushValue((Z1 <= Z2) ? 1U : 0U);
}

template <typename T>
ErrCode Worker::runGeUOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  T I1 = retrieveValue<T>(*Val1), I2 = retrieveValue<T>(*Val2);
  /// Return 1 if i1 is greater than or equal i2, 0 otherwise.
  return StackMgr.pushValue((I1 >= I2) ? 1U : 0U);
}

template <typename T>
ErrCode Worker::runGeSOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  T I1 = retrieveValue<T>(*Val1), I2 = retrieveValue<T>(*Val2);
  /// Return 1 if signed_interpret(i1) is greater than or equal to
  /// signed_interpret(i2), 0 otherwise.
  return StackMgr.pushValue(
      (signedInterpretation(I1) >= signedInterpretation(I2)) ? 1U : 0U);
}

template <typename T>
ErrCode Worker::runGeFOp(const ValueEntry *Val1, const ValueEntry *Val2) {
  T Z1 = retrieveValue<T>(*Val1), Z2 = retrieveValue<T>(*Val2);
  /// If either z1 or z2 is a NaN, then return 0.
  if (std::isnan(Z1) || std::isnan(Z2)) {
    return StackMgr.pushValue(0U);
  }
  /// If z1 and z2 are the same value, then return 1.
  if (Z1 == Z2) {
    return StackMgr.pushValue(1U);
  }
  /// If z1 is positive infinity, then return 1.
  if (std::isinf(Z1) && Z1 > 0) {
    return StackMgr.pushValue(1U);
  }
  /// If z1 is negative infinity, then return 0.
  if (std::isinf(Z1) && Z1 < 0) {
    return StackMgr.pushValue(0U);
  }
  /// If z2 is positive infinity, then return 0.
  if (std::isinf(Z2) && Z2 > 0) {
    return StackMgr.pushValue(0U);
  }
  /// If z2 is negative infinity, then return 1.
  if (std::isinf(Z2) && Z2 < 0) {
    return StackMgr.pushValue(1U);
  }
  /// If both z1 and z2 are zeroes, then return 1.
  if ((Z1 == +0.0f || Z1 == -0.0f) && (Z2 == +0.0f || Z2 == -0.0f)) {
    return StackMgr.pushValue(1U);
  }
  /// If z1 is larger than or equal to z2, then return 1. Else return 0.
  return StackMgr.pushValue((Z1 >= Z2) ? 1U : 0U);
}

} // namespace Executor
} // namespace SSVM
