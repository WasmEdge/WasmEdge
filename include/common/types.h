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

#include "common/enum_types.h"
#include "common/errcode.h"
#include "common/int128.h"
#include "common/variant.h"

#include <cstdint>
#include <type_traits>
#include <variant>
#include <vector>
#include <string>

namespace WasmEdge {

namespace {

/// Remove const, reference, and volitile.
template <typename T>
using RemoveCVRefT = std::remove_cv_t<std::remove_reference_t<T>>;

} // namespace

// >>>>>>>> Type definitions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

using Byte = uint8_t;
using S8 = int8_t;
using U8 = uint8_t;
using S16 = int16_t;
using U16 = uint16_t;
using S32 = int32_t;
using U32 = uint32_t;
using S64 = int64_t;
using U64 = uint64_t;
using Float32 = float ;
using Float64 = double;
using Char = char;
using String = std::string;
using Bool = bool;
//using Unit = ;

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
// Implementation of "record" type in interface types
struct Record{
  RecordField *field = new RecordField;
};

struct RecordField {
  std::string name;
  InterfaceType ty;
};

// Implementation of "variant" type in interface types 
struct Variants{
  VariantCase *cases = new VariantCase;
};

struct VariantCase {
  std::string name;
  InterfaceType ty;
};

// Implementation of "tuple" type in interface types 
 struct Tuple{
 InterfaceType *tyTup = new InterfaceType;
 };

// Implementation of "flags" type in interface types 
struct Flags{
std::string *names = new std::string;
};

// Implementation of "enum" type in interface types 
struct Enum{
 std::string *names = new std::string;
};

//Implentation of "union" type in interface types 
struct Union{
  InterfaceType *tyUn = new InterfaceType;
}; 

//Implementation of "expected" type in interface types 
struct Expecteds{
  InterfaceType tyEx;
  InterfaceType err;
};

/// NumType and RefType variant definitions.
using RefVariant = Variant<UnknownRef, FuncRef, ExternRef>;
using InterVariant = Variant<//Unit,
    Bool,
    S8,
    U8,
    S16,
    U16,
    S32,
    U32,
    S64,
    U64,
    Float32,
    Float64,
    Char,
    String, 
    Record,
    Variants,
    //List,
    Tuple,
    Flags,
    Enum, 
    Union, 
    //Option, 
    Expecteds>;
using ValVariant =
    Variant<uint32_t, int32_t, uint64_t, int64_t, float, double, uint128_t,
            int128_t, uint64x2_t, int64x2_t, uint32x4_t, int32x4_t, uint16x8_t,
            int16x8_t, uint8x16_t, int8x16_t, floatx4_t, doublex2_t, UnknownRef,
            FuncRef, ExternRef,Bool, S8, U8, S16, U16, S32, U32, S64, U64, Float32, Float64, 
            Char, String, Record, Variants, Tuple, Flags, Enums, Union, Expecteds>;

/// BlockType definition.
struct BlockType {
  bool IsValType;
  union {
    ValType Type;
    uint32_t Idx;
  } Data;
  BlockType() = default;
  BlockType(ValType VType) { setData(VType); }
  BlockType(uint32_t Idx) { setData(Idx); }
  void setData(ValType VType) {
    IsValType = true;
    Data.Type = VType;
  }
  void setData(uint32_t Idx) {
    IsValType = false;
    Data.Idx = Idx;
  }
};



/// NumType and RefType conversions.
inline constexpr ValType ToValType(const NumType Val) noexcept {
  return static_cast<ValType>(Val);
}
inline constexpr ValType ToValType(const RefType Val) noexcept {
  return static_cast<ValType>(Val);
}
inline constexpr ValType ToValType(const InterfaceType Val) noexcept {
  return static_cast<ValType>(Val);
}

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

/// Return true if Wasm Interface type 
template <typename T>
struct IsWasmInter 
    : std::bool_constant<std::is_same_v<RemoveCVRef<T>, Bool> ||
                         std::is_same_v<RemoveCVRef<T>, S8> ||
                         std::is_same_v<RemoveCVRef<T>, U8> ||
                         std::is_same_v<RemoveCVRef<T>, S16> ||
                         std::is_same_v<RemoveCVRef<T>, U16> ||
                         std::is_same_v<RemoveCVRef<T>, S32> ||
                         std::is_same_v<RemoveCVRef<T>, U32> ||
                         std::is_same_v<RemoveCVRef<T>, S64> ||
                         std::is_same_v<RemoveCVRef<T>, U64> ||
                         std::is_same_v<RemoveCVRef<T>, Float32> ||
                         std::is_same_v<RemoveCVRef<T>, Float64> ||
                         std::is_same_v<RemoveCVRef<T>, Char> ||
                         std::is_same_v<RemoveCVRef<T>, String> ||
                         std::is_same_v<RemoveCVRef<T>, Record> ||
                         std::is_same_v<RemoveCVRef<T>, Variants> ||
                         std::is_same_v<RemoveCVRef<T>, Tuple> ||
                         std::is_same_v<RemoveCVRef<T>, Flags> ||
                         std::is_same_v<RemoveCVRef<T>, Enum> ||
                         std::is_same_v<RemoveCVRef<T>, Union> ||
                         std::is_same_v<RemoveCVRef<T>, Expecteds>> {};
    

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

// <<<<<<<< Const expressions to checking value types <<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> Template to get value type from type >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

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
template <> inline ValType ValTypeFromType<Bool>() noexcept {
  return ValType::Bool;
}
template <> inline ValType ValTypeFromType<S8>() noexcept {
  return ValType::S8;
}
template <> inline ValType ValTypeFromType<U8>() noexcept {
  return ValType::U8;
}
template <> inline ValType ValTypeFromType<S16>() noexcept {
  return ValType::S16;
}
template <> inline ValType ValTypeFromType<U16>() noexcept {
  return ValType::U16;
}
template <> inline ValType ValTypeFromType<S32>() noexcept {
  return ValType::S32;
}
template <> inline ValType ValTypeFromType<U32>() noexcept {
  return ValType::U32;
}
template <> inline ValType ValTypeFromType<S64>() noexcept {
  return ValType::S64;
}
template <> inline ValType ValTypeFromType<U64>() noexcept {
  return ValType::U64;
}
template <> inline ValType ValTypeFromType<Float32>() noexcept {
  return ValType::Float32;
}
template <> inline ValType ValTypeFromType<Float64>() noexcept {
  return ValType::Float64;
}
template <> inline ValType ValTypeFromType<Char>() noexcept {
  return ValType::Char;
}
template <> inline ValType ValTypeFromType<String>() noexcept {
  return ValType::String;
}
template <> inline ValType ValTypeFromType<Record>() noexcept {
  return ValType::Record;
}
template <> inline ValType ValTypeFromType<Variants>() noexcept {
  return ValType::Char;
}
template <> inline ValType ValTypeFromType<Tuple>() noexcept {
  return ValType::Tuple;
}
template <> inline ValType ValTypeFromType<Flags>() noexcept {
  return ValType::Flags;
}
template <> inline ValType ValTypeFromType<Enum>() noexcept {
  return ValType::Enum;
}
template <> inline ValType ValTypeFromType<Union>() noexcept {
  return ValType::Union;
}
template <> inline ValType ValTypeFromType<Expecteds>() noexcept {
  return ValType::Expecteds;
}
// <<<<<<<< Template to get value type from type <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> Const expression to generate value from value type >>>>>>>>>>>>>>>>>

inline constexpr ValVariant ValueFromType(ValType Type) noexcept {
  switch (Type) {
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
  case ValType::Bool:
    return bool(1);
  case ValType::S8:
    return int8_t(0S);
  case ValType::U8:
    return uint8_t(0U);
  case ValType::S16:
    return int16_t(0S);
  case ValType::U16:
    return uint16_t(0U);
  case ValType::S32:
    return int32_t(0S);
  case ValType::U32:
    return uint32_t(0U);
  case ValType::S64:
    return int64_t(0S);
  case ValType::U64:
    return uint64_t(0U);
  case ValType::Float32:
    return float(0.0F);
  case ValType::Float64:
    return double(0.0);
  case ValType::Char:
    return char('0');
  case ValType::String:
    return string("0");


  case ValType::FuncRef:
  case ValType::ExternRef:
    return UnknownRef();
  case ValType::None:
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
