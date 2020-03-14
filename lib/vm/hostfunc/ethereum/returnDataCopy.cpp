// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/ethereum/returnDataCopy.h"

namespace SSVM {
namespace Executor {

ErrCode EEIReturnDataCopy::body(VM::EnvironmentManager &EnvMgr,
                                Instance::MemoryInstance &MemInst,
                                uint32_t ResultOffset, uint32_t DataOffset,
                                uint32_t Length) {
  /// Take additional gas of copy.
  if (addCopyCost(EnvMgr, Length) != ErrCode::Success) {
    return ErrCode::CostLimitExceeded;
  }
  return MemInst.setBytes(Env.getReturnData(), ResultOffset, DataOffset,
                          Length);
}

} // namespace Executor
} // namespace SSVM
