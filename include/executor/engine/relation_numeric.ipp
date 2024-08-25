// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/executor.h"

namespace WasmEdge {
namespace Executor {

template <typename T> TypeU<T> Executor::runEqzOp(ValVariant &Val) const {
  // Return 1 if i is zero, 0 otherwise.
  Val.emplace<uint32_t>(Val.get<T>() == 0 ? 1U : 0U);
  return {};
}

template <typename T>
TypeT<T> Executor::runEqOp(ValVariant &Val1, const ValVariant &Val2) const {
  // Return 1 if v1 == v2, 0 otherwise. NaN, inf, and +-0.0 cases handled.
  Val1.emplace<uint32_t>(Val1.get<T>() == Val2.get<T>() ? 1U : 0U);
  return {};
}

template <typename T>
TypeT<T> Executor::runNeOp(ValVariant &Val1, const ValVariant &Val2) const {
  // Return 1 if v1 != v2, 0 otherwise. NaN, inf, and +-0.0 cases handled.
  Val1.emplace<uint32_t>(Val1.get<T>() != Val2.get<T>() ? 1U : 0U);
  return {};
}

template <typename T>
TypeT<T> Executor::runLtOp(ValVariant &Val1, const ValVariant &Val2) const {
  // Return 1 if v1 < v2, 0 otherwise. Signed, NaN, inf, +-0.0 cases handled.
  Val1.emplace<uint32_t>(Val1.get<T>() < Val2.get<T>() ? 1U : 0U);
  return {};
}

template <typename T>
TypeT<T> Executor::runGtOp(ValVariant &Val1, const ValVariant &Val2) const {
  // Return 1 if v1 > v2, 0 otherwise. Signed, NaN, inf, +-0.0 cases handled.
  Val1.emplace<uint32_t>(Val1.get<T>() > Val2.get<T>() ? 1U : 0U);
  return {};
}

template <typename T>
TypeT<T> Executor::runLeOp(ValVariant &Val1, const ValVariant &Val2) const {
  // Return 1 if v1 <= v2, 0 otherwise. Signed, NaN, inf, +-0.0 cases handled.
  Val1.emplace<uint32_t>(Val1.get<T>() <= Val2.get<T>() ? 1U : 0U);
  return {};
}

template <typename T>
TypeT<T> Executor::runGeOp(ValVariant &Val1, const ValVariant &Val2) const {
  // Return 1 if v1 >= v2, 0 otherwise. Signed, NaN, inf, +-0.0 cases handled.
  Val1.emplace<uint32_t>(Val1.get<T>() >= Val2.get<T>() ? 1U : 0U);
  return {};
}

} // namespace Executor
} // namespace WasmEdge
