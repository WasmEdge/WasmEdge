// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "executor/executor.h"

#include <cstdint>

namespace WasmEdge {
namespace Executor {

namespace {
uint32_t calculateValueSize(ValType Type) noexcept {
  switch (Type) {
  case ValType::I32:
  case ValType::F32:
    return 1;
  case ValType::I64:
  case ValType::F64:
  case ValType::FuncRef:
  case ValType::ExternRef:
    return 2;
  case ValType::V128:
    return 4;
  default:
    assumingUnreachable();
  }
}
} // namespace

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
  StackMgr.setTopN(StackOffset - calculateValueSize(Type), Type, Val);
  return {};
}

Expect<void> Executor::runLocalTeeOp(Runtime::StackManager &StackMgr,
                                     uint32_t StackOffset,
                                     ValType Type) const noexcept {
  StackMgr.setTopN(StackOffset, Type,
                   StackMgr.getTopN(calculateValueSize(Type), Type));
  return {};
}

Expect<void> Executor::runGlobalGetOp(Runtime::StoreManager &StoreMgr,
                                      Runtime::StackManager &StackMgr,
                                      uint32_t Idx, ValType Type) noexcept {
  auto *GlobInst = getGlobInstByIdx(StoreMgr, StackMgr, Idx);
  assuming(GlobInst);
  assuming(GlobInst->getGlobalType().getValType() == Type);
  StackMgr.push(Type, GlobInst->getValue());
  return {};
}

Expect<void> Executor::runGlobalSetOp(Runtime::StoreManager &StoreMgr,
                                      Runtime::StackManager &StackMgr,
                                      const uint32_t Idx,
                                      ValType Type) noexcept {
  auto *GlobInst = getGlobInstByIdx(StoreMgr, StackMgr, Idx);
  assuming(GlobInst);
  assuming(GlobInst->getGlobalType().getValType() == Type);
  GlobInst->getValue() = StackMgr.pop(Type);
  return {};
}

} // namespace Executor
} // namespace WasmEdge
