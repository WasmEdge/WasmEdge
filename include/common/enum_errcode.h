// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasmedge/common/enum_errcode.h - Error code enumerations ----------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the enumerations of WasmEdge error code.
///
//===----------------------------------------------------------------------===//

#ifndef WASMEDGE_C_API_ENUM_ERRCODE_H
#define WASMEDGE_C_API_ENUM_ERRCODE_H

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

/// WasmEdge runtime phasing C++ enumeration class.
/// This enumeration is not exported to the C API.
enum class WasmPhase : uint8_t {
#define UseWasmPhase
#define Line(NAME, VALUE, STRING) NAME = VALUE,
#include "enum.inc"
#undef Line
#undef UseWasmPhase
};

static inline constexpr auto WasmPhaseStr = []() constexpr {
  using namespace std::literals::string_view_literals;
  std::pair<WasmPhase, std::string_view> Array[] = {
#define UseWasmPhase
#define Line(NAME, VALUE, STRING) {WasmPhase::NAME, STRING},
#include "enum.inc"
#undef Line
#undef UseWasmPhase
  };
  return DenseEnumMap(Array);
}
();

} // namespace WasmEdge
#endif

#if (defined(__cplusplus) && __cplusplus > 201402L) ||                         \
    (defined(_MSVC_LANG) && _MSVC_LANG > 201402L)
namespace WasmEdge {

/// Error code C++ enumeration class.
enum class ErrCode : uint8_t {
#define UseErrCode
#define Line(NAME, VALUE, STRING) NAME = VALUE,
#include "enum.inc"
#undef Line
#undef UseErrCode
};

static inline constexpr const auto ErrCodeStr = []() constexpr {
  using namespace std::literals::string_view_literals;
  std::pair<ErrCode, std::string_view> Array[] = {
#define UseErrCode
#define Line(NAME, VALUE, STRING) {ErrCode::NAME, STRING},
#include "enum.inc"
#undef Line
#undef UseErrCode
  };
  return SpareEnumMap(Array);
}
();

} // namespace WasmEdge
#endif

/// Error code C enumeration.
enum WasmEdge_ErrCode {
#define UseErrCode
#define Line(NAME, VALUE, STRING) WasmEdge_ErrCode_##NAME = VALUE,
#include "enum.inc"
#undef Line
#undef UseErrCode
};

#endif // WASMEDGE_C_API_ENUM_ERRCODE_H
