// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/executor.h"

#include <cstdint>

namespace WasmEdge {
namespace Executor {

Expect<void> Executor::runLocalGetOp(Runtime::StackManager &StackMgr,
                                     uint32_t StackOffset) const noexcept {
  const ValVariant Local = StackMgr.peekTopN<ValVariant>(StackOffset);
  StackMgr.push(Local);
  return {};
}

Expect<void> Executor::runLocalSetOp(Runtime::StackManager &StackMgr,
                                     uint32_t StackOffset) const noexcept {
  const ValVariant Val = StackMgr.pop<ValVariant>();
  StackMgr.emplaceTopN(StackOffset - 1, Val);
  return {};
}

Expect<void> Executor::runLocalTeeOp(Runtime::StackManager &StackMgr,
                                     uint32_t StackOffset) const noexcept {
  const ValVariant Val = StackMgr.peekTop<ValVariant>();
  StackMgr.emplaceTopN(StackOffset, Val);
  return {};
}

Expect<void> Executor::runGlobalGetOp(Runtime::StackManager &StackMgr,
                                      uint32_t Idx) const noexcept {
  auto *GlobInst = getGlobInstByIdx(StackMgr, Idx);
  assuming(GlobInst);
  const ValVariant Global = GlobInst->getValue();
  StackMgr.push(Global);
  return {};
}

Expect<void> Executor::runGlobalSetOp(Runtime::StackManager &StackMgr,
                                      uint32_t Idx) const noexcept {
  auto *GlobInst = getGlobInstByIdx(StackMgr, Idx);
  assuming(GlobInst);
  const ValVariant Val = StackMgr.pop<ValVariant>();
  GlobInst->setValue(Val);
  return {};
}

} // namespace Executor
} // namespace WasmEdge
