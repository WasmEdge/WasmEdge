// SPDX-License-Identifier: Apache-2.0
#include "executor/instance/function.h"

namespace SSVM {
namespace Executor {
namespace Instance {

/// Setter of module address. See "include/executor/instance/function.h".
ErrCode FunctionInstance::setModuleAddr(unsigned int Addr) {
  if (IsHostFunction) {
    return ErrCode::FunctionInvalid;
  }
  ModuleAddr = Addr;
  return ErrCode::Success;
}

/// Setter of function type. See "include/executor/instance/function.h".
ErrCode FunctionInstance::setFuncType(ModuleInstance::FType *Type) {
  FuncType = Type;
  return ErrCode::Success;
}

/// Setter of host function. See "include/executor/instance/function.h".
ErrCode FunctionInstance::setHostFuncAddr(unsigned int Addr) {
  if (!IsHostFunction) {
    return ErrCode::FunctionInvalid;
  }
  HostFuncAddr = Addr;
  return ErrCode::Success;
}

/// Setter of locals vector. See "include/executor/instance/function.h".
ErrCode FunctionInstance::setLocals(
    const std::vector<std::pair<unsigned int, AST::ValType>> &Loc) {
  if (IsHostFunction) {
    return ErrCode::FunctionInvalid;
  }
  Locals = Loc;
  return ErrCode::Success;
}

/// Setter of function body. See "include/executor/instance/function.h".
ErrCode FunctionInstance::setInstrs(AST::InstrVec &Expr) {
  if (IsHostFunction) {
    return ErrCode::FunctionInvalid;
  }
  Instrs = std::move(Expr);
  return ErrCode::Success;
}

} // namespace Instance
} // namespace Executor
} // namespace SSVM
