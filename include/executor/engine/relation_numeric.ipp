// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/executor.h"

namespace WasmEdge {
namespace Executor {

template <typename T>
TypeU<T> Executor::runEqzOp(Runtime::StackManager &StackMgr) const noexcept {
  // Return 1 if i is zero, 0 otherwise.
  StackMgr.emplaceTop<uint32_t>(StackMgr.peekTop<T>() == 0 ? 1U : 0U);
  return {};
}

template <typename T>
TypeT<T> Executor::runEqOp(Runtime::StackManager &StackMgr) const noexcept {
  // Return 1 if v1 == v2, 0 otherwise. NaN, inf, and +-0.0 cases handled.
  auto [Val2, Val1] = StackMgr.popsPeekTop<T, T>();
  StackMgr.emplaceTop<uint32_t>(Val1 == Val2 ? 1U : 0U);
  return {};
}

template <typename T>
TypeT<T> Executor::runNeOp(Runtime::StackManager &StackMgr) const noexcept {
  // Return 1 if v1 != v2, 0 otherwise. NaN, inf, and +-0.0 cases handled.
  auto [Val2, Val1] = StackMgr.popsPeekTop<T, T>();
  StackMgr.emplaceTop<uint32_t>(Val1 != Val2 ? 1U : 0U);
  return {};
}

template <typename T>
TypeT<T> Executor::runLtOp(Runtime::StackManager &StackMgr) const noexcept {
  // Return 1 if v1 < v2, 0 otherwise. Signed, NaN, inf, +-0.0 cases handled.
  auto [Val2, Val1] = StackMgr.popsPeekTop<T, T>();
  StackMgr.emplaceTop<uint32_t>(Val1 < Val2 ? 1U : 0U);
  return {};
}

template <typename T>
TypeT<T> Executor::runGtOp(Runtime::StackManager &StackMgr) const noexcept {
  // Return 1 if v1 > v2, 0 otherwise. Signed, NaN, inf, +-0.0 cases handled.
  auto [Val2, Val1] = StackMgr.popsPeekTop<T, T>();
  StackMgr.emplaceTop<uint32_t>(Val1 > Val2 ? 1U : 0U);
  return {};
}

template <typename T>
TypeT<T> Executor::runLeOp(Runtime::StackManager &StackMgr) const noexcept {
  // Return 1 if v1 <= v2, 0 otherwise. Signed, NaN, inf, +-0.0 cases handled.
  auto [Val2, Val1] = StackMgr.popsPeekTop<T, T>();
  StackMgr.emplaceTop<uint32_t>(Val1 <= Val2 ? 1U : 0U);
  return {};
}

template <typename T>
TypeT<T> Executor::runGeOp(Runtime::StackManager &StackMgr) const noexcept {
  // Return 1 if v1 >= v2, 0 otherwise. Signed, NaN, inf, +-0.0 cases handled.
  auto [Val2, Val1] = StackMgr.popsPeekTop<T, T>();
  StackMgr.emplaceTop<uint32_t>(Val1 >= Val2 ? 1U : 0U);
  return {};
}

} // namespace Executor
} // namespace WasmEdge
