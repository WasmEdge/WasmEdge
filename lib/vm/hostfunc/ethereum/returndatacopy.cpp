// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/ethereum/returndatacopy.h"

namespace SSVM {
namespace Executor {

ErrCode EEIReturnDataCopy::body(VM::EnvironmentManager &EnvMgr,
                                Instance::MemoryInstance &MemInst,
                                uint32_t ResultOffset, uint32_t DataOffset,
                                uint32_t Length) {
  /// Add cost.
  if (!EnvMgr.addCost(Cost)) {
    return ErrCode::Revert;
  }

  if (Length > 0) {
    return MemInst.setBytes(Env.getReturnData(), ResultOffset, DataOffset,
                            Length);
  }
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
