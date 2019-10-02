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
  if (IsHostFunction) {
    return ErrCode::FunctionInvalid;
  }
  FuncType = Type;
  return ErrCode::Success;
}

/// Setter of host function. See "include/executor/instance/function.h".
ErrCode FunctionInstance::setHostFunc(std::unique_ptr<HostFunction> &Func) {
  if (!IsHostFunction) {
    return ErrCode::FunctionInvalid;
  }
  HostFunc = std::move(Func);
  FuncType = HostFunc->getFuncType();
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
