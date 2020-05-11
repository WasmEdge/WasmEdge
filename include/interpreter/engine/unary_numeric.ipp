// SPDX-License-Identifier: Apache-2.0
#include "common/value.h"
#include "interpreter/interpreter.h"
#include "support/roundeven.h"
#include <cmath>

namespace SSVM {
namespace Interpreter {

template <typename T> TypeU<T> Interpreter::runClzOp(ValVariant &Val) const {
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
  } else {
    Val = static_cast<T>(sizeof(T) * 8);
  }
  return {};
}

template <typename T> TypeU<T> Interpreter::runCtzOp(ValVariant &Val) const {
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
  return {};
}

template <typename T> TypeU<T> Interpreter::runPopcntOp(ValVariant &Val) const {
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
  return {};
}

template <typename T> TypeF<T> Interpreter::runAbsOp(ValVariant &Val) const {
  Val = std::fabs(retrieveValue<T>(Val));
  return {};
}

template <typename T> TypeF<T> Interpreter::runNegOp(ValVariant &Val) const {
  Val = -retrieveValue<T>(Val);
  return {};
}

template <typename T> TypeF<T> Interpreter::runCeilOp(ValVariant &Val) const {
  Val = std::ceil(retrieveValue<T>(Val));
  return {};
}

template <typename T> TypeF<T> Interpreter::runFloorOp(ValVariant &Val) const {
  Val = std::floor(retrieveValue<T>(Val));
  return {};
}

template <typename T> TypeF<T> Interpreter::runTruncOp(ValVariant &Val) const {
  Val = std::trunc(retrieveValue<T>(Val));
  return {};
}

template <typename T>
TypeF<T> Interpreter::runNearestOp(ValVariant &Val) const {
  Val = SSVM::roundeven(T(retrieveValue<T>(Val)));
  return {};
}

template <typename T> TypeF<T> Interpreter::runSqrtOp(ValVariant &Val) const {
  Val = std::sqrt(retrieveValue<T>(Val));
  return {};
}

} // namespace Interpreter
} // namespace SSVM
