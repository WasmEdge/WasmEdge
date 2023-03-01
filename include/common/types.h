// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasmedge/common/types.h - Types definition ------------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the enumerations of Wasm VM used types and the type
/// recognition templates.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/enum_types.hpp"
#include "common/errcode.h"
#include "common/int128.h"
#include "common/variant.h"

#include <cstdint>
#include <type_traits>
#include <variant>
#include <vector>

namespace WasmEdge {

namespace {

/// Remove const, reference, and volitile.
template <typename T>
using RemoveCVRefT = std::remove_cv_t<std::remove_reference_t<T>>;

} // namespace

// >>>>>>>> Type definitions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

using Byte = uint8_t;

/// SIMD types definition.
using int64x2_t [[gnu::vector_size(16)]] = int64_t;
using uint64x2_t [[gnu::vector_size(16)]] = uint64_t;
using int32x4_t [[gnu::vector_size(16)]] = int32_t;
using uint32x4_t [[gnu::vector_size(16)]] = uint32_t;
using int16x8_t [[gnu::vector_size(16)]] = int16_t;
using uint16x8_t [[gnu::vector_size(16)]] = uint16_t;
using int8x16_t [[gnu::vector_size(16)]] = int8_t;
using uint8x16_t [[gnu::vector_size(16)]] = uint8_t;
using doublex2_t [[gnu::vector_size(16)]] = double;
using floatx4_t [[gnu::vector_size(16)]] = float;

/// UnknownRef definition.
struct UnknownRef {
  uint64_t Value = 0;
  UnknownRef() = default;
};

/// FuncRef definition.
namespace Runtime::Instance {
class FunctionInstance;
}
struct FuncRef {
#if __INTPTR_WIDTH__ == 32
  const uint32_t Padding = -1;
#endif
  const Runtime::Instance::FunctionInstance *Ptr = nullptr;
  FuncRef() = default;
  FuncRef(const Runtime::Instance::FunctionInstance *P) : Ptr(P) {}
};

/// ExternRef definition.
struct ExternRef {
#if __INTPTR_WIDTH__ == 32
  const uint32_t Padding = -1;
#endif
  void *Ptr = nullptr;
  ExternRef() = default;
  template <typename T> ExternRef(T *P) : Ptr(reinterpret_cast<void *>(P)) {}
};

/// NumType and RefType variant definitions.
using RefVariant = Variant<UnknownRef, FuncRef, ExternRef>;
using ValVariant =
    Variant<uint32_t, int32_t, uint64_t, int64_t, float, double, uint128_t,
            int128_t, uint64x2_t, int64x2_t, uint32x4_t, int32x4_t, uint16x8_t,
            int16x8_t, uint8x16_t, int8x16_t, floatx4_t, doublex2_t, UnknownRef,
            FuncRef, ExternRef>;

/// BlockType definition.
struct BlockType {
  enum class TypeEnum : uint8_t {
    Empty,
    ValType,
    TypeIdx,
  };
  TypeEnum TypeFlag;
  union {
    FullValType Type;
    uint32_t Idx;
  } Data;
  BlockType() = default;
  BlockType(FullValType VType) { setData(VType); }
  BlockType(uint32_t Idx) { setData(Idx); }
  void setEmpty() { TypeFlag = TypeEnum::Empty; }
  void setData(FullValType VType) {
    TypeFlag = TypeEnum::ValType;
    Data.Type = VType;
  }
  void setData(uint32_t Idx) {
    TypeFlag = TypeEnum::TypeIdx;
    Data.Idx = Idx;
  }
  bool isEmpty() const { return TypeFlag == TypeEnum::Empty; }
  bool isValType() const { return TypeFlag == TypeEnum::ValType; }
};

// <<<<<<<< Type definitions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> Const expressions to checking value types >>>>>>>>>>>>>>>>>>>>>>>>>>

/// Return true if Wasm unsign (uint32_t and uint64_t).
template <typename T>
struct IsWasmUnsign
    : std::bool_constant<std::is_same_v<RemoveCVRefT<T>, uint32_t> ||
                         std::is_same_v<RemoveCVRefT<T>, uint64_t> ||
                         std::is_same_v<RemoveCVRefT<T>, uint128_t> ||
                         std::is_same_v<RemoveCVRefT<T>, uint64x2_t> ||
                         std::is_same_v<RemoveCVRefT<T>, uint32x4_t> ||
                         std::is_same_v<RemoveCVRefT<T>, uint16x8_t> ||
                         std::is_same_v<RemoveCVRefT<T>, uint8x16_t>> {};
template <typename T>
inline constexpr const bool IsWasmUnsignV = IsWasmUnsign<T>::value;

/// Return true if Wasm sign (int32_t and int64_t).
template <typename T>
struct IsWasmSign
    : std::bool_constant<std::is_same_v<RemoveCVRefT<T>, int32_t> ||
                         std::is_same_v<RemoveCVRefT<T>, int64_t> ||
                         std::is_same_v<RemoveCVRefT<T>, int128_t> ||
                         std::is_same_v<RemoveCVRefT<T>, int64x2_t> ||
                         std::is_same_v<RemoveCVRefT<T>, int32x4_t> ||
                         std::is_same_v<RemoveCVRefT<T>, int16x8_t> ||
                         std::is_same_v<RemoveCVRefT<T>, int8x16_t>> {};
template <typename T>
inline constexpr const bool IsWasmSignV = IsWasmSign<T>::value;

/// Return true if Wasm float (float and double).
template <typename T>
struct IsWasmFloat
    : std::bool_constant<std::is_same_v<RemoveCVRefT<T>, float> ||
                         std::is_same_v<RemoveCVRefT<T>, double> ||
                         std::is_same_v<RemoveCVRefT<T>, floatx4_t> ||
                         std::is_same_v<RemoveCVRefT<T>, doublex2_t>> {};
template <typename T>
inline constexpr const bool IsWasmFloatV = IsWasmFloat<T>::value;

/// Return true if Wasm reference (funcref and externref).
template <typename T>
struct IsWasmRef
    : std::bool_constant<std::is_same_v<RemoveCVRefT<T>, FuncRef> ||
                         std::is_same_v<RemoveCVRefT<T>, ExternRef>> {};
template <typename T>
inline constexpr const bool IsWasmRefV = IsWasmRef<T>::value;

/// Return true if Wasm int (int32_t, uint32_t, int64_t, uint64_t).
template <typename T>
struct IsWasmInt : std::bool_constant<IsWasmSignV<T> || IsWasmUnsignV<T>> {};
template <typename T>
inline constexpr const bool IsWasmIntV = IsWasmInt<T>::value;

/// Return true if Wasm int or Wasm float.
template <typename T>
struct IsWasmNum : std::bool_constant<IsWasmIntV<T> || IsWasmFloatV<T>> {};
template <typename T>
inline constexpr const bool IsWasmNumV = IsWasmNum<T>::value;

/// Return true if Wasm native num types (uint32_t, uint64_t, float, double).
template <typename T>
struct IsWasmNativeNum
    : std::bool_constant<IsWasmUnsignV<T> || IsWasmFloatV<T>> {};
template <typename T>
inline constexpr const bool IsWasmNativeNumV = IsWasmNativeNum<T>::value;

/// Return true if Wasm value types (num types and reference types).
template <typename T>
struct IsWasmVal : std::bool_constant<IsWasmNumV<T> || IsWasmRefV<T>> {};
template <typename T>
inline constexpr const bool IsWasmValV = IsWasmVal<T>::value;

/// Return signed type.
template <typename T>
using MakeWasmSignedT =
    typename std::conditional<IsWasmFloatV<T>, std::common_type<T>,
                              std::make_signed<T>>::type::type;

/// Return unsigned type.
template <typename T>
using MakeWasmUnsignedT =
    typename std::conditional<IsWasmFloatV<T>, std::common_type<T>,
                              std::make_unsigned<T>>::type::type;

/// Cast-to-signed function.
template <typename T>
typename std::enable_if_t<IsWasmNumV<T>, MakeWasmSignedT<T>> toSigned(T Val) {
  return static_cast<MakeWasmSignedT<T>>(Val);
}

/// Cast-to-unsigned function.
template <typename T>
typename std::enable_if_t<IsWasmNumV<T>, MakeWasmUnsignedT<T>>
toUnsigned(T Val) {
  return static_cast<MakeWasmUnsignedT<T>>(Val);
}

// <<<<<<<< Const expressions to checking value types <<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> Template to get value type from type >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

template <typename T> inline FullValType ValTypeFromType() noexcept;

template <> inline FullValType ValTypeFromType<uint32_t>() noexcept {
  return FullValType(ValType::I32);
}
template <> inline FullValType ValTypeFromType<int32_t>() noexcept {
  return FullValType(ValType::I32);
}
template <> inline FullValType ValTypeFromType<uint64_t>() noexcept {
  return FullValType(ValType::I64);
}
template <> inline FullValType ValTypeFromType<int64_t>() noexcept {
  return FullValType(ValType::I64);
}
template <> inline FullValType ValTypeFromType<uint128_t>() noexcept {
  return FullValType(ValType::V128);
}
template <> inline FullValType ValTypeFromType<int128_t>() noexcept {
  return FullValType(ValType::V128);
}
template <> inline FullValType ValTypeFromType<float>() noexcept {
  return FullValType(ValType::F32);
}
template <> inline FullValType ValTypeFromType<double>() noexcept {
  return FullValType(ValType::F64);
}
template <> inline FullValType ValTypeFromType<FuncRef>() noexcept {
  return FullValType(ValType::FuncRef);
}
template <> inline FullValType ValTypeFromType<ExternRef>() noexcept {
  return FullValType(ValType::ExternRef);
}

// <<<<<<<< Template to get value type from type <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> Const expression to generate value from value type >>>>>>>>>>>>>>>>>

inline ValVariant ValueFromType(FullValType Type) noexcept {
  switch (Type.getTypeCode()) {
  case ValTypeCode::I32:
    return uint32_t(0U);
  case ValTypeCode::I64:
    return uint64_t(0U);
  case ValTypeCode::F32:
    return float(0.0F);
  case ValTypeCode::F64:
    return double(0.0);
  case ValTypeCode::V128:
    return uint128_t(0U);
  case ValTypeCode::Ref:
  case ValTypeCode::RefNull:
    return UnknownRef();
  default:
    assumingUnreachable();
  }
}

// <<<<<<<< Const expression to generate value from value type <<<<<<<<<<<<<<<<<

// >>>>>>>> Functions to retrieve reference inners >>>>>>>>>>>>>>>>>>>>>>>>>>>>>

inline constexpr bool isNullRef(const ValVariant &Val) {
  return Val.get<UnknownRef>().Value == 0;
}
inline constexpr bool isNullRef(const RefVariant &Val) {
  return Val.get<UnknownRef>().Value == 0;
}

inline const Runtime::Instance::FunctionInstance *
retrieveFuncRef(const ValVariant &Val) {
  return reinterpret_cast<const Runtime::Instance::FunctionInstance *>(
      Val.get<FuncRef>().Ptr);
}
inline const Runtime::Instance::FunctionInstance *
retrieveFuncRef(const RefVariant &Val) {
  return reinterpret_cast<const Runtime::Instance::FunctionInstance *>(
      Val.get<FuncRef>().Ptr);
}
inline const Runtime::Instance::FunctionInstance *
retrieveFuncRef(const FuncRef &Val) {
  return reinterpret_cast<const Runtime::Instance::FunctionInstance *>(Val.Ptr);
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

// <<<<<<<< Functions to retrieve reference inners <<<<<<<<<<<<<<<<<<<<<<<<<<<<<

} // namespace WasmEdge
