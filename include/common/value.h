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

#include "support/casting.h"
#include "support/variant.h"
#include "types.h"

#include <cstdint>
#include <vector>

namespace SSVM {

using ValVariant = Support::Variant<uint32_t, uint64_t, float, double>;
using Byte = uint8_t;

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
  }
}

/// Retrieve value.
template <typename T> inline const T &retrieveValue(const ValVariant &Val) {
  return *reinterpret_cast<const T *>(
      &std::get<Support::TypeToWasmTypeT<T>>(Val));
}
template <typename T> inline T &retrieveValue(ValVariant &Val) {
  return *reinterpret_cast<T *>(&std::get<Support::TypeToWasmTypeT<T>>(Val));
}
template <typename T> inline const T &&retrieveValue(const ValVariant &&Val) {
  return std::move(*reinterpret_cast<const T *>(
      &std::get<Support::TypeToWasmTypeT<T>>(Val)));
}
template <typename T> inline T &&retrieveValue(ValVariant &&Val) {
  return std::move(
      *reinterpret_cast<T *>(&std::get<Support::TypeToWasmTypeT<T>>(Val)));
}

} // namespace SSVM
