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
#include "common/dense_enum_map.h"
#include "common/spare_enum_map.h"
#include <cstdint>
#include <string_view>
#endif

#if (defined(__cplusplus) && __cplusplus > 201402L) ||                         \
    (defined(_MSVC_LANG) && _MSVC_LANG > 201402L)
namespace WasmEdge {

/// WASM Value type C++ enumeration class.
enum class ValType : uint8_t {
  None = 0x40,
  I32 = 0x7F,
  I64 = 0x7E,
  F32 = 0x7D,
  F64 = 0x7C,
  V128 = 0x7B,
  FuncRef = 0x70,
  ExternRef = 0x6F
};

static inline constexpr const auto ValTypeStr = []() constexpr {
  using namespace std::literals::string_view_literals;
  std::pair<ValType, std::string_view> Array[] = {
      {ValType::None, "none"sv},       {ValType::I32, "i32"sv},
      {ValType::I64, "i64"sv},         {ValType::F32, "f32"sv},
      {ValType::F64, "f64"sv},         {ValType::V128, "v128"sv},
      {ValType::FuncRef, "funcref"sv}, {ValType::ExternRef, "externref"sv},
  };
  return SpareEnumMap(Array);
}
();

} // namespace WasmEdge

#endif

/// WASM Value type C enumeration.
enum WasmEdge_ValType {
  WasmEdge_ValType_I32 = 0x7FU,
  WasmEdge_ValType_I64 = 0x7EU,
  WasmEdge_ValType_F32 = 0x7DU,
  WasmEdge_ValType_F64 = 0x7CU,
  WasmEdge_ValType_V128 = 0x7BU,
  WasmEdge_ValType_FuncRef = 0x70U,
  WasmEdge_ValType_ExternRef = 0x6FU
};

#if (defined(__cplusplus) && __cplusplus > 201402L) ||                         \
    (defined(_MSVC_LANG) && _MSVC_LANG > 201402L)
namespace WasmEdge {

/// WASM Number type C++ enumeration class.
enum class NumType : uint8_t {
  I32 = 0x7F,
  I64 = 0x7E,
  F32 = 0x7D,
  F64 = 0x7C,
  V128 = 0x7B
};

} // namespace WasmEdge
#endif

/// WASM Number type C enumeration.
enum WasmEdge_NumType {
  WasmEdge_NumType_I32 = 0x7FU,
  WasmEdge_NumType_I64 = 0x7EU,
  WasmEdge_NumType_F32 = 0x7DU,
  WasmEdge_NumType_F64 = 0x7CU,
  WasmEdge_NumType_V128 = 0x7BU
};

#if (defined(__cplusplus) && __cplusplus > 201402L) ||                         \
    (defined(_MSVC_LANG) && _MSVC_LANG > 201402L)
namespace WasmEdge {

/// WASM Reference type C++ enumeration class.
enum class RefType : uint8_t { ExternRef = 0x6F, FuncRef = 0x70 };

} // namespace WasmEdge
#endif

/// WASM Reference type C enumeration.
enum WasmEdge_RefType {
  WasmEdge_RefType_FuncRef = 0x70U,
  WasmEdge_RefType_ExternRef = 0x6FU
};

#if (defined(__cplusplus) && __cplusplus > 201402L) ||                         \
    (defined(_MSVC_LANG) && _MSVC_LANG > 201402L)
namespace WasmEdge {

/// WASM Mutability C++ enumeration class.
enum class ValMut : uint8_t { Const = 0x00, Var = 0x01 };

static inline constexpr auto ValMutStr = []() constexpr {
  using namespace std::literals::string_view_literals;
  std::pair<ValMut, std::string_view> Array[] = {
      {ValMut::Const, "const"sv},
      {ValMut::Var, "var"sv},
  };
  return DenseEnumMap(Array);
} // namespace WasmEdge
();

} // namespace WasmEdge
#endif

/// WASM Mutability C enumeration.
enum WasmEdge_Mutability {
  WasmEdge_Mutability_Const = 0x00U,
  WasmEdge_Mutability_Var = 0x01U
};

#if (defined(__cplusplus) && __cplusplus > 201402L) ||                         \
    (defined(_MSVC_LANG) && _MSVC_LANG > 201402L)
namespace WasmEdge {

/// WASM External type C++ enumeration class.
enum class ExternalType : uint8_t {
  Function = 0x00U,
  Table = 0x01U,
  Memory = 0x02U,
  Global = 0x03U
};

static inline constexpr auto ExternalTypeStr = []() constexpr {
  using namespace std::literals::string_view_literals;
  std::pair<ExternalType, std::string_view> Array[] = {
      {ExternalType::Function, "function"sv},
      {ExternalType::Table, "table"sv},
      {ExternalType::Memory, "memory"sv},
      {ExternalType::Global, "global"sv},
  };
  return DenseEnumMap(Array);
}
();

} // namespace WasmEdge
#endif

/// WASM External type C enumeration.
enum WasmEdge_ExternalType {
  WasmEdge_ExternalType_Function = 0x00U,
  WasmEdge_ExternalType_Table = 0x01U,
  WasmEdge_ExternalType_Memory = 0x02U,
  WasmEdge_ExternalType_Global = 0x03U
};

#endif // WASMEDGE_C_API_ENUM_TYPES_H
