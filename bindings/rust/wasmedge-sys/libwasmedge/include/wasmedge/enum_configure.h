// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasmedge/common/enum_configure.h - Configure related enumerations -===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definitions of configure related enumerations.
///
//===----------------------------------------------------------------------===//

#ifndef WASMEDGE_C_API_ENUM_CONFIGURE_H
#define WASMEDGE_C_API_ENUM_CONFIGURE_H

#if (defined(__cplusplus) && __cplusplus > 201402L) ||                         \
    (defined(_MSVC_LANG) && _MSVC_LANG > 201402L)
#include "dense_enum_map.h"
#include <cstdint>
#include <string_view>
#endif

#if (defined(__cplusplus) && __cplusplus > 201402L) ||                         \
    (defined(_MSVC_LANG) && _MSVC_LANG > 201402L)
namespace WasmEdge {

/// WASM Proposal C++ enumeration class.
enum class Proposal : uint8_t {
#define UseProposal
#define Line(NAME, STRING) NAME,
#include "enum.inc"
#undef Line
#undef UseProposal
  Max
};

static inline constexpr auto ProposalStr = []() constexpr {
  using namespace std::literals::string_view_literals;
  std::pair<Proposal, std::string_view> Array[] = {
#define UseProposal
#define Line(NAME, STRING) {Proposal::NAME, STRING##sv},
#include "enum.inc"
#undef Line
#undef UseProposal
  };
  return DenseEnumMap(Array);
}
();

} // namespace WasmEdge
#endif

/// WASM Proposal C enumeration.
enum WasmEdge_Proposal {
#define UseProposal
#define Line(NAME, STRING) WasmEdge_Proposal_##NAME,
#include "enum.inc"
#undef Line
#undef UseProposal
};

#if (defined(__cplusplus) && __cplusplus > 201402L) ||                         \
    (defined(_MSVC_LANG) && _MSVC_LANG > 201402L)
namespace WasmEdge {

/// Host Module Registration C++ enumeration class.
enum class HostRegistration : uint8_t {
#define UseHostRegistration
#define Line(NAME) NAME,
#include "enum.inc"
#undef Line
#undef UseHostRegistration
  Max
};

} // namespace WasmEdge
#endif

/// Host Module Registration C enumeration.
enum WasmEdge_HostRegistration {
#define UseHostRegistration
#define Line(NAME) WasmEdge_HostRegistration_##NAME,
#include "enum.inc"
#undef Line
#undef UseHostRegistration
};

/// AOT compiler optimization level C enumeration.
enum WasmEdge_CompilerOptimizationLevel {
  // Disable as many optimizations as possible.
  WasmEdge_CompilerOptimizationLevel_O0 = 0,
  // Optimize quickly without destroying debuggability.
  WasmEdge_CompilerOptimizationLevel_O1,
  // Optimize for fast execution as much as possible without triggering
  // significant incremental compile time or code size growth.
  WasmEdge_CompilerOptimizationLevel_O2,
  // Optimize for fast execution as much as possible.
  WasmEdge_CompilerOptimizationLevel_O3,
  // Optimize for small code size as much as possible without triggering
  // significant incremental compile time or execution time slowdowns.
  WasmEdge_CompilerOptimizationLevel_Os,
  // Optimize for small code size as much as possible.
  WasmEdge_CompilerOptimizationLevel_Oz
};

/// AOT compiler output binary format C enumeration.
enum WasmEdge_CompilerOutputFormat {
  // Native dynamic library format.
  WasmEdge_CompilerOutputFormat_Native = 0,
  // WebAssembly with AOT compiled codes in custom sections.
  WasmEdge_CompilerOutputFormat_Wasm
};

#endif // WASMEDGE_C_API_ENUM_CONFIGURE_H
