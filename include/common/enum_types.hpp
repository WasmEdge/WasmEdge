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

class FullRefType {
public:
  FullRefType() = default;
  FullRefType(const RefType TypeCode) : TypeCode(TypeCode) {}
  RefType getTypeCode() const { return TypeCode; }
  WasmEdge_ValTypeExt getExt() const { return Ext; }

  friend bool operator==(const FullRefType &LHS,
                         const FullRefType &RHS) noexcept {
    return LHS.TypeCode == RHS.TypeCode;
  }
  friend bool operator!=(const FullRefType &LHS,
                         const FullRefType &RHS) noexcept {
    return !(LHS.TypeCode == RHS.TypeCode);
  }

private:
  RefType TypeCode;
  WasmEdge_ValTypeExt Ext;
};

class FullValType {
public:
  FullValType() = default;
  FullValType(const ValType TypeCode) : TypeCode(TypeCode) {}
  FullValType(const RefType RType) {
    switch (RType) {
    case RefType::ExternRef: {
      TypeCode = ValType::ExternRef;
      break;
    }
    case RefType::FuncRef: {
      TypeCode = ValType::FuncRef;
      break;
    }
    }
  }
  FullValType(const WasmEdge_FullValType VType)
      : TypeCode(static_cast<ValType>(VType.TypeCode)), Ext(VType.Ext) {}
  FullValType(const FullRefType VType) : Ext(VType.getExt()) {
    switch (VType.getTypeCode()) {
    case RefType::ExternRef: {
      TypeCode = ValType::ExternRef;
      break;
    }
    case RefType::FuncRef: {
      TypeCode = ValType::FuncRef;
      break;
    }
    }
  }
  ValType getTypeCode() const { return TypeCode; }
  WasmEdge_FullValType asCStruct() const {
    return WasmEdge_FullValType{
        .TypeCode = static_cast<WasmEdge_ValType>(TypeCode),
        .Ext = Ext,
    };
  }

  friend bool operator==(const FullValType &LHS,
                         const FullValType &RHS) noexcept {
    return LHS.TypeCode == RHS.TypeCode;
  }
  friend bool operator!=(const FullValType &LHS,
                         const FullValType &RHS) noexcept {
    return !(LHS.TypeCode == RHS.TypeCode);
  }

private:
  ValType TypeCode;
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
