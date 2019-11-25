#include "executor/instance/memory.h"
#include "ast/instruction.h"
#include "executor/common.h"
#include "executor/worker.h"
#include "executor/worker/util.h"
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
  return StackMgr.push(Value(MemoryInst->getDataPageSize()));
}

ErrCode Worker::runMemoryGrowOp() {
  /// Get Memory Instance
  Instance::MemoryInstance *MemoryInst = nullptr;
  if (ErrCode Status = getMemInstByIdx(0, MemoryInst);
      Status != ErrCode::Success) {
    return Status;
  };

  /// Pop N for growing page size.
  Value N;
  if (ErrCode Status = StackMgr.pop(N); Status != ErrCode::Success) {
    return Status;
  }

  /// Grow page and push result.
  unsigned int CurrPageSize = MemoryInst->getDataPageSize();
  if (MemoryInst->growPage(retrieveValue<uint32_t>(N)) != ErrCode::Success) {
    return StackMgr.push(static_cast<uint32_t>(-1));
  }
  return StackMgr.push(CurrPageSize);
}

} // namespace Executor
} // namespace SSVM
