#include "executor/instance/function.h"

namespace SSVM {
namespace Executor {
namespace Instance {

/// Setter of module address. See "include/executor/instance/function.h".
ErrCode FunctionInstance::setModuleAddr(unsigned int Addr) {
  ModuleAddr = Addr;
  return ErrCode::Success;
}

/// Setter of type index in module. See "include/executor/instance/function.h".
ErrCode FunctionInstance::setTypeIdx(unsigned int Id) {
  TypeIdx = Id;
  return ErrCode::Success;
}

/// Setter of locals vector. See "include/executor/instance/function.h".
ErrCode FunctionInstance::setLocals(
    const std::vector<std::pair<unsigned int, AST::ValType>> &Loc) {
  Locals = Loc;
  return ErrCode::Success;
}

/// Setter of function body. See "include/executor/instance/function.h".
ErrCode FunctionInstance::setInstrs(AST::InstrVec &Expr) {
  Instrs = std::move(Expr);
  return ErrCode::Success;
}

} // namespace Instance
} // namespace Executor
} // namespace SSVM
