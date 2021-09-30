// SPDX-License-Identifier: Apache-2.0

#include "interpreter/interpreter.h"

#include <memory>
#include <vector>

namespace WasmEdge {
namespace Interpreter {

Expect<void> Interpreter::runLocalGetOp(const uint32_t Idx) {
  const uint32_t Offset = StackMgr.getOffset(Idx);
  StackMgr.push(StackMgr.getBottomN(Offset));
  return {};
}

Expect<void> Interpreter::runLocalSetOp(const uint32_t Idx) {
  const uint32_t Offset = StackMgr.getOffset(Idx);
  StackMgr.getBottomN(Offset) = StackMgr.pop();
  return {};
}

Expect<void> Interpreter::runLocalTeeOp(const uint32_t Idx) {
  const ValVariant &Val = StackMgr.getTop();
  const uint32_t Offset = StackMgr.getOffset(Idx);
  StackMgr.getBottomN(Offset) = Val;
  return {};
}

Expect<void> Interpreter::runGlobalGetOp(Runtime::StoreManager &StoreMgr,
                                         const uint32_t Idx) {
  auto *GlobInst = getGlobInstByIdx(StoreMgr, Idx);
  StackMgr.push(GlobInst->getValue());
  return {};
}

Expect<void> Interpreter::runGlobalSetOp(Runtime::StoreManager &StoreMgr,
                                         const uint32_t Idx) {
  auto *GlobInst = getGlobInstByIdx(StoreMgr, Idx);
  GlobInst->getValue() = StackMgr.pop();
  return {};
}

} // namespace Interpreter
} // namespace WasmEdge
