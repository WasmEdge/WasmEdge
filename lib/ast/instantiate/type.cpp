#include "ast/type.h"

namespace AST {

/// Instantiation of function section. See "include/ast/section.h".
Executor::ErrCode
FunctionType::instantiate(StoreMgr &Mgr,
                          std::unique_ptr<ModuleInstance> &ModInst) {
  /// Instantiation will only move param and return lists to module instance.
  return ModInst->addFuncType(ParamTypes, ReturnTypes);
}

} // namespace AST