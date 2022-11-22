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

static inline constexpr const auto RefTypeCodeStr = []() constexpr {
  using namespace std::literals::string_view_literals;
  std::pair<RefTypeCode, std::string_view> Array[] = {
#define UseRefTypeCode
#define Line(NAME, VALUE) {RefTypeCode::NAME, #NAME},
#include "enum.inc"
#undef Line
#undef UseRefTypeCode
  };
  return SpareEnumMap(Array);
}
();

class FullRefType {
public:
  FullRefType() = default;
  FullRefType(const RefType RType) {
    switch (RType) {
    case RefType::ExternRef: {
      *this = FullRefType(RefTypeCode::RefNull, HeapType::Extern);
      break;
    }
    case RefType::FuncRef: {
      *this = FullRefType(RefTypeCode::RefNull, HeapType::Func);
      break;
    }
    }
  }
  FullRefType(const HeapType HType) : TypeCode(RefTypeCode::RefNull) {
    assuming(HType != HeapType::Defined);
    Ext.HeapType = static_cast<WasmEdge_HeapType>(HType);
  }
  FullRefType(const RefTypeCode TypeCode, const HeapType HType)
      : TypeCode(TypeCode) {
    assuming(HType != HeapType::Defined);
    Ext.HeapType = static_cast<WasmEdge_HeapType>(HType);
  }
  FullRefType(const RefTypeCode TypeCode, const uint32_t TypeIdx)
      : TypeCode(TypeCode) {
    Ext.HeapType = WasmEdge_HeapType_Defined;
    Ext.DefinedTypeIdx = TypeIdx;
  }
  RefTypeCode getTypeCode() const { return TypeCode; }
  WasmEdge_RefTypeExt getExt() const { return Ext; }

  friend bool operator==(const FullRefType &LHS,
                         const FullRefType &RHS) noexcept {
    if (LHS.TypeCode != RHS.TypeCode) {
      return false;
    }
    if (LHS.Ext.HeapType != RHS.Ext.HeapType) {
      return false;
    }
    if (LHS.Ext.HeapType == WasmEdge_HeapType_Defined) {
      return LHS.Ext.DefinedTypeIdx == RHS.Ext.DefinedTypeIdx;
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
  WasmEdge_RefTypeExt Ext;
};

class FullValType {
public:
  FullValType() = default;
  FullValType(const ValType VType) {
    switch (VType) {
    case ValType::I32: {
      *this = FullValType(NumType::I32);
      break;
    }
    case ValType::I64: {
      *this = FullValType(NumType::I64);
      break;
    }
    case ValType::F32: {
      *this = FullValType(NumType::F32);
      break;
    }
    case ValType::F64: {
      *this = FullValType(NumType::F64);
      break;
    }
    case ValType::V128: {
      *this = FullValType(NumType::V128);
      break;
    }
    case ValType::ExternRef: {
      *this = FullValType(RefType::ExternRef);
      break;
    }
    case ValType::FuncRef: {
      *this = FullValType(RefType::FuncRef);
      break;
    }
    }
  }
  FullValType(const NumType NType) {
    switch (NType) {
    case NumType::F32: {
      TypeCode = ValTypeCode::F32;
      break;
    }
    case NumType::F64: {
      TypeCode = ValTypeCode::F64;
      break;
    }
    case NumType::I32: {
      TypeCode = ValTypeCode::I32;
      break;
    }
    case NumType::I64: {
      TypeCode = ValTypeCode::I64;
      break;
    }
    case NumType::V128: {
      TypeCode = ValTypeCode::V128;
      break;
    }
    }
    Ext = WasmEdge_ValTypeExt();
  }
  FullValType(const WasmEdge_FullValType VType)
      : TypeCode(static_cast<ValTypeCode>(VType.TypeCode)), Ext(VType.Ext) {}
  FullValType(const FullRefType VType) {
    switch (VType.getTypeCode()) {
    case RefTypeCode::Ref: {
      TypeCode = ValTypeCode::Ref;
      break;
    }
    case RefTypeCode::RefNull: {
      TypeCode = ValTypeCode::RefNull;
      break;
    }
    }
    Ext.RefTypeExt = VType.getExt();
  }
  ValTypeCode getTypeCode() const { return TypeCode; }
  WasmEdge_ValTypeExt getExt() const { return Ext; }
  WasmEdge_FullValType asCStruct() const {
    return WasmEdge_FullValType{
        .TypeCode = static_cast<WasmEdge_ValTypeCode>(TypeCode),
        .Ext = Ext,
    };
  }

  friend bool operator==(const FullValType &LHS,
                         const FullValType &RHS) noexcept {
    if (LHS.TypeCode != RHS.TypeCode) {
      return false;
    }

    if (LHS.TypeCode != ValTypeCode::Ref &&
        LHS.TypeCode != ValTypeCode::RefNull) {
      return true;
    }

    if (LHS.Ext.RefTypeExt.HeapType != RHS.Ext.RefTypeExt.HeapType) {
      return false;
    }
    if (LHS.Ext.RefTypeExt.HeapType != WasmEdge_HeapType_Defined) {
      return true;
    }

    return LHS.Ext.RefTypeExt.DefinedTypeIdx ==
           RHS.Ext.RefTypeExt.DefinedTypeIdx;
  }
  friend bool operator!=(const FullValType &LHS,
                         const FullValType &RHS) noexcept {
    return !(LHS.TypeCode == RHS.TypeCode);
  }

private:
  ValTypeCode TypeCode;
  WasmEdge_ValTypeExt Ext;
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
