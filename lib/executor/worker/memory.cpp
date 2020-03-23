// SPDX-License-Identifier: Apache-2.0
#include "executor/instance/memory.h"
#include "common/ast/instruction.h"
#include "executor/common.h"
#include "executor/worker.h"
#include "common/value.h"
#include "support/casting.h"

#include <cstdint>
#include <type_traits>

namespace SSVM {
namespace Executor {

ErrCode Worker::runMemorySizeOp() {
  /// Get Memory Instance
  Instance::MemoryInstance *MemoryInst = nullptr;
  if (ErrCode Status = getMemInstByIdx(0, MemoryInst);
      Status != ErrCode::Success) {
    return Status;
  };

  /// Push SZ = page size to stack.
  return StackMgr.push(uint32_t(MemoryInst->getDataPageSize()));
}

ErrCode Worker::runMemoryGrowOp() {
  /// Get Memory Instance
  Instance::MemoryInstance *MemoryInst = nullptr;
  if (ErrCode Status = getMemInstByIdx(0, MemoryInst);
      Status != ErrCode::Success) {
    return Status;
  };

  /// Pop N for growing page size.
  uint32_t &N = retrieveValue<uint32_t>(StackMgr.getTop());

  /// Grow page and push result.
  unsigned int CurrPageSize = MemoryInst->getDataPageSize();
  if (MemoryInst->growPage(N) != ErrCode::Success) {
    N = -1;
  } else {
    N = CurrPageSize;
  }
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
