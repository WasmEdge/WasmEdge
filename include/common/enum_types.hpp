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
#include "errcode.h"
#include "spare_enum_map.h"

#include <cstdint>
#include <string_view>

namespace WasmEdge {

/// WASM Value type C++ enumeration class.
enum class ValTypeCode : uint8_t {
#define UseValTypeCode
#define Line(NAME, VALUE, STRING) NAME = VALUE,
#include "enum.inc"
#undef Line
#undef UseValTypeCode
};

static inline constexpr const auto ValTypeStr = []() constexpr {
  using namespace std::literals::string_view_literals;
  std::pair<ValTypeCode, std::string_view> Array[] = {
#define UseValTypeCode
#define Line(NAME, VALUE, STRING) {ValTypeCode::NAME, STRING},
#include "enum.inc"
#undef Line
#undef UseValTypeCode
  };
  return SpareEnumMap(Array);
}();

/// WASM Number type C++ enumeration class.
enum class NumTypeCode : uint8_t {
#define UseNumTypeCode
#define Line(NAME, VALUE) NAME = VALUE,
#include "enum.inc"
#undef Line
#undef UseNumTypeCode
};

/// WASM Reference type C++ enumeration class.
enum class RefTypeCode : uint8_t {
#define UseRefTypeCode
#define Line(NAME, VALUE) NAME = VALUE,
#include "enum.inc"
#undef Line
#undef UseRefTypeCode
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
