// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "executor/executor.h"

#include <cstdint>

namespace WasmEdge {
namespace Executor {

Expect<void> Executor::runLocalGetOp(Runtime::StackManager &StackMgr,
                                     uint32_t StackOffset) const noexcept {
  StackMgr.push(StackMgr.getTopN(StackOffset));
  return {};
}

Expect<void> Executor::runLocalSetOp(Runtime::StackManager &StackMgr,
                                     uint32_t StackOffset) const noexcept {
  StackMgr.getTopN(StackOffset - 1) = StackMgr.pop();
  return {};
}

Expect<void> Executor::runLocalTeeOp(Runtime::StackManager &StackMgr,
                                     uint32_t StackOffset) const noexcept {
  const ValVariant &Val = StackMgr.getTop();
  StackMgr.getTopN(StackOffset) = Val;
  return {};
}

Expect<void> Executor::runGlobalGetOp(Runtime::StoreManager &StoreMgr,
                                      Runtime::StackManager &StackMgr,
                                      const uint32_t Idx) noexcept {
  auto *GlobInst = getGlobInstByIdx(StoreMgr, StackMgr, Idx);
  assuming(GlobInst);
  StackMgr.push(GlobInst->getValue());
  return {};
}

Expect<void> Executor::runGlobalSetOp(Runtime::StoreManager &StoreMgr,
                                      Runtime::StackManager &StackMgr,
                                      const uint32_t Idx) noexcept {
  auto *GlobInst = getGlobInstByIdx(StoreMgr, StackMgr, Idx);
  assuming(GlobInst);
  GlobInst->getValue() = StackMgr.pop();
  return {};
}

} // namespace Executor
} // namespace WasmEdge
