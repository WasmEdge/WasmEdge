// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/executor.h"

#include <cstdint>

namespace WasmEdge {
namespace Executor {

Expect<void> Executor::runLocalGetOp(Runtime::StackManager &StackMgr,
                                     uint32_t StackOffset) const noexcept {
  StackMgr.push(StackMgr.peekTopN<ValVariant>(StackOffset));
  return {};
}

Expect<void> Executor::runLocalSetOp(Runtime::StackManager &StackMgr,
                                     uint32_t StackOffset) const noexcept {
  // Copy the operand into the local slot while it is still on the (GC-rooted)
  // value stack, then pop it. Popping into a native-stack local first would
  // leave a GC-managed reference unrooted (the collector scans the value stack,
  // not this thread's C++ stack), so a concurrent collection could reclaim it
  // before it is stored back.
  const ValVariant Val = StackMgr.peekTop<ValVariant>();
  // Shade the stored reference. The local slot is itself a scanned root, but
  // this is a downward move within the stack: in a multi-mutator collection the
  // root scan on another thread may have already passed this (lower) local slot
  // when the store lands, after which the source is popped from the
  // not-yet-scanned top -- leaving the only copy in already-scanned territory.
  // The barrier keeps the referenced object out of the white set (no-op outside
  // a collection).
  Allocator.writeBarrier(Val);
  StackMgr.emplaceTopN(StackOffset, Val);
  StackMgr.pop<ValVariant>();
  return {};
}

Expect<void> Executor::runLocalTeeOp(Runtime::StackManager &StackMgr,
                                     uint32_t StackOffset) const noexcept {
  const ValVariant Val = StackMgr.peekTop<ValVariant>();
  // Shade the stored reference: same multi-mutator downward-store hazard as
  // local.set (the tee copy is consumed immediately, leaving the live copy in
  // the possibly-already-scanned local slot).
  Allocator.writeBarrier(Val);
  StackMgr.emplaceTopN(StackOffset, Val);
  return {};
}

Expect<void> Executor::runGlobalGetOp(Runtime::StackManager &StackMgr,
                                      uint32_t Idx) const noexcept {
  auto *GlobInst = getGlobInstByIdx(StackMgr, Idx);
  assuming(GlobInst);
  // Push the global's value. The global is itself a registered GC root, so the
  // referenced object stays rooted across the push (no barrier needed here,
  // unlike global.set).
  StackMgr.push(GlobInst->getValue());
  return {};
}

Expect<void> Executor::runGlobalSetOp(Runtime::StackManager &StackMgr,
                                      uint32_t Idx) const noexcept {
  auto *GlobInst = getGlobInstByIdx(StackMgr, Idx);
  assuming(GlobInst);
  // Keep the operand on the (GC-rooted) value stack across setValue() instead
  // of popping it into a native-stack local: a GC-managed reference would
  // otherwise be unrooted (the collector does not scan this thread's C++ stack)
  // until setValue installs it, so a concurrent collection could reclaim it.
  GlobInst->setValue(StackMgr.peekTop<ValVariant>());
  StackMgr.pop<ValVariant>();
  return {};
}

} // namespace Executor
} // namespace WasmEdge
