#include "ast/description.h"

namespace SSVM {
namespace AST {

/// Instantiation of import description. See "include/ast/description.h".
Executor::ErrCode ImportDesc::instantiate(Executor::StoreManager &Mgr,
                                          unsigned int ModInstId) {
  Executor::ErrCode Status = Executor::ErrCode::Success;

  /// Get the module instance from ID.
  Executor::ModuleInstance *ModInst = nullptr;
  if ((Status = Mgr.getModule(ModInstId, ModInst)) !=
      Executor::ErrCode::Success)
    return Status;

  /// Add the imports into module istance.
  switch (ExtType) {
  case ExternalType::Function: /// Function type index
  {
    /// Find the function instance in Store.
    Executor::FunctionInstance *FuncInst = nullptr;
    if ((Status = Mgr.findFunction(ModName, ExtName, FuncInst)) !=
        Executor::ErrCode::Success)
      return Status;
    /// Set the function type index.
    if ((Status = FuncInst->setTypeIdx(*std::get<0>(ExtContent))) !=
        Executor::ErrCode::Success)
      return Status;
    /// Set the function address to module instance.
    if ((Status = ModInst->addFuncAddr(FuncInst->Addr)) !=
        Executor::ErrCode::Success)
      return Status;
    break;
  }
  case ExternalType::Table:  /// Table type TODO
  case ExternalType::Memory: /// Memory type TODO
  case ExternalType::Global: /// Global type TODO
  default:
    break;
  }
  return Executor::ErrCode::Success;
}

/// Instantiation of import description. See "include/ast/description.h".
Executor::ErrCode ExportDesc::instantiate(Executor::StoreManager &Mgr,
                                          unsigned int ModInstId) {
  Executor::ErrCode Status = Executor::ErrCode::Success;

  /// Get the module instance from ID.
  Executor::ModuleInstance *ModInst = nullptr;
  if ((Status = Mgr.getModule(ModInstId, ModInst)) !=
      Executor::ErrCode::Success)
    return Status;

  /// Add the name of function to function instance.
  if (ExtType == ExternalType::Function) {
    unsigned int FuncAddr = 0;
    Executor::FunctionInstance *FuncInst = nullptr;
    /// Find function address.
    if ((Status = ModInst->getFuncAddr(ExtIdx, FuncAddr)) !=
        Executor::ErrCode::Success)
      return Status;
    /// Get function instance.
    if ((Status = Mgr.getFunction(FuncAddr, FuncInst)) !=
        Executor::ErrCode::Success)
      return Status;
    /// Set function name. TODO: module name
    if ((Status = FuncInst->setNames("", ExtName)) !=
        Executor::ErrCode::Success)
      return Status;
    /// Set start function index.
    if (ExtName == "main") {
      if ((Status = ModInst->setStartIdx(ExtIdx)) != Executor::ErrCode::Success)
        return Status;
    }
  }

  /// TODO: make export instance and add to module.

  return Executor::ErrCode::Success;
}

} // namespace AST
} // namespace SSVM
