#include "ast/type.h"

namespace SSVM {
namespace AST {

/// Instantiation of limit. See "include/ast/section.h".
template <typename T>
Executor::ErrCode Limit::instantiate(StoreMgr &Mgr,
                                     std::unique_ptr<T> &Instance) {
  /// Instantiation will only set Max value to instance.
  return Instance->setLimit(Type == LimitType::HasMinMax ? true : false, Max);
}

/// Instantiation of function type. See "include/ast/section.h".
Executor::ErrCode FunctionType::instantiate(StoreMgr &Mgr,
                                            unsigned int ModInstId) {
  Executor::ErrCode Status = Executor::ErrCode::Success;

  /// Get the module instance from ID.
  ModuleInstance *ModInst = nullptr;
  if ((Status = Mgr.getModule(ModInstId, ModInst)) !=
      Executor::ErrCode::Success)
    return Status;

  /// Instantiation will only move param and return lists to module instance.
  return ModInst->addFuncType(ParamTypes, ReturnTypes);
}

/// Instantiation of memory type. See "include/ast/section.h".
Executor::ErrCode
MemoryType::instantiate(StoreMgr &Mgr,
                        std::unique_ptr<MemoryInstance> &MemInst) {
  /// Instantiation will only set limit memory instance.
  return Memory->instantiate(Mgr, MemInst);
}

/// Instantiation of table type. See "include/ast/section.h".
Executor::ErrCode
TableType::instantiate(StoreMgr &Mgr, std::unique_ptr<TableInstance> &TabInst) {
  Executor::ErrCode Status = Executor::ErrCode::Success;

  /// Instantiation sets element type and limits to table instance.
  if ((Status = Table->instantiate(Mgr, TabInst)) != Executor::ErrCode::Success)
    return Status;
  return TabInst->setElemType(Type);
}

/// Instantiation of global type. See "include/ast/section.h".
Executor::ErrCode
GlobalType::instantiate(StoreMgr &Mgr,
                        std::unique_ptr<GlobalInstance> &GlobInst) {
  /// Instantiation sets value type and mutibility to global instance.
  return GlobInst->setGlobalType(Type, Mut);
}

} // namespace AST
} // namespace SSVM