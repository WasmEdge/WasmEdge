// SPDX-License-Identifier: Apache-2.0

#include "interpreter/interpreter.h"

namespace WasmEdge {
namespace Interpreter {

template <typename T> TypeU<T> Interpreter::runEqzOp(ValVariant &Val) const {
  /// Return 1 if i is zero, 0 otherwise.
  Val.emplace<uint32_t>(Val.get<T>() == 0 ? 1U : 0U);
  return {};
}

template <typename T>
TypeT<T> Interpreter::runEqOp(ValVariant &Val1, const ValVariant &Val2) const {
  /// Return 1 if v1 == v2, 0 otherwise. NaN, inf, and +-0.0 cases handled.
  Val1.emplace<uint32_t>(Val1.get<T>() == Val2.get<T>() ? 1U : 0U);
  return {};
}

template <typename T>
TypeT<T> Interpreter::runNeOp(ValVariant &Val1, const ValVariant &Val2) const {
  /// Return 1 if v1 != v2, 0 otherwise. NaN, inf, and +-0.0 cases handled.
  Val1.emplace<uint32_t>(Val1.get<T>() != Val2.get<T>() ? 1U : 0U);
  return {};
}

template <typename T>
TypeT<T> Interpreter::runLtOp(ValVariant &Val1, const ValVariant &Val2) const {
  /// Return 1 if v1 < v2, 0 otherwise. Signed, NaN, inf, +-0.0 cases handled.
  Val1.emplace<uint32_t>(Val1.get<T>() < Val2.get<T>() ? 1U : 0U);
  return {};
}

template <typename T>
TypeT<T> Interpreter::runGtOp(ValVariant &Val1, const ValVariant &Val2) const {
  /// Return 1 if v1 > v2, 0 otherwise. Signed, NaN, inf, +-0.0 cases handled.
  Val1.emplace<uint32_t>(Val1.get<T>() > Val2.get<T>() ? 1U : 0U);
  return {};
}

template <typename T>
TypeT<T> Interpreter::runLeOp(ValVariant &Val1, const ValVariant &Val2) const {
  /// Return 1 if v1 <= v2, 0 otherwise. Signed, NaN, inf, +-0.0 cases handled.
  Val1.emplace<uint32_t>(Val1.get<T>() <= Val2.get<T>() ? 1U : 0U);
  return {};
}

template <typename T>
TypeT<T> Interpreter::runGeOp(ValVariant &Val1, const ValVariant &Val2) const {
  /// Return 1 if v1 >= v2, 0 otherwise. Signed, NaN, inf, +-0.0 cases handled.
  Val1.emplace<uint32_t>(Val1.get<T>() >= Val2.get<T>() ? 1U : 0U);
  return {};
}

} // namespace Interpreter
} // namespace WasmEdge
