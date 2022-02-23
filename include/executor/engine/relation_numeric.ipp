// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "executor/executor.h"

namespace WasmEdge {
namespace Executor {

template <typename T>
TypeU<T> Executor::runEqzOp(Runtime::StackManager &StackMgr) const noexcept {
  // Return 1 if i is zero, 0 otherwise.
  T Val = StackMgr.pop<T>();
  StackMgr.push<uint32_t>(Val == 0 ? 1U : 0U);
  return {};
}

template <typename T>
TypeT<T> Executor::runEqOp(Runtime::StackManager &StackMgr) const noexcept {
  // Return 1 if v1 == v2, 0 otherwise. NaN, inf, and +-0.0 cases handled.
  T RHS = StackMgr.pop<T>();
  T LHS = StackMgr.pop<T>();
  StackMgr.push<uint32_t>(LHS == RHS ? 1U : 0U);
  return {};
}

template <typename T>
TypeT<T> Executor::runNeOp(Runtime::StackManager &StackMgr) const noexcept {
  // Return 1 if v1 != v2, 0 otherwise. NaN, inf, and +-0.0 cases handled.
  T RHS = StackMgr.pop<T>();
  T LHS = StackMgr.pop<T>();
  StackMgr.push<uint32_t>(LHS != RHS ? 1U : 0U);
  return {};
}

template <typename T>
TypeT<T> Executor::runLtOp(Runtime::StackManager &StackMgr) const noexcept {
  // Return 1 if v1 < v2, 0 otherwise. Signed, NaN, inf, +-0.0 cases handled.
  T RHS = StackMgr.pop<T>();
  T LHS = StackMgr.pop<T>();
  StackMgr.push<uint32_t>(LHS < RHS ? 1U : 0U);
  return {};
}

template <typename T>
TypeT<T> Executor::runGtOp(Runtime::StackManager &StackMgr) const noexcept {
  // Return 1 if v1 > v2, 0 otherwise. Signed, NaN, inf, +-0.0 cases handled.
  T RHS = StackMgr.pop<T>();
  T LHS = StackMgr.pop<T>();
  StackMgr.push<uint32_t>(LHS > RHS ? 1U : 0U);
  return {};
}

template <typename T>
TypeT<T> Executor::runLeOp(Runtime::StackManager &StackMgr) const noexcept {
  // Return 1 if v1 <= v2, 0 otherwise. Signed, NaN, inf, +-0.0 cases handled.
  T RHS = StackMgr.pop<T>();
  T LHS = StackMgr.pop<T>();
  StackMgr.push<uint32_t>(LHS <= RHS ? 1U : 0U);
  return {};
}

template <typename T>
TypeT<T> Executor::runGeOp(Runtime::StackManager &StackMgr) const noexcept {
  // Return 1 if v1 >= v2, 0 otherwise. Signed, NaN, inf, +-0.0 cases handled.
  T RHS = StackMgr.pop<T>();
  T LHS = StackMgr.pop<T>();
  StackMgr.push<uint32_t>(LHS >= RHS ? 1U : 0U);
  return {};
}

} // namespace Executor
} // namespace WasmEdge
