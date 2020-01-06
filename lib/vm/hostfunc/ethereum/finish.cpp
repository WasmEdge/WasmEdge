// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/ethereum/finish.h"

namespace SSVM {
namespace Executor {

EEIFinish::EEIFinish(VM::EVMEnvironment &Env, uint64_t Cost) : EEI(Env, Cost) {
  initializeFuncType<EEIFinish>();
}

ErrCode EEIFinish::run(VM::EnvironmentManager &EnvMgr, StackManager &StackMgr,
                       Instance::MemoryInstance &MemInst) {
  return invoke<EEIFinish>(EnvMgr, StackMgr, MemInst);
}

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
