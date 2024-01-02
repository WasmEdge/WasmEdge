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

#include <array>
#include <cstdint>
#include <type_traits>
#include <variant>

namespace WasmEdge {

namespace {

/// Remove const, reference, and volitile.
template <typename T>
using RemoveCVRefT = std::remove_cv_t<std::remove_reference_t<T>>;

} // namespace

// >>>>>>>> Type definitions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

using Byte = uint8_t;

/// SIMD types definition.
template <typename Ty, size_t TotalSize,
          std::enable_if_t<(TotalSize % sizeof(Ty) == 0), int> = 0>
#if defined(_MSC_VER) && !defined(__clang__)
/// Because MSVC does not support [[gnu::vector_size(16)]] or
/// __attribute__((vector_size(16)), we use this type to fill the gap.
using SIMDArray = std::array<Ty, (TotalSize / sizeof(Ty))>;
#else
using SIMDArray [[gnu::vector_size(TotalSize)]] = Ty;
#endif

using int64x2_t = SIMDArray<int64_t, 16>;
using uint64x2_t = SIMDArray<uint64_t, 16>;
using int32x4_t = SIMDArray<int32_t, 16>;
using uint32x4_t = SIMDArray<uint32_t, 16>;
using int16x8_t = SIMDArray<int16_t, 16>;
using uint16x8_t = SIMDArray<uint16_t, 16>;
using int8x16_t = SIMDArray<int8_t, 16>;
using uint8x16_t = SIMDArray<uint8_t, 16>;
using doublex2_t = SIMDArray<double, 16>;
using floatx4_t = SIMDArray<float, 16>;

// The bit pattern of the value types:
// ----------------------------------------------------------------------------
//  byte | 0th | 1st |      2nd     |         3rd         |      4th ~ 7th
// ------|-----------|--------------|---------------------|--------------------
//       |           | ValTypeCode  |         For the HeapType use
//       |           | 0x7F, 0x7E,  | (Function references and GC proposal)
//       |           | 0x7D, 0x7C,  | HeapTypeCode        |
//       |           |   (numtype)  | 0x00, 0x40,         |    Type index
//  code | Reserved  | 0x7B,        | 0x70, 0x6F,         |    (uint32_t)
//       | (Padding) |   (vectype)  | (func-ref proposal) |
//       |           | 0x78, 0x77,  | 0x73, 0x72, 0x71,   |
//       |           | (packedtype) | 0x6E, 0x6D, 0x6C,   |
//       |           | 0x64, 0x63   | 0x6B, 0x6A          |
//       |           |   (reftype)  |    (GC proposal)    |
// ----------------------------------------------------------------------------
// In order to compress the various value type definitions into uint64_t length,
// WasmEdge implements the ValType class for extending the value types.
// As the definitions in the typed function references and GC proposal, the
// `FuncRef` and `ExternRef` are reinterpreted as the `ref.null` types,
// respectively. Therefore, WasmEdge hendles them into `ref null func` and `ref
// null extern` in the ValType classes.

/// ValType class definition.
class ValType {
public:
  // Note: The padding bytes are reserved and should not be written.
  ValType() noexcept = default;
  // General constructors for initializing data.
  ValType(TypeCode C, TypeCode HT, uint32_t I) noexcept {
    Inner.Data.Code = C;
    Inner.Data.HTCode = HT;
    Inner.Data.Idx = I;
  }
  ValType(const std::array<uint8_t, 8> R) noexcept {
    std::copy_n(R.cbegin(), 8, Inner.Raw);
  }
  // Constructor for the value type codes without heap type immediates.
  ValType(TypeCode C) noexcept {
    Inner.Data.Idx = 0;
    switch (C) {
    case TypeCode::I32:
    case TypeCode::I64:
    case TypeCode::F32:
    case TypeCode::F64:
      // Number type
    case TypeCode::V128:
      // Vector type
    case TypeCode::I8:
    case TypeCode::I16:
      // Packed type
      Inner.Data.Code = C;
      Inner.Data.HTCode = TypeCode::Epsilon;
      break;
    case TypeCode::NullFuncRef:
    case TypeCode::NullExternRef:
    case TypeCode::NullRef:
    case TypeCode::FuncRef:
    case TypeCode::ExternRef:
    case TypeCode::AnyRef:
    case TypeCode::EqRef:
    case TypeCode::I31Ref:
    case TypeCode::StructRef:
    case TypeCode::ArrayRef:
      // Abstract heap type
      Inner.Data.Code = TypeCode::RefNull;
      Inner.Data.HTCode = C;
      break;
    case TypeCode::Ref:
    case TypeCode::RefNull:
      // Reference type with heap immediates should use the constructors below.
    default:
      assumingUnreachable();
    }
  }
  // Constructor for the value type with abs heap type in reference type.
  ValType(TypeCode C, TypeCode HT) noexcept {
    Inner.Data.Code = C;
    Inner.Data.HTCode = HT;
    Inner.Data.Idx = 0;
    assuming(isAbsHeapType());
    assuming(isRefType());
  }
  // Constructor for the value type with type index in reference type.
  ValType(TypeCode C, uint32_t I) noexcept {
    Inner.Data.Code = C;
    Inner.Data.HTCode = TypeCode::TypeIndex;
    Inner.Data.Idx = I;
    assuming(isRefType());
  }

  friend bool operator==(const ValType &LHS, const ValType &RHS) noexcept {
    return (LHS.Inner.Data.Code == RHS.Inner.Data.Code) &&
           (LHS.Inner.Data.HTCode == RHS.Inner.Data.HTCode) &&
           (LHS.Inner.Data.Idx == RHS.Inner.Data.Idx);
  }
  friend bool operator!=(const ValType &LHS, const ValType &RHS) noexcept {
    return !(LHS == RHS);
  }

  TypeCode getCode() const noexcept { return Inner.Data.Code; }
  TypeCode getHeapTypeCode() const noexcept { return Inner.Data.HTCode; }
  uint32_t getTypeIndex() const noexcept { return Inner.Data.Idx; }
  const std::array<uint8_t, 8> getRawData() const noexcept {
    std::array<uint8_t, 8> R;
    std::copy_n(Inner.Raw, 8, R.begin());
    return R;
  }

  bool isDefaultable() const noexcept {
    return Inner.Data.Code != TypeCode::Ref;
  }

  bool isNumType() const noexcept {
    switch (Inner.Data.Code) {
    case TypeCode::I32:
    case TypeCode::I64:
    case TypeCode::F32:
    case TypeCode::F64:
    case TypeCode::V128:
      return true;
    default:
      return false;
    }
  }

  bool isRefType() const noexcept {
    switch (Inner.Data.Code) {
    case TypeCode::Ref:
    case TypeCode::RefNull:
      return true;
    default:
      return false;
    }
  }

  bool isPackType() const noexcept {
    switch (Inner.Data.Code) {
    case TypeCode::I8:
    case TypeCode::I16:
      return true;
    default:
      return false;
    }
  }

  bool isFuncRefType() const noexcept {
    return (Inner.Data.HTCode == TypeCode::FuncRef) ||
           (Inner.Data.HTCode == TypeCode::TypeIndex);
  }

  bool isExternRefType() const noexcept {
    return (Inner.Data.HTCode == TypeCode::ExternRef);
  }

  bool isNullableRefType() const noexcept {
    return (Inner.Data.Code == TypeCode::RefNull);
  }

  bool isAbsHeapType() const noexcept {
    switch (Inner.Data.HTCode) {
    case TypeCode::NullFuncRef:
    case TypeCode::NullExternRef:
    case TypeCode::NullRef:
    case TypeCode::FuncRef:
    case TypeCode::ExternRef:
    case TypeCode::AnyRef:
    case TypeCode::EqRef:
    case TypeCode::I31Ref:
    case TypeCode::StructRef:
    case TypeCode::ArrayRef:
      return true;
    default:
      return false;
    }
  }

protected:
  union {
    uint8_t Raw[8];
    struct {
      uint8_t Paddings[2];
      TypeCode Code;
      TypeCode HTCode;
      uint32_t Idx;
    } Data;
  } Inner;
};

/// BlockType definition.
class BlockType {
public:
  // Note: The BlockType should be compressed into 8 bytes to reduce the
  // instruction class size.
  enum class TypeEnum : uint8_t {
    Empty,
    ValType,
    TypeIdx,
  };

  BlockType() noexcept = default;
  BlockType(const ValType &VType) noexcept { setData(VType); }
  BlockType(uint32_t Idx) noexcept { setData(Idx); }

  void setEmpty() noexcept { Inner.Data.TypeFlag = TypeEnum::Empty; }
  void setData(const ValType &VType) noexcept {
    Inner.Type = VType;
    Inner.Data.TypeFlag = TypeEnum::ValType;
  }
  void setData(uint32_t Idx) noexcept {
    Inner.Data.Idx = Idx;
    Inner.Data.TypeFlag = TypeEnum::TypeIdx;
  }
  bool isEmpty() const noexcept {
    return Inner.Data.TypeFlag == TypeEnum::Empty;
  }
  bool isValType() const noexcept {
    return Inner.Data.TypeFlag == TypeEnum::ValType;
  }
  ValType getValType() const noexcept { return Inner.Type; }
  uint32_t getTypeIndex() const noexcept { return Inner.Data.Idx; }

private:
  // The ValType has reserved the padding 2 bytes.
  // Therefore, use the first byte to store the flag.
  union {
    // The ValType has 8 bytes length.
    ValType Type;
    // The Data struct has 8 bytes length.
    struct {
      TypeEnum TypeFlag;
      uint8_t Paddings[3];
      uint32_t Idx;
    } Data;
  } Inner;
};

// <<<<<<<< Type definitions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> Value definitions >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

/// FuncRef definition.
namespace Runtime::Instance {
class FunctionInstance;
}

/// NumType and RefType variant definitions.
struct RefVariant {
#if __INTPTR_WIDTH__ == 32
  uint32_t Padding = -1;
#endif
  void *Ptr = nullptr;
  RefVariant() = default;
  template <typename T>
  RefVariant(const T *P) : Ptr(reinterpret_cast<void *>(const_cast<T *>(P))) {}
  template <typename T> RefVariant(T *P) : Ptr(reinterpret_cast<void *>(P)) {}
  bool isNull() const { return Ptr == nullptr; }

  template <typename T> T *asPtr() const { return reinterpret_cast<T *>(Ptr); }
};

using ValVariant =
    Variant<uint32_t, int32_t, uint64_t, int64_t, float, double, uint128_t,
            int128_t, uint64x2_t, int64x2_t, uint32x4_t, int32x4_t, uint16x8_t,
            int16x8_t, uint8x16_t, int8x16_t, floatx4_t, doublex2_t,
            RefVariant>;

// <<<<<<<< Value definitions <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

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
struct IsWasmRef : std::is_same<RemoveCVRefT<T>, RefVariant> {};
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

template <typename T> inline ValType ValTypeFromType() noexcept;

template <> inline ValType ValTypeFromType<uint32_t>() noexcept {
  return ValType(TypeCode::I32);
}
template <> inline ValType ValTypeFromType<int32_t>() noexcept {
  return ValType(TypeCode::I32);
}
template <> inline ValType ValTypeFromType<uint64_t>() noexcept {
  return ValType(TypeCode::I64);
}
template <> inline ValType ValTypeFromType<int64_t>() noexcept {
  return ValType(TypeCode::I64);
}
template <> inline ValType ValTypeFromType<uint128_t>() noexcept {
  return ValType(TypeCode::V128);
}
template <> inline ValType ValTypeFromType<int128_t>() noexcept {
  return ValType(TypeCode::V128);
}
template <> inline ValType ValTypeFromType<float>() noexcept {
  return ValType(TypeCode::F32);
}
template <> inline ValType ValTypeFromType<double>() noexcept {
  return ValType(TypeCode::F64);
}

// <<<<<<<< Template to get value type from type <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> Const expression to generate value from value type >>>>>>>>>>>>>>>>>

inline ValVariant ValueFromType(ValType Type) noexcept {
  switch (Type.getCode()) {
  case TypeCode::I32:
    return uint32_t(0U);
  case TypeCode::I64:
    return uint64_t(0U);
  case TypeCode::F32:
    return float(0.0F);
  case TypeCode::F64:
    return double(0.0);
  case TypeCode::V128:
    return uint128_t(0U);
  case TypeCode::FuncRef:
  case TypeCode::ExternRef:
  case TypeCode::Ref:
  case TypeCode::RefNull:
    return RefVariant();
  default:
    assumingUnreachable();
  }
}

// <<<<<<<< Const expression to generate value from value type <<<<<<<<<<<<<<<<<

// >>>>>>>> Functions to retrieve reference inners >>>>>>>>>>>>>>>>>>>>>>>>>>>>>

inline const Runtime::Instance::FunctionInstance *
retrieveFuncRef(const RefVariant &Val) {
  return Val.asPtr<Runtime::Instance::FunctionInstance>();
}

template <typename T> inline T &retrieveExternRef(const RefVariant &Val) {
  return *Val.asPtr<T>();
}

// <<<<<<<< Functions to retrieve reference inners <<<<<<<<<<<<<<<<<<<<<<<<<<<<<

} // namespace WasmEdge

template <>
struct fmt::formatter<WasmEdge::ValType> : fmt::formatter<std::string_view> {
  fmt::format_context::iterator
  format(const WasmEdge::ValType &Type,
         fmt::format_context &Ctx) const noexcept {
    using namespace std::literals;
    // For the number types, print the type directly.
    if (!Type.isRefType()) {
      return formatter<std::string_view>::format(
          WasmEdge::TypeCodeStr[Type.getCode()], Ctx);
    }
    // For the reference types, print the details.
    fmt::memory_buffer Buffer;
    fmt::format_to(std::back_inserter(Buffer), "{} {}"sv,
                   WasmEdge::TypeCodeStr[Type.getCode()],
                   WasmEdge::TypeCodeStr[Type.getHeapTypeCode()]);
    if (Type.getHeapTypeCode() == WasmEdge::TypeCode::TypeIndex) {
      fmt::format_to(std::back_inserter(Buffer), "[{}]"sv, Type.getTypeIndex());
    }
    return formatter<std::string_view>::format(
        std::string_view(Buffer.data(), Buffer.size()), Ctx);
  }
};
