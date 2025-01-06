// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/system/stacktrace.h - Runtime call stack trace -----------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains helper to collect calling stacks for various operating
/// system.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/span.h"
#include "runtime/stackmgr.h"

namespace WasmEdge {

Span<void *const> stackTrace(Span<void *> Buffer) noexcept;

Span<const uint32_t>
interpreterStackTrace(const Runtime::StackManager &StackMgr,
                      Span<uint32_t> Buffer) noexcept;

Span<const uint32_t> compiledStackTrace(const Runtime::StackManager &StackMgr,
                                        Span<uint32_t> Buffer) noexcept;

Span<const uint32_t> compiledStackTrace(const Runtime::StackManager &StackMgr,
                                        Span<void *const> Stack,
                                        Span<uint32_t> Buffer) noexcept;

void dumpStackTrace(Span<const uint32_t>) noexcept;

} // namespace WasmEdge
