// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/system/fault.h - Memory and arithmetic exception ---------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the software exception handler for various operating
/// systems.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/errcode.h"
#include <array>
#include <atomic>
#include <csetjmp>

namespace WasmEdge {

class Fault {
public:
  Fault();

  ~Fault() noexcept;

  [[noreturn]] static void emitFault(ErrCode Error);

  std::jmp_buf &buffer() noexcept { return Buffer; }

  Span<void *const> stacktrace() const noexcept {
    return Span<void *const>{StackTraceBuffer}.first(StackTraceSize);
  }

  // Arm signal-safe shadow-root head truncation for the enclosing compiled call
  // (GC proposal, spec R7-M2). On an abnormal fault, emitFault's longjmp skips
  // the per-frame shadow-frame pops that generated code emits, which would leave
  // the thread's shadow-root head pointing at a ShadowFrame in the compiled
  // stack the jump is about to unwind. Because the shadow chain is a strict
  // stack parallel to the compiled call stack, resetting the head to the value
  // it held at this boundary collapses the whole abandoned suffix in one store.
  // Call once, right after building the Fault, with the thread's stable head
  // cell and its value at the boundary. The head cell is type-erased to a
  // lock-free atomic<pointer> to keep the system/ layer free of a gc/
  // dependency. Passing a null HeadCell (the default) disables truncation (e.g.
  // a thread with no registered shadow stack).
  void armShadowRestore(std::atomic<void *> *HeadCell,
                        void *BoundaryHead) noexcept {
    ShadowHeadCell = HeadCell;
    ShadowHeadBoundary = BoundaryHead;
  }

private:
  Fault *Prev = nullptr;
  // Signal-safe shadow-root truncation target; null when unarmed. Written before
  // the guarded call, read (and stored through) in emitFault before the longjmp.
  std::atomic<void *> *ShadowHeadCell = nullptr;
  void *ShadowHeadBoundary = nullptr;
  std::jmp_buf Buffer;
  std::array<void *, 256> StackTraceBuffer;
  size_t StackTraceSize;
};

} // namespace WasmEdge

#define PREPARE_FAULT(f) (static_cast<uint32_t>(setjmp((f).buffer())))
