// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/common/enum_configure.hpp - Configure C++ enumerations ---===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definitions of configure related C++ enumerations.
///
//===----------------------------------------------------------------------===//

#pragma once

#include "dense_enum_map.h"

#include <cstdint>
#include <string_view>

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
}();

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
