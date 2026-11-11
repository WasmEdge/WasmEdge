// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors
#pragma once

#include "ast/instruction.h"
#include "common/errcode.h"
#include "common/span.h"
#include "runtime/stackmgr.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace WasmEdge {
namespace Coredump {

/// WasmEdge extension sections, omitted in wasmgdb mode. Integers use the
/// LEB128 encoding of the Wasm binary format.
///
///   wasmedge.tables    ::= version:u32 vec(table)
///   table              ::= type:tabletype size:u64 elems:vec(ref)
///   wasmedge.instances ::= version:u32 vec(instance)
///   instance           ::= tables:vec(u32) datadropped:vec(u8)
///                          elemdropped:vec(u8)
///   wasmedge.globals   ::= version:u32 vec(global)
///   global             ::= globalidx:u32 type:globaltype ref
///   ref                ::= 0x00                              null
///                        | 0x01 instanceidx:u32 funcidx:u32   function
///                        | 0x02                              missing
///
/// Instance indices follow the coreinstances order and function indices
/// include imports. Missing references are non-null externrefs, GC references,
/// and functions absent from all dumped modules. Dropped flags are 0 (live) or
/// 1 (dropped). Each version covers the layout of its own section only.
///
/// Extension types keep the original limits, mutability, and reference types;
/// concrete type indices keep their module-local meaning. The standard
/// sections use nullable abstract tops instead, so readers need GC enabled for
/// anyref placeholders (including unresolved concrete types) and
/// ExceptionHandling for exnref placeholders.
static inline constexpr const uint32_t kTablesVersion = 1;
static inline constexpr const uint32_t kInstancesVersion = 1;
static inline constexpr const uint32_t kGlobalsVersion = 1;

/// What to dump and where to write it. The caller must keep the referenced
/// modules, stack, instructions, and path alive until generate returns.
struct Request {
  /// Modules to dump, in order. The first non-null module names the executable.
  /// Null pointers are skipped and duplicate instances are included only once.
  Span<const Runtime::Instance::ModuleInstance *const> Modules;
  /// An in-flight interpreter stack, or nullptr between invocations.
  const Runtime::StackManager *Stack = nullptr;
  /// Innermost frame's instruction. Ignored when Stack is nullptr.
  AST::InstrView::iterator PC = nullptr;
  /// Emit the layout readable by wasmgdb.
  bool ForWasmgdb = false;
  /// Output path. Empty selects coredump.<unix time> in the working directory.
  std::string_view Path;
};

/// Dump the requested state, retaining none of the request's references.
/// Modules must not be empty. Stack modules are appended when missing.
/// A null Stack emits a main thread with zero frames.
/// \returns the path of the written file, or an error code.
Expect<std::string> generate(const Request &Req) noexcept;

/// Collect the trapping execution's modules and forward to generate.
///
/// \param StackMgr the stack manager of the trapping execution.
/// \param PC the instruction which caused the trap. May be an empty iterator
/// when the trapping instruction is unknown.
/// \param ForWasmgdb generate a coredump consumable by the `wasmgdb` debugger.
/// Its parser reads the size of the operand stack vector but not its content,
/// therefore the operand stack is omitted in this mode.
///
/// \returns the path of the generated coredump file, or an error code.
Expect<std::string> generateCoredump(const Runtime::StackManager &StackMgr,
                                     AST::InstrView::iterator PC,
                                     bool ForWasmgdb) noexcept;

} // namespace Coredump
} // namespace WasmEdge
