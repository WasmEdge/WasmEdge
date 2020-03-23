// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>
#include <type_traits>

namespace SSVM {
namespace Support {

/// Remove const, reference, and volitile.
template <typename T>
using RemoveCVRefT = std::remove_cv_t<std::remove_reference_t<T>>;

/// Return true if Wasm unsign (uint32_t and uint64_t).
template <typename T>
struct IsWasmUnsign
    : std::bool_constant<std::is_same_v<RemoveCVRefT<T>, uint32_t> ||
                         std::is_same_v<RemoveCVRefT<T>, uint64_t>> {};
template <typename T>
inline constexpr const bool IsWasmUnsignV = IsWasmUnsign<T>::value;

/// Return true if Wasm sign (int32_t and int64_t).
template <typename T>
struct IsWasmSign
    : std::bool_constant<std::is_same_v<RemoveCVRefT<T>, int32_t> ||
                         std::is_same_v<RemoveCVRefT<T>, int64_t>> {};
template <typename T>
inline constexpr const bool IsWasmSignV = IsWasmSign<T>::value;

/// Return true if Wasm float (float and double).
template <typename T>
struct IsWasmFloat
    : std::bool_constant<std::is_same_v<RemoveCVRefT<T>, float> ||
                         std::is_same_v<RemoveCVRefT<T>, double>> {};
template <typename T>
inline constexpr const bool IsWasmFloatV = IsWasmFloat<T>::value;

/// Return true if Wasm int (int32_t, uint32_t, int64_t, uint64_t).
template <typename T>
struct IsWasmInt : std::bool_constant<IsWasmSignV<T> || IsWasmUnsignV<T>> {};
template <typename T>
inline constexpr const bool IsWasmIntV = IsWasmInt<T>::value;

/// Return true if Wasm int or Wasm float.
template <typename T>
struct IsWasmType : std::bool_constant<IsWasmIntV<T> || IsWasmFloatV<T>> {};
template <typename T>
inline constexpr const bool IsWasmTypeV = IsWasmType<T>::value;

/// Return true if Wasm built-in types (T is uint32_t, uint64_t, float, double).
template <typename T>
struct IsWasmBuiltIn : std::bool_constant<IsWasmUnsignV<T> || IsWasmFloatV<T>> {
};
template <typename T>
inline constexpr const bool IsWasmBuiltInV = IsWasmBuiltIn<T>::value;

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
typename std::enable_if_t<IsWasmTypeV<T>, MakeWasmSignedT<T>> toSigned(T Val) {
  return static_cast<MakeWasmSignedT<T>>(Val);
}

/// Cast-to-unsigned function.
template <typename T>
typename std::enable_if_t<IsWasmTypeV<T>, MakeWasmUnsignedT<T>>
toUnsigned(T Val) {
  return static_cast<MakeWasmUnsignedT<T>>(Val);
}

template <typename T> struct TypeToWasmType { using type = T; };
template <> struct TypeToWasmType<int32_t> { using type = uint32_t; };
template <> struct TypeToWasmType<int64_t> { using type = uint64_t; };
template <typename T>
using TypeToWasmTypeT =
    typename std::enable_if_t<IsWasmTypeV<T>, typename TypeToWasmType<T>::type>;

} // namespace Support
} // namespace SSVM
