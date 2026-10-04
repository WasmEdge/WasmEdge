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

  // Make emitFault restore the shadow-root head to \p BoundaryHead before its
  // longjmp. The longjmp skips the shadow pops of the compiled frames, and the
  // shadow chain is a stack parallel to the call stack, so one store removes
  // the abandoned frames. Call once, right after building the Fault. A null
  // HeadCell (the default) disables this. The cell is type-erased to keep
  // system/ free of a gc/ dependency. \p Walkers (optional) counts scanners
  // that walk the chain; emitFault waits until it is zero after the store, so
  // no walk reads the frames that the longjmp abandons.
  void armShadowRestore(std::atomic<void *> *HeadCell, void *BoundaryHead,
                        std::atomic<uint32_t> *Walkers = nullptr) noexcept {
    ShadowHeadCell = HeadCell;
    ShadowHeadBoundary = BoundaryHead;
    ShadowWalkers = Walkers;
  }

private:
  Fault *Prev = nullptr;
  // Shadow-root restore target (see armShadowRestore); null when unarmed.
  std::atomic<void *> *ShadowHeadCell = nullptr;
  void *ShadowHeadBoundary = nullptr;
  std::atomic<uint32_t> *ShadowWalkers = nullptr;
  std::jmp_buf Buffer;
  std::array<void *, 256> StackTraceBuffer;
  size_t StackTraceSize;
};

} // namespace WasmEdge

#define PREPARE_FAULT(f) (static_cast<uint32_t>(setjmp((f).buffer())))
