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

// The bit pattern of the value types:
// -----------------------------------------------------------------------
//  byte | 0-th | 1-st |     2-nd    |     3-rd     |     4-th ~ 7-th
// ------|-------------|-------------|--------------|---------------------
//       |             | ValTypeCode | HeapTypeCode |     Type index
//       |             | 0x7F, 0x7E, | 0x6F or 0x70 |     (uint32_t)
//  code |  Reserved   | 0x7D, 0x7C, |              |
//       |  (Padding)  | 0x7B, 0x6B, |        For the HeapType use
//       |             | or 0x6C     |    (Function references proposal)
// -----------------------------------------------------------------------
// Due to compress the whole value types into uint64_t length, WasmEdge
// implements the HeapType into the RefType and ValType classes.
// As the definitions in the function references proposal, the `FuncRef` and
// `ExternRef` are reinterpreted as the `ref.null` types, respectively.
// Therefore, WasmEdge hendles them into `ref null func` and `ref null extern`
// in the ValType classes.

/// HeapTypeCode enumeration class.
enum class HeapTypeCode : uint8_t {
  // This enum class is for the internal use only.
  NotHeapType = 0x00,
  Extern = 0x6F,
  Func = 0x70,
  TypeIndex = 0x40,
};

/// TypeBase definition. The basic data structure of value types.
class ValTypeBase {
public:
  // Note: The padding bytes are reserved and should not be written.
  ValTypeBase() noexcept = default;
  ValTypeBase(ValTypeCode C, HeapTypeCode HT, uint32_t I) noexcept {
    Inner.Data.Code = C;
    Inner.Data.HTCode = HT;
    Inner.Data.Idx = I;
  }
  ValTypeBase(const std::array<uint8_t, 8> R) noexcept {
    std::copy_n(R.cbegin(), 8, Inner.Raw);
  }

  friend bool operator==(const ValTypeBase &LHS,
                         const ValTypeBase &RHS) noexcept {
    return (LHS.Inner.Data.Code == RHS.Inner.Data.Code) &&
           (LHS.Inner.Data.HTCode == RHS.Inner.Data.HTCode) &&
           (LHS.Inner.Data.Idx == RHS.Inner.Data.Idx);
  }
  friend bool operator!=(const ValTypeBase &LHS,
                         const ValTypeBase &RHS) noexcept {
    return !(LHS == RHS);
  }

  ValTypeCode getCode() const noexcept { return Inner.Data.Code; }
  HeapTypeCode getHeapTypeCode() const noexcept { return Inner.Data.HTCode; }
  uint32_t getTypeIndex() const noexcept { return Inner.Data.Idx; }
  const std::array<uint8_t, 8> getRawData() const noexcept {
    std::array<uint8_t, 8> R;
    std::copy_n(Inner.Raw, 8, R.begin());
    return R;
  }

  bool isNumType() const noexcept {
    switch (Inner.Data.Code) {
    case ValTypeCode::I32:
    case ValTypeCode::I64:
    case ValTypeCode::F32:
    case ValTypeCode::F64:
    case ValTypeCode::V128:
      return true;
    default:
      return false;
    }
  }

  bool isRefType() const noexcept {
    switch (Inner.Data.Code) {
    case ValTypeCode::Ref:
    case ValTypeCode::RefNull:
      return true;
    default:
      return false;
    }
  }

  bool isFuncRefType() const noexcept {
    return (Inner.Data.HTCode == HeapTypeCode::Func);
  }

  bool isExternRefType() const noexcept {
    return (Inner.Data.HTCode == HeapTypeCode::Extern);
  }

  bool isNullableRefType() const noexcept {
    return (Inner.Data.Code == ValTypeCode::RefNull);
  }

protected:
  union {
    uint8_t Raw[8];
    struct {
      uint8_t Paddings[2];
      ValTypeCode Code;
      HeapTypeCode HTCode;
      uint32_t Idx;
    } Data;
  } Inner;
};

/// HeapType definition. The RefType is the subset of the RefType.
class HeapType : public ValTypeBase {
public:
  HeapType() noexcept = default;
  // Constructor for the heap types (func and extern).
  HeapType(HeapTypeCode HT) noexcept
      : ValTypeBase(ValTypeCode::RefNull, HT, 0) {
    assuming((Inner.Data.Code == ValTypeCode::RefNull));
    assuming((Inner.Data.HTCode != HeapTypeCode::TypeIndex) &&
             (Inner.Data.HTCode != HeapTypeCode::NotHeapType));
  }
  // Constructor for the heap types (type index).
  HeapType(uint32_t I) noexcept
      : ValTypeBase(ValTypeCode::RefNull, HeapTypeCode::TypeIndex, I) {
    assuming((Inner.Data.Code == ValTypeCode::RefNull));
  }
};

/// RefType definition. The RefType is the subset of the ValType.
class RefType : public ValTypeBase {
public:
  RefType() noexcept = default;
  // Constructor for the old type of externref and funcref.
  RefType(RefTypeCode C) noexcept
      : ValTypeBase(ValTypeCode::RefNull, static_cast<HeapTypeCode>(C), 0) {
    assuming(Inner.Data.Code == ValTypeCode::RefNull);
    assuming((Inner.Data.HTCode == HeapTypeCode::Func) ||
             (Inner.Data.HTCode == HeapTypeCode::Extern));
  }
  // Constructor for the heap types (func and extern).
  RefType(RefTypeCode C, HeapType HT) noexcept
      : ValTypeBase(static_cast<ValTypeCode>(C), HT.getHeapTypeCode(),
                    HT.getTypeIndex()) {
    assuming((Inner.Data.Code == ValTypeCode::Ref) ||
             (Inner.Data.Code == ValTypeCode::RefNull));
  }
  // Constructor for setting the raw data.
  RefType(const std::array<uint8_t, 8> R) noexcept : ValTypeBase(R) {}
};

/// ValType definition.
class ValType : public ValTypeBase {
public:
  ValType() noexcept = default;
  // Constructor for the value type codes without heap type immediates.
  ValType(ValTypeCode C) noexcept
      : ValTypeBase(C, HeapTypeCode::NotHeapType, 0) {
    switch (C) {
    case ValTypeCode::I32:
    case ValTypeCode::I64:
    case ValTypeCode::F32:
    case ValTypeCode::F64:
    case ValTypeCode::V128:
      break;
    case ValTypeCode::ExternRef:
    case ValTypeCode::FuncRef:
      Inner.Data.HTCode = static_cast<HeapTypeCode>(Inner.Data.Code);
      Inner.Data.Code = ValTypeCode::RefNull;
      break;
    case ValTypeCode::Ref:
    case ValTypeCode::RefNull:
    default:
      assumingUnreachable();
    }
  }
  // Constructor for the number type codes.
  ValType(NumTypeCode C) noexcept
      : ValTypeBase(static_cast<ValTypeCode>(C), HeapTypeCode::NotHeapType, 0) {
  }
  // Constructor for the reference type codes without heap type immediates.
  ValType(RefTypeCode C) noexcept
      : ValTypeBase(ValTypeCode::RefNull, static_cast<HeapTypeCode>(C), 0) {
    assuming(Inner.Data.Code == ValTypeCode::RefNull);
    assuming((Inner.Data.HTCode == HeapTypeCode::Func) ||
             (Inner.Data.HTCode == HeapTypeCode::Extern));
  }
  // Constructor for the heap types (func and extern).
  ValType(RefTypeCode C, HeapType HT) noexcept
      : ValTypeBase(static_cast<ValTypeCode>(C), HT.getHeapTypeCode(),
                    HT.getTypeIndex()) {
    assuming((Inner.Data.Code == ValTypeCode::Ref) ||
             (Inner.Data.Code == ValTypeCode::RefNull));
  }
  // Constructor for the RefTypes.
  ValType(const RefType &RType) noexcept
      : ValTypeBase(RType.getCode(), RType.getHeapTypeCode(),
                    RType.getTypeIndex()) {}
  // Constructor for setting the raw data.
  ValType(const std::array<uint8_t, 8> R) noexcept : ValTypeBase(R) {}

  RefType toRefType() const noexcept {
    assuming(isRefType());
    std::array<uint8_t, 8> R;
    std::copy_n(Inner.Raw, 8, R.begin());
    return RefType(R);
  }
};

/// BlockType definition.
class BlockType {
public:
  // Note: The BlockType should be compressed into 8 bytes to reduce the
  // insturction class size.
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

/// Value variant definitions.
using RefVariant = Variant<UnknownRef, FuncRef, ExternRef>;
using ValVariant =
    Variant<uint32_t, int32_t, uint64_t, int64_t, float, double, uint128_t,
            int128_t, uint64x2_t, int64x2_t, uint32x4_t, int32x4_t, uint16x8_t,
            int16x8_t, uint8x16_t, int8x16_t, floatx4_t, doublex2_t, UnknownRef,
            FuncRef, ExternRef>;

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

template <typename T> inline ValType ValTypeFromType() noexcept;

template <> inline ValType ValTypeFromType<uint32_t>() noexcept {
  return ValType(ValTypeCode::I32);
}
template <> inline ValType ValTypeFromType<int32_t>() noexcept {
  return ValType(ValTypeCode::I32);
}
template <> inline ValType ValTypeFromType<uint64_t>() noexcept {
  return ValType(ValTypeCode::I64);
}
template <> inline ValType ValTypeFromType<int64_t>() noexcept {
  return ValType(ValTypeCode::I64);
}
template <> inline ValType ValTypeFromType<uint128_t>() noexcept {
  return ValType(ValTypeCode::V128);
}
template <> inline ValType ValTypeFromType<int128_t>() noexcept {
  return ValType(ValTypeCode::V128);
}
template <> inline ValType ValTypeFromType<float>() noexcept {
  return ValType(ValTypeCode::F32);
}
template <> inline ValType ValTypeFromType<double>() noexcept {
  return ValType(ValTypeCode::F64);
}
template <> inline ValType ValTypeFromType<FuncRef>() noexcept {
  return ValType(ValTypeCode::FuncRef);
}
template <> inline ValType ValTypeFromType<ExternRef>() noexcept {
  return ValType(ValTypeCode::ExternRef);
}

// <<<<<<<< Template to get value type from type <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// >>>>>>>> Const expression to generate value from value type >>>>>>>>>>>>>>>>>

inline ValVariant ValueFromType(ValType Type) noexcept {
  switch (Type.getCode()) {
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
  case ValTypeCode::FuncRef:
  case ValTypeCode::ExternRef:
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
