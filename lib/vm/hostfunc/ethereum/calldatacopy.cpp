// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/ethereum/calldatacopy.h"

namespace SSVM {
namespace Executor {

EEICallDataCopy::EEICallDataCopy(VM::EVMEnvironment &Env, uint64_t Cost)
    : EEI(Env, Cost) {
  initializeFuncType<EEICallDataCopy>();
}

ErrCode EEICallDataCopy::run(VM::EnvironmentManager &EnvMgr,
                             StackManager &StackMgr,
                             Instance::MemoryInstance &MemInst) {
  return invoke<EEICallDataCopy>(EnvMgr, StackMgr, MemInst);
}

ErrCode EEICallDataCopy::body(VM::EnvironmentManager &EnvMgr,
                              Instance::MemoryInstance &MemInst,
                              uint32_t ResultOffset, uint32_t DataOffset,
                              uint32_t Length) {
  /// Add cost.
  if (!EnvMgr.addCost(Cost)) {
    return ErrCode::Revert;
  }

  if (Length > 0) {
    return MemInst.setBytes(Env.getCallData(), ResultOffset, DataOffset,
                            Length);
  }

  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
