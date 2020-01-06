// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/ethereum/revert.h"

namespace SSVM {
namespace Executor {

EEIRevert::EEIRevert(VM::EVMEnvironment &Env, uint64_t Cost) : EEI(Env, Cost) {
  initializeFuncType<EEIRevert>();
}

ErrCode EEIRevert::run(VM::EnvironmentManager &EnvMgr, StackManager &StackMgr,
                       Instance::MemoryInstance &MemInst) {
  return invoke<EEIRevert>(EnvMgr, StackMgr, MemInst);
}

ErrCode EEIRevert::body(VM::EnvironmentManager &EnvMgr,
                        Instance::MemoryInstance &MemInst, uint32_t DataOffset,
                        uint32_t DataLength) {
  /// Add cost.
  if (!EnvMgr.addCost(Cost)) {
    return ErrCode::Revert;
  }

  Env.getReturnData().clear();
  if (DataLength > 0) {
    if (ErrCode Status =
            MemInst.getBytes(Env.getReturnData(), DataOffset, DataLength);
        Status != ErrCode::Success) {
      return Status;
    }
  }

  /// Return: void
  return ErrCode::Revert;
}

} // namespace Executor
} // namespace SSVM
