// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/ethereum/getcaller.h"
#include "support/hexstr.h"

namespace SSVM {
namespace Executor {

EEIGetCaller::EEIGetCaller(VM::EVMEnvironment &Env, uint64_t Cost)
    : EEI(Env, Cost) {
  initializeFuncType<EEIGetCaller>();
}

ErrCode EEIGetCaller::run(VM::EnvironmentManager &EnvMgr,
                          StackManager &StackMgr,
                          Instance::MemoryInstance &MemInst) {
  return invoke<EEIGetCaller>(EnvMgr, StackMgr, MemInst);
}

ErrCode EEIGetCaller::body(VM::EnvironmentManager &EnvMgr,
                           Instance::MemoryInstance &MemInst,
                           uint32_t ResultOffset) {
  /// Add cost.
  if (!EnvMgr.addCost(Cost)) {
    return ErrCode::Revert;
  }

  std::vector<unsigned char> Data;
  Support::convertStringToHex(Env.getCaller(), Data, 40);
  return MemInst.setBytes(Data, ResultOffset, 0, 20);
}

} // namespace Executor
} // namespace SSVM
