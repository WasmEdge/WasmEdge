// SPDX-License-Identifier: Apache-2.0
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

#include "enum_types.h"
#include "int128.h"

#include <cstdint>
#include <type_traits>
#include <variant>

namespace WasmEdge {

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

/// BlockType definition.
using BlockType = std::variant<ValType, uint32_t>;

/// UnknownRef definition.
struct UnknownRef {
  uint64_t Value = 0;
  UnknownRef() = default;
};

/// FuncRef definition.
struct FuncRef {
  uint32_t NotNull = 0;
  uint32_t Idx = 0;
  FuncRef() = default;
  FuncRef(uint32_t I) : NotNull(1), Idx(I) {}
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

/// NumType and RefType conversions.
inline constexpr ValType ToValType(const NumType Val) noexcept {
  return static_cast<ValType>(Val);
}
inline constexpr ValType ToValType(const RefType Val) noexcept {
  return static_cast<ValType>(Val);
}

///
/// The followings are const expressions to checking types.
///

namespace {
/// Remove const, reference, and volitile.
template <typename T>
using RemoveCVRefT = std::remove_cv_t<std::remove_reference_t<T>>;
} // namespace

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

/// Cast-to-signed function.
template <typename T>
typename std::enable_if_t<IsWasmNumV<T>, MakeWasmSignedT<T>> toSigned(T Val) {
  return static_cast<MakeWasmSignedT<T>>(Val);
}

/// Cast-to-unsigned function.
template <typename T>
typename std::enable_if_t<IsWasmNumV<T>, MakeWasmUnsignedT<T>>
toUnsigned(T Val) {
  return static_cast<MakeWasmUnsignedT<T>>(Val);
}

template <typename T> struct TypeToWasmType { using type = T; };
template <> struct TypeToWasmType<int32_t> { using type = uint32_t; };
template <> struct TypeToWasmType<int64_t> { using type = uint64_t; };
template <> struct TypeToWasmType<int128_t> { using type = uint128_t; };
template <> struct TypeToWasmType<int64x2_t> { using type = uint64x2_t; };
template <> struct TypeToWasmType<int32x4_t> { using type = uint32x4_t; };
template <> struct TypeToWasmType<int16x8_t> { using type = uint16x8_t; };
template <> struct TypeToWasmType<int8x16_t> { using type = uint8x16_t; };
template <typename T>
using TypeToWasmTypeT =
    typename std::enable_if_t<IsWasmValV<T>, typename TypeToWasmType<T>::type>;

} // namespace WasmEdge
