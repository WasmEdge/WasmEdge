// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/common/enum_ast.hpp - AST C++ enumerations ---------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definitions of AST and instruction node enumerations
/// used by the Wasm VM.
///
//===----------------------------------------------------------------------===//

// This header is not exported to the C API.

#pragma once

#include "common/dense_enum_map.h"
#include "common/spare_enum_map.h"
#include "common/spdlog.h"

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
}();

/// Instruction opcode enumeration class.
enum class OpCode : uint32_t {
#define UseOpCode
#define Line(NAME, STRING, PREFIX) NAME,
#define Line_FB(NAME, STRING, PREFIX, EXTEND) NAME,
#define Line_FC(NAME, STRING, PREFIX, EXTEND) NAME,
#define Line_FD(NAME, STRING, PREFIX, EXTEND) NAME,
#define Line_FE(NAME, STRING, PREFIX, EXTEND) NAME,
#include "enum.inc"
#undef Line
#undef Line_FB
#undef Line_FC
#undef Line_FD
#undef Line_FE
#undef UseOpCode
};

/// Instruction opcode enumeration string mapping.
static inline constexpr const auto OpCodeStr = []() constexpr {
  using namespace std::literals::string_view_literals;
  std::pair<OpCode, std::string_view> Array[] = {
#define UseOpCode
#define Line(NAME, STRING, PREFIX) {OpCode::NAME, STRING},
#define Line_FB(NAME, STRING, PREFIX, EXTEND) {OpCode::NAME, STRING},
#define Line_FC(NAME, STRING, PREFIX, EXTEND) {OpCode::NAME, STRING},
#define Line_FD(NAME, STRING, PREFIX, EXTEND) {OpCode::NAME, STRING},
#define Line_FE(NAME, STRING, PREFIX, EXTEND) {OpCode::NAME, STRING},
#include "enum.inc"
#undef Line
#undef Line_FB
#undef Line_FC
#undef Line_FD
#undef Line_FE
#undef UseOpCode
  };
  return SpareEnumMap(Array);
}();

/// Component Model Value opcode C++ enumeration class.
enum class ComponentCanonOpCode : uint8_t {
#define UseComponentCanonOpCode
#define Line(NAME, VALUE, STRING) NAME = VALUE,
#include "enum.inc"
#undef Line
#undef UseComponentCanonOpCode
};

static inline constexpr const auto ComponentCanonOpCodeStr = []() constexpr {
  using namespace std::literals::string_view_literals;
  std::pair<ComponentCanonOpCode, std::string_view> Array[] = {
#define UseComponentCanonOpCode
#define Line(NAME, VALUE, STRING) {ComponentCanonOpCode::NAME, STRING},
#include "enum.inc"
#undef Line
#undef UseComponentCanonOpCode
  };
  return SpareEnumMap(Array);
}();

/// Component Model Value Opt code C++ enumeration class.
enum class ComponentCanonOptCode : uint8_t {
#define UseComponentCanonOptCode
#define Line(NAME, VALUE, STRING) NAME = VALUE,
#include "enum.inc"
#undef Line
#undef UseComponentCanonOptCode
};

static inline constexpr const auto ComponentCanonOptCodeStr = []() constexpr {
  using namespace std::literals::string_view_literals;
  std::pair<ComponentCanonOptCode, std::string_view> Array[] = {
#define UseComponentCanonOptCode
#define Line(NAME, VALUE, STRING) {ComponentCanonOptCode::NAME, STRING},
#include "enum.inc"
#undef Line
#undef UseComponentCanonOptCode
  };
  return DenseEnumMap(Array);
}();

} // namespace WasmEdge

template <>
struct fmt::formatter<WasmEdge::ASTNodeAttr>
    : fmt::formatter<std::string_view> {
  template <typename FmtCtx>
  auto format(const WasmEdge::ASTNodeAttr &Attr,
         FmtCtx &Ctx) WASMEDGE_FMT_CONST noexcept -> decltype(Ctx.out()) {
    return formatter<std::string_view>::format(WasmEdge::ASTNodeAttrStr[Attr],
                                               Ctx);
  }
};

template <>
struct fmt::formatter<WasmEdge::OpCode> : fmt::formatter<std::string_view> {
  template <typename FmtCtx>
  auto format(const WasmEdge::OpCode &Code,
         FmtCtx &Ctx) WASMEDGE_FMT_CONST noexcept -> decltype(Ctx.out()) {
    return formatter<std::string_view>::format(WasmEdge::OpCodeStr[Code], Ctx);
  }
};
