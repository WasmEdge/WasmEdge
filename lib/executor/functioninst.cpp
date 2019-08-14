#include "executor/functioninst.h"

namespace SSVM {
namespace Executor {

/// Setter of module address. See "include/executor/functioninst.h".
Executor::ErrCode FunctionInstance::setModuleAddr(unsigned int Addr) {
  ModuleAddr = Addr;
  return Executor::ErrCode::Success;
}

/// Setter of type index in module. See "include/executor/functioninst.h".
Executor::ErrCode FunctionInstance::setTypeIdx(unsigned int Id) {
  TypeIdx = Id;
  return Executor::ErrCode::Success;
}

/// Setter of locals vector. See "include/executor/functioninst.h".
Executor::ErrCode FunctionInstance::setLocals(
    std::vector<std::pair<unsigned int, AST::ValType>> &Loc) {
  Locals = std::move(Loc);
  return Executor::ErrCode::Success;
}

/// Setter of function body. See "include/executor/functioninst.h".
Executor::ErrCode FunctionInstance::setExpression(
    std::vector<std::unique_ptr<AST::Instruction>> &Expr) {
  Instrs = std::move(Expr);
  return Executor::ErrCode::Success;
}

/// Setter of module and function name. See "include/executor/functioninst.h".
Executor::ErrCode FunctionInstance::setNames(const std::string &Mod,
                                             const std::string &Func) {
  ModName = Mod;
  FuncName = Func;
  return Executor::ErrCode::Success;
}

/// Match the module and function name.
bool FunctionInstance::isName(const std::string &Mod, const std::string &Func) {
  return Mod == ModName && Func == FuncName;
}

} // namespace Executor
} // namespace SSVM
