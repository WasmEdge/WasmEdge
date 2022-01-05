// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "executor/executor.h"

#include <memory>
#include <vector>

namespace WasmEdge {
namespace Executor {

Expect<void> Executor::runLocalGetOp(const uint32_t Idx) {
  const uint32_t Offset = StackMgr.getOffset(Idx);
  StackMgr.push(StackMgr.getBottomN(Offset));
  return {};
}

Expect<void> Executor::runLocalSetOp(const uint32_t Idx) {
  const uint32_t Offset = StackMgr.getOffset(Idx);
  StackMgr.getBottomN(Offset) = StackMgr.pop();
  return {};
}

Expect<void> Executor::runLocalTeeOp(const uint32_t Idx) {
  const ValVariant &Val = StackMgr.getTop();
  const uint32_t Offset = StackMgr.getOffset(Idx);
  StackMgr.getBottomN(Offset) = Val;
  return {};
}

Expect<void> Executor::runGlobalGetOp(Runtime::StoreManager &StoreMgr,
                                      const uint32_t Idx) {
  auto *GlobInst = getGlobInstByIdx(StoreMgr, Idx);
  StackMgr.push(GlobInst->getValue());
  return {};
}

Expect<void> Executor::runGlobalSetOp(Runtime::StoreManager &StoreMgr,
                                      const uint32_t Idx) {
  auto *GlobInst = getGlobInstByIdx(StoreMgr, Idx);
  if (GlobInst) {
    GlobInst->getValue() = StackMgr.pop();
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
