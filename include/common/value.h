// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/common/value.h - SSVM value variant definition ---------------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the value struct used in SSVM.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "types.h"
#include "variant.h"

#include <cstdint>
#include <vector>

namespace SSVM {

/// Definition of number_type.
using RefVariant = Variant<uint64_t, FuncRef, ExternRef>;
using ValVariant =
    Variant<uint32_t, uint64_t, uint128_t, uint64x2_t, uint32x4_t, uint16x8_t,
            uint8x16_t, floatx4_t, doublex2_t, float, double, RefVariant,
            FuncRef, ExternRef>;
using Byte = uint8_t;

/// Reference types helper functions.
inline constexpr RefVariant genNullRef(const RefType Type) noexcept {
  return UINT64_C(0);
}
inline constexpr RefVariant genFuncRef(const uint32_t Idx) noexcept {
  return FuncRef{1, Idx};
}
template <typename T> inline RefVariant genExternRef(T *Ref) noexcept {
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
template <> inline ValType ValTypeFromType<uint128_t>() noexcept {
  return ValType::V128;
}
template <> inline ValType ValTypeFromType<int128_t>() noexcept {
  return ValType::V128;
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
  case ValType::V128:
    return uint128_t(0U);
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
