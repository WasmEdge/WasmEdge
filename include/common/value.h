// SPDX-License-Identifier: Apache-2.0
//===-- wasmedge/common/value.h - WasmEdge value variant definition -------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the value struct used in WasmEdge.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "types.h"
#include "variant.h"

#include <cstdint>
#include <vector>

namespace WasmEdge {

/// Definition of number_type.
using RefVariant = Variant<UnknownRef, FuncRef, ExternRef>;
using ValVariant =
    Variant<uint32_t, int32_t, uint64_t, int64_t, float, double, uint128_t,
            int128_t, uint64x2_t, int64x2_t, uint32x4_t, int32x4_t, uint16x8_t,
            int16x8_t, uint8x16_t, int8x16_t, floatx4_t, doublex2_t, UnknownRef,
            FuncRef, ExternRef>;
using Byte = uint8_t;

/// Reference types helper functions.
inline constexpr UnknownRef genNullRef(const RefType /*Type*/) noexcept {
  return UnknownRef();
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
  case ValType::None:
    __builtin_unreachable();
  }
  __builtin_unreachable();
}

/// Retrieve references.
inline constexpr bool isNullRef(const ValVariant &Val) {
  return Val.get<UnknownRef>().Value == 0;
}
inline constexpr bool isNullRef(const RefVariant &Val) {
  return Val.get<UnknownRef>().Value == 0;
}
inline constexpr uint32_t retrieveFuncIdx(const ValVariant &Val) {
  return Val.get<FuncRef>().Idx;
}
inline constexpr uint32_t retrieveFuncIdx(const RefVariant &Val) {
  return Val.get<FuncRef>().Idx;
}
inline constexpr uint32_t retrieveFuncIdx(const FuncRef &Val) {
  return Val.Idx;
}
template <typename T> inline T &retrieveExternRef(const ValVariant &Val) {
  return *reinterpret_cast<T *>(Val.get<ExternRef>().Ptr);
}
template <typename T> inline T &retrieveExternRef(const RefVariant &Val) {
  return *reinterpret_cast<T *>(Val.get<ExternRef>().Ptr);
}
template <typename T> inline T &retrieveExternRef(const ExternRef &Val) {
  return *reinterpret_cast<T *>(Val.Ptr);
}

} // namespace WasmEdge
