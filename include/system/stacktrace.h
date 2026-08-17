// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/system/stacktrace.h - Runtime call stack trace -----------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains helpers for collecting calling stacks for various
/// operating systems.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/span.h"
#include "runtime/stackmgr.h"

namespace WasmEdge {

namespace Runtime::Instance {
class ModuleInstance;
} // namespace Runtime::Instance

/// One frame of a recorded stack trace: the function's defining module and its
/// index within that module.
struct StackTraceEntry {
  const Runtime::Instance::ModuleInstance *Module;
  uint32_t FuncIndex;
};

Span<void *const> stackTrace(Span<void *> Buffer) noexcept;

Span<const StackTraceEntry>
interpreterStackTrace(const Runtime::StackManager &StackMgr,
                      Span<StackTraceEntry> Buffer) noexcept;

Span<const StackTraceEntry>
compiledStackTrace(Span<const Runtime::Instance::ModuleInstance *const> Modules,
                   Span<void *const> Stack,
                   Span<StackTraceEntry> Buffer) noexcept;

} // namespace WasmEdge
