// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "executor/executor.h"

#include <cstdint>

namespace WasmEdge {
namespace Executor {

Expect<void> Executor::runLocalGetOp(Runtime::StackManager &StackMgr,
                                     const uint32_t Idx) {
  const uint32_t Offset = StackMgr.getOffset(Idx);
  StackMgr.push(StackMgr.getBottomN(Offset));
  return {};
}

Expect<void> Executor::runLocalSetOp(Runtime::StackManager &StackMgr,
                                     const uint32_t Idx) {
  const uint32_t Offset = StackMgr.getOffset(Idx);
  StackMgr.getBottomN(Offset) = StackMgr.pop();
  return {};
}

Expect<void> Executor::runLocalTeeOp(Runtime::StackManager &StackMgr,
                                     const uint32_t Idx) {
  const ValVariant &Val = StackMgr.getTop();
  const uint32_t Offset = StackMgr.getOffset(Idx);
  StackMgr.getBottomN(Offset) = Val;
  return {};
}

Expect<void> Executor::runGlobalGetOp(Runtime::StoreManager &StoreMgr,
                                      Runtime::StackManager &StackMgr,
                                      const uint32_t Idx) {
  auto *GlobInst = getGlobInstByIdx(StoreMgr, StackMgr, Idx);
  assuming(GlobInst);
  StackMgr.push(GlobInst->getValue());
  return {};
}

Expect<void> Executor::runGlobalSetOp(Runtime::StoreManager &StoreMgr,
                                      Runtime::StackManager &StackMgr,
                                      const uint32_t Idx) {
  auto *GlobInst = getGlobInstByIdx(StoreMgr, StackMgr, Idx);
  assuming(GlobInst);
  GlobInst->getValue() = StackMgr.pop();
  return {};
}

} // namespace Executor
} // namespace WasmEdge
