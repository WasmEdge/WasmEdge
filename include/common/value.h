// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/value.h - Wasm value variant definition ------------*- C++ -*-===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the value struct of Wasm.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "support/variant.h"
#include "types.h"

#include <cstdint>
#include <vector>

namespace SSVM {

/// Definition of number_type.
using ValVariant =
    Support::Variant<uint32_t, uint64_t, float, double, FuncRef, ExternRef>;
using Byte = uint8_t;

/// Reference types helper functions.
inline constexpr ValVariant genNullRef(const RefType Type) {
  return static_cast<uint64_t>(0);
}
inline constexpr ValVariant genFuncRef(const uint32_t Idx) {
  return FuncRef{1, Idx};
}
template <typename T> inline ValVariant genExternRef(T *Ref) {
  return ExternRef{reinterpret_cast<uint64_t *>(Ref)};
}

template <typename T> inline ValType ValTypeFromType() noexcept;

template <> inline ValType ValTypeFromType<uint32_t>() noexcept {
  return ValType::I32;
}
template <> inline ValType ValTypeFromType<int32_t>() noexcept {
  return ValType::I32;
}
template <> inline ValType ValTypeFromType<uint64_t>() noexcept {
  return ValType::I64;
}
template <> inline ValType ValTypeFromType<int64_t>() noexcept {
  return ValType::I64;
}
template <> inline ValType ValTypeFromType<float>() noexcept {
  return ValType::F32;
}
template <> inline ValType ValTypeFromType<double>() noexcept {
  return ValType::F64;
}
template <> inline ValType ValTypeFromType<FuncRef>() noexcept {
  return ValType::FuncRef;
}
template <> inline ValType ValTypeFromType<ExternRef>() noexcept {
  return ValType::ExternRef;
}

inline constexpr ValVariant ValueFromType(ValType Type) noexcept {
  switch (Type) {
  default:
  case ValType::I32:
    return uint32_t(0U);
  case ValType::I64:
    return uint64_t(0U);
  case ValType::F32:
    return float(0.0F);
  case ValType::F64:
    return double(0.0);
  case ValType::FuncRef:
    return genNullRef(RefType::FuncRef);
  case ValType::ExternRef:
    return genNullRef(RefType::ExternRef);
  }
}

/// Retrieve value.
template <typename T> inline const T &retrieveValue(const ValVariant &Val) {
  return *reinterpret_cast<const T *>(&std::get<TypeToWasmTypeT<T>>(Val));
}
template <typename T> inline T &retrieveValue(ValVariant &Val) {
  return *reinterpret_cast<T *>(&std::get<TypeToWasmTypeT<T>>(Val));
}
template <typename T> inline const T &&retrieveValue(const ValVariant &&Val) {
  return std::move(
      *reinterpret_cast<const T *>(&std::get<TypeToWasmTypeT<T>>(Val)));
}
template <typename T> inline T &&retrieveValue(ValVariant &&Val) {
  return std::move(*reinterpret_cast<T *>(&std::get<TypeToWasmTypeT<T>>(Val)));
}

/// Retrieve references.
inline constexpr bool isNullRef(const ValVariant &Val) {
  return std::get<uint64_t>(Val) == 0;
}
inline constexpr uint32_t retrieveFuncIdx(const ValVariant &Val) {
  return std::get<FuncRef>(Val).Idx;
}
template <typename T> inline T &retrieveExternRef(const ValVariant &Val) {
  return *reinterpret_cast<T *>(std::get<ExternRef>(Val).Ptr);
}

} // namespace SSVM
