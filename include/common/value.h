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

/// Definition of number_type.
using ValVariant = Support::Variant<uint32_t, uint64_t, float, double>;
using Byte = uint8_t;

/// Reference type pattern:
/// bits | 0 ---- 15 | 16 ---- 31 | 32 ----------------- 63
///         is null     Ref Type      u32 reference index
inline constexpr ValVariant genRefType(const RefType Type) {
  return (static_cast<uint64_t>(Type) << 32) + (static_cast<uint64_t>(1) << 48);
}
inline constexpr ValVariant genRefType(const RefType Type, const uint32_t Idx) {
  return (static_cast<uint64_t>(Type) << 32) + Idx;
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
    return genRefType(RefType::FuncRef);
  case ValType::ExternRef:
    return genRefType(RefType::ExternRef);
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

/// Retrieve references.
inline constexpr RefType retrieveRefType(const ValVariant &Val) {
  return static_cast<RefType>((std::get<uint64_t>(Val) >> 32) & 0xFF);
}
inline constexpr bool isNullRef(const ValVariant &Val) {
  return (std::get<uint64_t>(Val) >> 48) > 0;
}
inline constexpr uint32_t retrieveRefIdx(const ValVariant &Val) {
  return static_cast<uint32_t>(std::get<uint64_t>(Val));
}

} // namespace SSVM
