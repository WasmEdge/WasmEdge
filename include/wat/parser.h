// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2026 Second State INC

//===-- wasmedge/wat/parser.h - WAT text format parser --------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the WAT parser API, which provides
/// functions to parse WebAssembly Text Format (.wat) files and convert them
/// to AST::Module.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/module.h"
#include "common/configure.h"
#include "common/errcode.h"
#include "common/span.h"

#include <cstdint>
#include <filesystem>
#include <string_view>
#include <vector>

namespace WasmEdge {
namespace WAT {

/// Parse WAT source text and convert to an AST::Module.
///
/// Parses the WAT text, builds a CST, and converts it into the WasmEdge
/// internal AST representation. This is the primary entry point for loading
/// WAT from memory.
///
/// \param Source The WAT source text to parse and convert.
/// \param Conf The configuration controlling enabled proposals.
/// \returns The resulting AST::Module on success, or an error code on failure.
Expect<AST::Module> parseWat(std::string_view Source, const Configure &Conf);

/// Parse a WAT file from disk and convert to an AST::Module.
///
/// Reads the file at \p Path (in binary mode, preserving the exact bytes so
/// tree-sitter byte offsets stay aligned) and forwards its contents to
/// parseWat. A convenience over parseWat for callers that have a file path.
///
/// \param Path The filesystem path of the .wat file to read.
/// \param Conf The configuration controlling enabled proposals.
/// \returns The resulting AST::Module on success, or an error code on failure
///          (IllegalPath if the file cannot be opened).
Expect<AST::Module> parseWatFile(const std::filesystem::path &Path,
                                 const Configure &Conf);

/// Heuristic check to decide whether a byte buffer should be routed to the
/// WAT text parser rather than the WASM binary parser.
///
/// Skips an optional UTF-8 BOM, leading whitespace and line comments
/// (';; ...'), then returns true if the next byte is '('. WAT text starts with
/// '(' -- an opening '(module ...)' or a block comment '(; ... ;)' -- whereas
/// WASM binary starts with the \0asm magic. Block comments are covered by the
/// '(' check.
///
/// This is a routing heuristic, not full validation; the real parser handles
/// UTF-8 and illegal-character checking.
///
/// \param Input The byte sequence to check.
/// \returns true if the input looks like WAT text rather than binary.
bool maybeWAT(Span<const uint8_t> Input) noexcept;

} // namespace WAT
} // namespace WasmEdge
