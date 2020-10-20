// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/common/types.h - Types definition ------------------*- C++ -*-===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the enumerations of Wasm VM used types.
///
//===----------------------------------------------------------------------===//
#pragma once

#include <cstdint>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <variant>

namespace SSVM {

namespace {
/// Remove const, reference, and volitile.
template <typename T>
using RemoveCVRefT = std::remove_cv_t<std::remove_reference_t<T>>;
} // namespace

/// Value types enumeration class.
enum class ValType : uint8_t {
  None = 0x40,
  I32 = 0x7F,
  I64 = 0x7E,
  F32 = 0x7D,
  F64 = 0x7C,
  FuncRef = 0x70,
  ExternRef = 0x6F
};

static inline std::unordered_map<ValType, std::string> ValTypeStr = {
    {ValType::None, "none"},
    {ValType::I32, "i32"},
    {ValType::I64, "i64"},
    {ValType::F32, "f32"},
    {ValType::F64, "f64"},
    {ValType::FuncRef, "funcref"},
    {ValType::ExternRef, "externref"}};

/// Block type definition.
using BlockType = std::variant<ValType, uint32_t>;

/// FuncRef definition.
struct FuncRef {
  uint32_t NotNull = 0;
  uint32_t Idx = 0;
};

/// ExternRef definition.
struct ExternRef {
  uint64_t *Ptr = nullptr;
};

/// Number types enumeration class.
enum class NumType : uint8_t { I32 = 0x7F, I64 = 0x7E, F32 = 0x7D, F64 = 0x7C };
inline constexpr ValType ToValType(const NumType Val) noexcept {
  return static_cast<ValType>(Val);
}

/// Reference types enumeration class.
enum class RefType : uint8_t { ExternRef = 0x6F, FuncRef = 0x70 };
inline constexpr ValType ToValType(const RefType Val) noexcept {
  return static_cast<ValType>(Val);
}

/// Value mutability enumeration class.
enum class ValMut : uint8_t { Const = 0x00, Var = 0x01 };

static inline std::unordered_map<ValMut, std::string> ValMutStr = {
    {ValMut::Const, "const"}, {ValMut::Var, "var"}};

/// External type enumeration class.
enum class ExternalType : uint8_t {
  Function = 0x00U,
  Table = 0x01U,
  Memory = 0x02U,
  Global = 0x03U
};

static inline std::unordered_map<ExternalType, std::string> ExternalTypeStr = {
    {ExternalType::Function, "function"},
    {ExternalType::Table, "table"},
    {ExternalType::Memory, "memory"},
    {ExternalType::Global, "global"}};

///
/// The following are const expressions to checking types.
///
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
template <typename T>
using TypeToWasmTypeT =
    typename std::enable_if_t<IsWasmValV<T>, typename TypeToWasmType<T>::type>;

} // namespace SSVM
