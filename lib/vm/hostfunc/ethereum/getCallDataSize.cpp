// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/ethereum/getCallDataSize.h"

namespace SSVM {
namespace Executor {

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
