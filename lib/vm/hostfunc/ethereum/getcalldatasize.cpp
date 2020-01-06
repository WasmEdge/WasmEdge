// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/ethereum/getcalldatasize.h"

namespace SSVM {
namespace Executor {

EEIGetCallDataSize::EEIGetCallDataSize(VM::EVMEnvironment &Env, uint64_t Cost)
    : EEI(Env, Cost) {
  initializeFuncType<EEIGetCallDataSize>();
}

ErrCode EEIGetCallDataSize::run(VM::EnvironmentManager &EnvMgr,
                                StackManager &StackMgr,
                                Instance::MemoryInstance &MemInst) {
  return invoke<EEIGetCallDataSize>(EnvMgr, StackMgr, MemInst);
}

ErrCode EEIGetCallDataSize::body(VM::EnvironmentManager &EnvMgr,
                                 Instance::MemoryInstance &MemInst,
                                 uint32_t &Ret) {
  /// Add cost.
  if (!EnvMgr.addCost(Cost)) {
    return ErrCode::Revert;
  }
  /// Return: Length(u32)
  Ret = Env.getCallData().size();
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
