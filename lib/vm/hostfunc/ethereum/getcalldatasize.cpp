// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/ethereum/getcalldatasize.h"
#include "executor/common.h"
#include "stdint.h"

namespace SSVM {
namespace Executor {

EEIGetCallDataSize::EEIGetCallDataSize(VM::EVMEnvironment &Env, uint64_t Cost)
    : EEI(Env, Cost) {
  appendReturnDef(AST::ValType::I32);
}

ErrCode EEIGetCallDataSize::run(VM::EnvironmentManager &EnvMgr,
                                std::vector<Value> &Args,
                                std::vector<Value> &Res, StoreManager &Store,
                                Instance::ModuleInstance *ModInst) {
  /// Arg: void
  if (Args.size() != 0) {
    return ErrCode::CallFunctionError;
  }
  /// Add cost.
  if (!EnvMgr.addCost(Cost)) {
    return ErrCode::Revert;
  }

  /// Return: Length(u32)
  Res[0] = uint32_t(Env.getCallData().size());
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
