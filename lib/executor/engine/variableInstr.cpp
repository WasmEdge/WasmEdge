// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

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
  // Copy into the local slot while the operand is still on the GC-rooted value
  // stack, then pop: popping into a native-stack local first would leave a ref
  // unrooted (the collector scans the value stack, not the C++ stack) and
  // reclaimable.
  const ValVariant Val = StackMgr.peekTop<ValVariant>();
  // Shade the stored reference (a downward move): another thread's root scan
  // may have already passed this lower local slot when the store lands, while
  // the source is popped from the not-yet-scanned top -- leaving the only copy
  // in already-scanned territory. No-op outside a collection.
  Allocator.writeBarrier(Val);
  StackMgr.emplaceTopN(StackOffset, Val);
  StackMgr.pop<ValVariant>();
  return {};
}

Expect<void> Executor::runLocalTeeOp(Runtime::StackManager &StackMgr,
                                     uint32_t StackOffset) const noexcept {
  const ValVariant Val = StackMgr.peekTop<ValVariant>();
  // Shade the stored reference: same downward-store hazard as local.set (the
  // tee copy is consumed immediately, leaving the live copy in the
  // possibly-already-scanned local slot).
  Allocator.writeBarrier(Val);
  StackMgr.emplaceTopN(StackOffset, Val);
  return {};
}

Expect<void> Executor::runGlobalGetOp(Runtime::StackManager &StackMgr,
                                      uint32_t Idx) const noexcept {
  auto *GlobInst = getGlobInstByIdx(StackMgr, Idx);
  assuming(GlobInst);
  // The global is itself a registered GC root, so its value stays rooted across
  // the push (no barrier, unlike global.set).
  StackMgr.push(GlobInst->getValue());
  return {};
}

Expect<void> Executor::runGlobalSetOp(Runtime::StackManager &StackMgr,
                                      uint32_t Idx) const noexcept {
  auto *GlobInst = getGlobInstByIdx(StackMgr, Idx);
  assuming(GlobInst);
  // Keep the operand on the GC-rooted value stack across setValue() rather than
  // popping into a native-stack local: until setValue installs it, a ref would
  // be unrooted (the collector does not scan the C++ stack) and reclaimable.
  GlobInst->setValue(StackMgr.peekTop<ValVariant>());
  StackMgr.pop<ValVariant>();
  return {};
}

} // namespace Executor
} // namespace WasmEdge
