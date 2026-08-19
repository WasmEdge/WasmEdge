// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/wat/parser.h - WAT text format parser --------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// WAT parser API: parse WebAssembly Text Format (.wat) into AST::Module.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/module.h"
#include "common/configure.h"
#include "common/errcode.h"
#include "common/span.h"

#include <cstdint>
#include <string_view>

namespace WasmEdge {
namespace WAT {

/// Parse WAT source text into an AST::Module (primary entry point).
///
/// Builds a CST, then converts it to the WasmEdge internal AST.
///
/// \param Source The WAT source text to parse and convert.
/// \returns The resulting AST::Module on success, or an error code on failure.
Expect<AST::Module> parseWat(std::string_view Source, const Configure &Conf);

/// Heuristic to route a buffer to the WAT text parser vs. the WASM binary
/// parser.
///
/// After a leading UTF-8 BOM, whitespace, and ';;' line comments, returns true
/// if the next byte is '(' — enough to distinguish WAT from a binary (which
/// starts with '\0asm'). No content validation; the parser handles that.
///
/// \param Input The byte sequence to check.
/// \returns true if the input looks like WAT text rather than binary.
bool maybeWAT(Span<const uint8_t> Input) noexcept;

} // namespace WAT
} // namespace WasmEdge
