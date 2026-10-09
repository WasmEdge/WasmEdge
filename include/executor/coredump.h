// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors
#pragma once

#include "ast/instruction.h"
#include "common/errcode.h"
#include "common/span.h"
#include "runtime/stackmgr.h"

#include <string>
#include <string_view>

namespace WasmEdge {
namespace Coredump {

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
