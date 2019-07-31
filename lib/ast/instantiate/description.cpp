#include "ast/description.h"

namespace AST {

/// Instantiation of import description. See "include/ast/description.h".
Executor::ErrCode ImportDesc::instantiate(StoreMgr &Mgr,
                                          unsigned int ModInstId) {
  Executor::ErrCode Status = Executor::ErrCode::Success;

  /// Get the module instance from ID.
  ModuleInstance *ModInst = nullptr;
  if ((Status = Mgr.getModule(ModInstId, ModInst)) !=
      Executor::ErrCode::Success)
    return Status;

  /// Add the imports into module istance.
  switch (ExtContent.index()) {
  case 0: /// Function type index
  {
    /// Find the function instance in Store.
    FunctionInstance *FuncInst = nullptr;
    if ((Status = Mgr.findFunction(ModName, ExtName, FuncInst)) !=
        Executor::ErrCode::Success)
      return Status;
    /// Set the function type index.
    if ((Status = FuncInst->setTypeIdx(*std::get<0>(ExtContent))) !=
        Executor::ErrCode::Success)
      return Status;
    /// Set the function address to module instance.
    if ((Status = ModInst->addFuncAddr(FuncInst->Id)) !=
        Executor::ErrCode::Success)
      return Status;
    break;
  }
  case 1: /// Table type TODO
  case 2: /// Memory type TODO
  case 3: /// Global type TODO
  default:
    break;
  }
  return Executor::ErrCode::Success;
}

} // namespace AST