// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasmedge/common/enum_types.h - WASM types related enumerations ----===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definitions of WASM types related enumerations.
///
//===----------------------------------------------------------------------===//

#ifndef WASMEDGE_C_API_ENUM_TYPES_H
#define WASMEDGE_C_API_ENUM_TYPES_H

#if (defined(__cplusplus) && __cplusplus > 201402L) ||                         \
    (defined(_MSVC_LANG) && _MSVC_LANG > 201402L)
#include "dense_enum_map.h"
#include "spare_enum_map.h"
#include <cstdint>
#include <string_view>
#endif

#if (defined(__cplusplus) && __cplusplus > 201402L) ||                         \
    (defined(_MSVC_LANG) && _MSVC_LANG > 201402L)
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

} // namespace WasmEdge

#endif

/// WASM Value type C enumeration.
enum WasmEdge_ValType {
#define UseValType
#define Line(NAME, VALUE, STRING) WasmEdge_ValType_##NAME = VALUE,
#include "enum.inc"
#undef Line
#undef UseValType
};

#if (defined(__cplusplus) && __cplusplus > 201402L) ||                         \
    (defined(_MSVC_LANG) && _MSVC_LANG > 201402L)
namespace WasmEdge { 
  /// WASM Interface type C++ enumeration class.
enum class InterfaceType : uint8_t {
#define UseInterfaceType
#define Line(NAME, VALUE, STRING) NAME = VALUE,
#include "enum.inc"
#undef Line
#undef UseInterfaceType
};

static inline constexpr const auto InterTypeStr = []() constexpr {
  using namespace std::literals::string_view_literals;
  std::pair<InterfaceType, std::string_view> Array[] = {
#define UseInterfaceType
#define Line(NAME, VALUE, STRING) {InterfaceType::NAME, STRING},
#include "enum.inc"
#undef Line
#undef UseInterfaceType
  };
  return SpareEnumMap(Array);
}
();

} // namespace WasmEdge
#endif 

/// WASM Interface type C enumeration.
enum WasmEdge_InterfaceType {
#define UseInterfaceType
#define Line(NAME, VALUE, STRING) WasmEdge_InterfaceType_##NAME = VALUE,
#include "enum.inc"
#undef Line
#undef UseInterfaceType
};

#if (defined(__cplusplus) && __cplusplus > 201402L) ||                         \
    (defined(_MSVC_LANG) && _MSVC_LANG > 201402L)
namespace WasmEdge {

/// WASM Number type C++ enumeration class.
enum class NumType : uint8_t {
#define UseNumType
#define Line(NAME, VALUE) NAME = VALUE,
#include "enum.inc"
#undef Line
#undef UseNumType
};

} // namespace WasmEdge
#endif

/// WASM Number type C enumeration.
enum WasmEdge_NumType {
#define UseNumType
#define Line(NAME, VALUE) WasmEdge_NumType_##NAME = VALUE,
#include "enum.inc"
#undef Line
#undef UseNumType
};

#if (defined(__cplusplus) && __cplusplus > 201402L) ||                         \
    (defined(_MSVC_LANG) && _MSVC_LANG > 201402L)
namespace WasmEdge {

/// WASM Reference type C++ enumeration class.
enum class RefType : uint8_t {
#define UseRefType
#define Line(NAME, VALUE) NAME = VALUE,
#include "enum.inc"
#undef Line
#undef UseRefType
};

} // namespace WasmEdge
#endif

/// WASM Reference type C enumeration.
enum WasmEdge_RefType {
#define UseRefType
#define Line(NAME, VALUE) WasmEdge_RefType_##NAME = VALUE,
#include "enum.inc"
#undef Line
#undef UseRefType
};

#if (defined(__cplusplus) && __cplusplus > 201402L) ||                         \
    (defined(_MSVC_LANG) && _MSVC_LANG > 201402L)
namespace WasmEdge {

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
} // namespace WasmEdge
();

} // namespace WasmEdge
#endif

/// WASM Mutability C enumeration.
enum WasmEdge_Mutability {
#define UseValMut
#define Line(NAME, VALUE, STRING) WasmEdge_Mutability_##NAME = VALUE,
#include "enum.inc"
#undef Line
#undef UseValMut
};

#if (defined(__cplusplus) && __cplusplus > 201402L) ||                         \
    (defined(_MSVC_LANG) && _MSVC_LANG > 201402L)
namespace WasmEdge {

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
#endif

/// WASM External type C enumeration.
enum WasmEdge_ExternalType {
#define UseExternalType
#define Line(NAME, VALUE, STRING) WasmEdge_ExternalType_##NAME = VALUE,
#include "enum.inc"
#undef Line
#undef UseExternalType
};

#endif // WASMEDGE_C_API_ENUM_TYPES_H
