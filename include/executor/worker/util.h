// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ast/instruction.h"
#include "executor/entry/value.h"
#include "support/casting.h"

namespace SSVM {
namespace Executor {

template <typename T> struct TypeToWasmType { using type = T; };
template <> struct TypeToWasmType<int32_t> { using type = uint32_t; };
template <> struct TypeToWasmType<int64_t> { using type = uint64_t; };
template <typename T> using TypeToWasmTypeT = typename TypeToWasmType<T>::type;

/// Retrieve value.
template <typename T> inline const T &retrieveValue(const Value &Val) {
  return *reinterpret_cast<const T *>(&std::get<TypeToWasmTypeT<T>>(Val));
}
template <typename T> inline T &retrieveValue(Value &Val) {
  return *reinterpret_cast<T *>(&std::get<TypeToWasmTypeT<T>>(Val));
}
template <typename T> inline const T &&retrieveValue(const Value &&Val) {
  return std::move(
      *reinterpret_cast<const T *>(&std::get<TypeToWasmTypeT<T>>(Val)));
}
template <typename T> inline T &&retrieveValue(Value &&Val) {
  return std::move(*reinterpret_cast<T *>(&std::get<TypeToWasmTypeT<T>>(Val)));
}

} // namespace Executor
} // namespace SSVM
