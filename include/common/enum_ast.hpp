// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasmedge/common/enum_ast.hpp - AST C++ enumerations ---------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definitions of Wasm VM used AST and instruction nodes
/// enumerations.
///
//===----------------------------------------------------------------------===//

// This header is not exported to the C API.

#pragma once

#include "dense_enum_map.h"
#include "spare_enum_map.h"

#include <cstdint>
#include <string>

namespace WasmEdge {

/// AST node attributes enumeration class.
enum class ASTNodeAttr : uint8_t {
#define UseASTNodeAttr
#define Line(NAME, STRING) NAME,
#include "enum.inc"
#undef Line
#undef UseASTNodeAttr
};

/// AST node attributes enumeration string mapping.
static inline constexpr auto ASTNodeAttrStr = []() constexpr {
  using namespace std::literals::string_view_literals;
  std::pair<ASTNodeAttr, std::string_view> Array[] = {
#define UseASTNodeAttr
#define Line(NAME, STRING) {ASTNodeAttr::NAME, STRING},
#include "enum.inc"
#undef Line
#undef UseASTNodeAttr
  };
  return DenseEnumMap(Array);
}
();

/// Instruction opcode enumeration class.
enum class OpCode : uint16_t {
#define UseOpCode
#define Line(NAME, VALUE, STRING) NAME = VALUE,
#include "enum.inc"
#undef Line
#undef UseOpCode
};

/// Instruction opcode enumeration string mapping.
static inline constexpr const auto OpCodeStr = []() constexpr {
  using namespace std::literals::string_view_literals;
  std::pair<OpCode, std::string_view> Array[] = {
#define UseOpCode
#define Line(NAME, VALUE, STRING) {OpCode::NAME, STRING},
#include "enum.inc"
#undef Line
#undef UseOpCode
  };
  return SpareEnumMap(Array);
}
();

} // namespace WasmEdge
