// SPDX-License-Identifier: Apache-2.0
#include "vm/hostfunc/ethereum/getcalldatasize.h"
#include "executor/common.h"
#include "stdint.h"

namespace SSVM {
namespace Executor {

EEIGetCallDataSize::EEIGetCallDataSize(VM::EVMEnvironment &Env) : EEI(Env) {
  appendReturnDef(AST::ValType::I32);
}

ErrCode EEIGetCallDataSize::run(VM::EnvironmentManager &EnvMgr, std::vector<Value> &Args,
                                std::vector<Value> &Res, StoreManager &Store,
                                Instance::ModuleInstance *ModInst) {
  /// Arg: void
  if (Args.size() != 0) {
    return ErrCode::CallFunctionError;
  }

  /// Return: Length(u32)
  Res[0] = uint32_t(Env.getCallData().size());
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
