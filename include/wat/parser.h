// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/wat/parser.h - WAT text format parser --------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// The WAT parser API. This API parses the WebAssembly Text Format (.wat)
/// into an AST::Module.
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

/// Parse WAT source text into an AST::Module. This function is the primary
/// entry point.
///
/// The function first builds a CST. Then it converts the CST into the
/// WasmEdge internal AST.
///
/// \param Source The WAT source text to parse and convert.
/// \returns The AST::Module on success, or an error code on failure.
Expect<AST::Module> parseWat(std::string_view Source, const Configure &Conf);

/// Examine a buffer and find if its content is WAT text or a WASM binary.
///
/// The function skips a UTF-8 BOM, whitespace, and ';;' line comments at the
/// start of the buffer. The function then returns true if the next byte is
/// '('. This rule is sufficient, because a WASM binary starts with '\0asm'.
/// The function does not validate the content. The parser does that.
///
/// \param Input The byte sequence to examine.
/// \returns true if the input is WAT text, or false if it is a binary.
bool maybeWAT(Span<const uint8_t> Input) noexcept;

} // namespace WAT
} // namespace WasmEdge
