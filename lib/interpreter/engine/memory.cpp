// SPDX-License-Identifier: Apache-2.0
#include "common/value.h"
#include "interpreter/interpreter.h"

namespace SSVM {
namespace Interpreter {

Expect<void>
Interpreter::runMemorySizeOp(Runtime::Instance::MemoryInstance &MemInst) {
  /// Push SZ = page size to stack.
  StackMgr.push(MemInst.getDataPageSize());
  return {};
}

Expect<void>
Interpreter::runMemoryGrowOp(Runtime::Instance::MemoryInstance &MemInst) {
  /// Pop N for growing page size.
  uint32_t &N = retrieveValue<uint32_t>(StackMgr.getTop());

  /// Grow page and push result.
  const uint32_t CurrPageSize = MemInst.getDataPageSize();
  if (auto Res = MemInst.growPage(N)) {
    N = CurrPageSize;
  } else {
    N = -1;
  }
  return {};
}

} // namespace Interpreter
} // namespace SSVM
