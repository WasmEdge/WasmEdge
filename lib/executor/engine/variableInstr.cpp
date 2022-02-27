// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "executor/executor.h"

#include <cstdint>

namespace WasmEdge {
namespace Executor {

Expect<void> Executor::runLocalGetOp(Runtime::StackManager &StackMgr,
                                     uint32_t StackOffset,
                                     ValType Type) const noexcept {
  const ValVariant Val = StackMgr.getTopN(StackOffset, Type);
  StackMgr.push(Type, Val);
  return {};
}

Expect<void> Executor::runLocalSetOp(Runtime::StackManager &StackMgr,
                                     uint32_t StackOffset,
                                     ValType Type) const noexcept {
  const ValVariant Val = StackMgr.pop(Type);
  StackMgr.setTopN(StackOffset - 1, Type, Val);
  return {};
}

Expect<void> Executor::runLocalTeeOp(Runtime::StackManager &StackMgr,
                                     uint32_t StackOffset,
                                     ValType Type) const noexcept {
  StackMgr.setTopN(StackOffset, Type, StackMgr.getTopN(1, Type));
  return {};
}

Expect<void> Executor::runGlobalGetOp(Runtime::StoreManager &StoreMgr,
                                      Runtime::StackManager &StackMgr,
                                      uint32_t Idx,
                                      ValType Type) noexcept {
  auto *GlobInst = getGlobInstByIdx(StoreMgr, StackMgr, Idx);
  assuming(GlobInst);
  assuming(GlobInst->getGlobalType().getValType() == Type);
  StackMgr.push(GlobInst->getGlobalType().getValType(), GlobInst->getValue());
  return {};
}

Expect<void> Executor::runGlobalSetOp(Runtime::StoreManager &StoreMgr,
                                      Runtime::StackManager &StackMgr,
                                      const uint32_t Idx,
                                      ValType Type) noexcept {
  auto *GlobInst = getGlobInstByIdx(StoreMgr, StackMgr, Idx);
  assuming(GlobInst);
  assuming(GlobInst->getGlobalType().getValType() == Type);
  GlobInst->getValue() = StackMgr.popUnknown();
  return {};
}

} // namespace Executor
} // namespace WasmEdge
