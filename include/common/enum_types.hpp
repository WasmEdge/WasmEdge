// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

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
#include "errcode.h"
#include "spare_enum_map.h"

#include <cstdint>
#include <string_view>

namespace WasmEdge {

/// WASM Value type C++ enumeration class.
enum class TypeCode : uint8_t {
#define UseTypeCode
#define Line(NAME, VALUE, STRING) NAME = VALUE,
#include "enum.inc"
#undef Line
#undef UseTypeCode
};

static inline constexpr const auto TypeCodeStr = []() constexpr {
  using namespace std::literals::string_view_literals;
  std::pair<TypeCode, std::string_view> Array[] = {
#define UseTypeCode
#define Line(NAME, VALUE, STRING) {TypeCode::NAME, STRING},
#include "enum.inc"
#undef Line
#undef UseTypeCode
  };
  return SpareEnumMap(Array);
}();

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
}();

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
}();

} // namespace WasmEdge

template <>
struct fmt::formatter<WasmEdge::ValMut> : fmt::formatter<std::string_view> {
  fmt::format_context::iterator
  format(const WasmEdge::ValMut &Mut, fmt::format_context &Ctx) const noexcept {
    return formatter<std::string_view>::format(WasmEdge::ValMutStr[Mut], Ctx);
  }
};

template <>
struct fmt::formatter<WasmEdge::ExternalType>
    : fmt::formatter<std::string_view> {
  fmt::format_context::iterator
  format(const WasmEdge::ExternalType &Type,
         fmt::format_context &Ctx) const noexcept {
    return formatter<std::string_view>::format(WasmEdge::ExternalTypeStr[Type],
                                               Ctx);
  }
};
