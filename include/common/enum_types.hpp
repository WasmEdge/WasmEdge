// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasmedge/common/enum_types.hpp - WASM types C++ enumerations ------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definitions of WASM types related C++ enumerations.
///
//===----------------------------------------------------------------------===//

#pragma once

#include "dense_enum_map.h"
#include "enum_types.h"
#include "errcode.h"
#include "spare_enum_map.h"

#include <cstdint>
#include <string_view>

namespace WasmEdge {

/// WASM Value type C++ enumeration class.
enum class ValType : uint8_t {
#define UseValType
#define Line(NAME, VALUE, STRING) NAME = VALUE,
#include "enum.inc"
#undef Line
#undef UseValType
};

static inline constexpr const auto ValTypeStr = []() constexpr {
  using namespace std::literals::string_view_literals;
  std::pair<ValType, std::string_view> Array[] = {
#define UseValType
#define Line(NAME, VALUE, STRING) {ValType::NAME, STRING},
#include "enum.inc"
#undef Line
#undef UseValType
  };
  return SpareEnumMap(Array);
}
();

/// WASM Number type C++ enumeration class.
enum class NumType : uint8_t {
#define UseNumType
#define Line(NAME, VALUE) NAME = VALUE,
#include "enum.inc"
#undef Line
#undef UseNumType
};

/// WASM Reference type C++ enumeration class.
enum class RefType : uint8_t {
#define UseRefType
#define Line(NAME, VALUE) NAME = VALUE,
#include "enum.inc"
#undef Line
#undef UseRefType
};

enum class HeapType : uint8_t {
#define UseHeapType
#define Line(NAME, VALUE) NAME = VALUE,
#include "enum.inc"
#undef Line
#undef UseHeapType

  Defined,
};

enum class ValTypeCode : uint8_t {
#define UseNumType
#define Line(NAME, VALUE) NAME = VALUE,
#include "enum.inc"
#undef Line
#undef UseNumType

#define UseRefTypeCode
#define Line(NAME, VALUE) NAME = VALUE,
#include "enum.inc"
#undef Line
#undef UseRefTypeCode
};

static inline constexpr const auto ValTypeCodeStr = []() constexpr {
  using namespace std::literals::string_view_literals;
  std::pair<ValTypeCode, std::string_view> Array[] = {
#define UseNumType
#define Line(NAME, VALUE) {ValTypeCode::NAME, #NAME},
#include "enum.inc"
#undef Line
#undef UseNumType

#define UseRefTypeCode
#define Line(NAME, VALUE) {ValTypeCode::NAME, #NAME},
#include "enum.inc"
#undef Line
#undef UseRefTypeCode
  };
  return SpareEnumMap(Array);
}
();

enum RefTypeCode : uint8_t {
#define UseRefTypeCode
#define Line(NAME, VALUE) NAME = VALUE,
#include "enum.inc"
#undef Line
#undef UseRefTypeCode
};

class RefTypeExt {
public:
  RefTypeExt() = default;
  RefTypeExt(HeapType HType) : HType(HType), DefinedTypeIdx(0) {
    assuming(HType != HeapType::Defined);
  }
  RefTypeExt(uint32_t TypeIdx)
      : HType(HeapType::Defined), DefinedTypeIdx(TypeIdx) {}

  RefTypeExt(WasmEdge_RefTypeExt Ext)
      : HType(static_cast<HeapType>(Ext.HeapType)),
        DefinedTypeIdx(Ext.DefinedTypeIdx) {
    switch (Ext.HeapType) {
    case (uint8_t)HeapType::Func:
    case (uint8_t)HeapType::Extern:
    case (uint8_t)HeapType::Defined:
      break;
    default:
      assumingUnreachable();
    }
  }

  WasmEdge_RefTypeExt asCStruct() const {
    return WasmEdge_RefTypeExt{
        .HeapType = static_cast<WasmEdge_HeapType>(HType),
        .DefinedTypeIdx = DefinedTypeIdx,
    };
  }

  HeapType getHType() const { return HType; }

  uint32_t getDefinedTypeIdx() const { return DefinedTypeIdx; }

private:
  HeapType HType;
  uint32_t DefinedTypeIdx;
};

class FullRefType {
public:
  FullRefType() = default;
  FullRefType(const RefType RType)
      : TypeCode(RefTypeCode::RefNull), Ext(static_cast<HeapType>(RType)) {}
  FullRefType(const HeapType HType)
      : TypeCode(RefTypeCode::RefNull), Ext(HType) {
    assuming(HType != HeapType::Defined);
  }
  FullRefType(const RefTypeCode TypeCode, const HeapType HType)
      : TypeCode(TypeCode), Ext(HType) {
    assuming(HType != HeapType::Defined);
  }
  FullRefType(const RefTypeCode TypeCode, const uint32_t TypeIdx)
      : TypeCode(TypeCode), Ext(TypeIdx) {}
  FullRefType(const RefTypeCode TypeCode, const RefTypeExt Ext)
      : TypeCode(TypeCode), Ext(Ext) {}
  RefTypeCode getTypeCode() const { return TypeCode; }
  RefTypeExt getExt() const { return Ext; }

  friend bool operator==(const FullRefType &LHS,
                         const FullRefType &RHS) noexcept {
    if (LHS.TypeCode != RHS.TypeCode) {
      return false;
    }
    if (LHS.Ext.getHType() != RHS.Ext.getHType()) {
      return false;
    }
    if (LHS.Ext.getHType() == HeapType::Defined) {
      return LHS.Ext.getDefinedTypeIdx() == RHS.Ext.getDefinedTypeIdx();
    } else {
      return true;
    }
  }
  friend bool operator!=(const FullRefType &LHS,
                         const FullRefType &RHS) noexcept {
    return !(LHS.TypeCode == RHS.TypeCode);
  }

private:
  RefTypeCode TypeCode;
  RefTypeExt Ext;
};

class FullValType {
public:
  FullValType() = default;
  FullValType(const ValType VType) {
    switch (VType) {
    case ValType::I32:
    case ValType::I64:
    case ValType::F32:
    case ValType::F64:
    case ValType::V128: {
      *this = FullValType(static_cast<NumType>(VType));
      break;
    }
    case ValType::ExternRef:
    case ValType::FuncRef: {
      *this = FullValType(static_cast<RefType>(VType));
      break;
    }
    }
  }
  FullValType(const NumType NType)
      : TypeCode(static_cast<ValTypeCode>(NType)), Ext({}) {}
  FullValType(const WasmEdge_FullValType VType)
      : TypeCode(static_cast<ValTypeCode>(VType.TypeCode)), Ext({}) {
    if (isRefType()) {
      Ext.RTypeExt = VType.Ext.RefTypeExt;
    }
  }
  FullValType(const FullRefType RType)
      : TypeCode(static_cast<ValTypeCode>(RType.getTypeCode())),
        Ext({.RTypeExt = RType.getExt()}) {}
  ValTypeCode getTypeCode() const { return TypeCode; }
  bool isNumType() const {
    switch (TypeCode) {
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
  bool isRefType() const {
    switch (TypeCode) {
    case ValTypeCode::Ref:
    case ValTypeCode::RefNull:
      return true;
    default:
      return false;
    }
  }
  WasmEdge_FullValType asCStruct() const {
    if (isNumType()) {
      return WasmEdge_FullValType{
          .TypeCode = static_cast<WasmEdge_ValTypeCode>(TypeCode),
          .Ext = {},
      };
    } else {
      return WasmEdge_FullValType{
          .TypeCode = static_cast<WasmEdge_ValTypeCode>(TypeCode),
          .Ext = {.RefTypeExt = Ext.RTypeExt.asCStruct()}};
    }
  }

  FullRefType asRefType() const {
    assuming(isRefType());
    return FullRefType(static_cast<RefTypeCode>(TypeCode), Ext.RTypeExt);
  }

  friend bool operator==(const FullValType &LHS,
                         const FullValType &RHS) noexcept {
    if (LHS.TypeCode != RHS.TypeCode) {
      return false;
    }

    if (LHS.isNumType()) {
      return true;
    }

    return LHS.asRefType() == RHS.asRefType();
  }
  friend bool operator!=(const FullValType &LHS,
                         const FullValType &RHS) noexcept {
    return !(LHS.TypeCode == RHS.TypeCode);
  }

private:
  ValTypeCode TypeCode;
  union ValTypeExt {
    RefTypeExt RTypeExt;
  } Ext;
};

/// WASM Mutability C++ enumeration class.
enum class ValMut : uint8_t {
#define UseValMut
#define Line(NAME, VALUE, STRING) NAME = VALUE,
#include "enum.inc"
#undef Line
#undef UseValMut
};

static inline constexpr auto ValMutStr = []() constexpr {
  using namespace std::literals::string_view_literals;
  std::pair<ValMut, std::string_view> Array[] = {
#define UseValMut
#define Line(NAME, VALUE, STRING) {ValMut::NAME, STRING},
#include "enum.inc"
#undef Line
#undef UseValMut
  };
  return DenseEnumMap(Array);
}
();

/// WASM External type C++ enumeration class.
enum class ExternalType : uint8_t {
#define UseExternalType
#define Line(NAME, VALUE, STRING) NAME = VALUE,
#include "enum.inc"
#undef Line
#undef UseExternalType
};

static inline constexpr auto ExternalTypeStr = []() constexpr {
  using namespace std::literals::string_view_literals;
  std::pair<ExternalType, std::string_view> Array[] = {
#define UseExternalType
#define Line(NAME, VALUE, STRING) {ExternalType::NAME, STRING},
#include "enum.inc"
#undef Line
#undef UseExternalType
  };
  return DenseEnumMap(Array);
}
();

} // namespace WasmEdge
