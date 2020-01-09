// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/ethereum/finish.h"

namespace SSVM {
namespace Executor {

ErrCode EEIFinish::body(VM::EnvironmentManager &EnvMgr,
                        Instance::MemoryInstance &MemInst, uint32_t DataOffset,
                        uint32_t DataLength) {
  /// Add cost.
  if (!EnvMgr.addCost(Cost)) {
    return ErrCode::Revert;
  }

  Env.getReturnData().clear();
  if (DataLength > 0) {
    return MemInst.getBytes(Env.getReturnData(), DataOffset, DataLength);
  }

  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
