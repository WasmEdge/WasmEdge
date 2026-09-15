// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors
#pragma once

#include "ast/instruction.h"
#include "common/errcode.h"
#include "runtime/stackmgr.h"

#include <string>

namespace WasmEdge {
namespace Coredump {

/// Dump the current execution state into a coredump file.
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
