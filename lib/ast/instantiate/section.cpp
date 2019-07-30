#include "ast/section.h"
#include "executor/storemgr.h"

namespace AST {

/// Instantiate function types in Module Instance. See "include/ast/section.h".
Executor::ErrCode
TypeSection::instantiate(StoreMgr &Mgr,
                         std::unique_ptr<ModuleInstance> &ModInst) {
  Executor::ErrCode Status = Executor::ErrCode::Success;

  /// Recursively call function types' instantiation.
  for (auto it = Content.begin(); it != Content.end(); it++) {
    if ((Status = (*it)->instantiate(Mgr, ModInst)) !=
        Executor::ErrCode::Success)
      return Status;
  }
  Content.clear();
  return Status;
}

/// Instantiation of function section. See "include/ast/section.h".
Executor::ErrCode
FunctionSection::instantiate(StoreMgr &Mgr,
                             std::vector<unsigned int> &TypeIdx) {
  /// Instantiation will only move content to output.
  TypeIdx = std::move(Content);
  return Executor::ErrCode::Success;
}

/// Instantiation of table section. See "include/ast/section.h".
Executor::ErrCode
TableSection::instantiate(StoreMgr &Mgr,
                          std::unique_ptr<ModuleInstance> &ModInst) {
  Executor::ErrCode Status = Executor::ErrCode::Success;

  /// Recursively call table types' instantiation.
  unsigned int Idx = 0;
  for (auto it = Content.begin(); it != Content.end(); it++) {
    /// Make a new table instance.
    auto NewTabInst = std::make_unique<TableInstance>();
    unsigned int NewTabInstId = 0;
    /// Set table instance data.
    if ((Status = (*it)->instantiate(Mgr, NewTabInst)) !=
        Executor::ErrCode::Success)
      return Status;
    /// Insert table instance to store manager.
    if ((Status = Mgr.queryTableEntry(NewTabInstId)) !=
        Executor::ErrCode::Success)
      return Status;
    NewTabInst->Id = NewTabInstId;
    if ((Status = Mgr.insertTableInst(NewTabInstId, std::move(NewTabInst))) !=
        Executor::ErrCode::Success)
      return Status;
    /// Set external value (table address) to module instance.
    if ((Status = ModInst->setTableAddr(Idx, NewTabInstId)) !=
        Executor::ErrCode::Success)
      return Status;

    Idx++;
  }
  Content.clear();
  return Status;
}

/// Instantiation of memory section. See "include/ast/section.h".
Executor::ErrCode
MemorySection::instantiate(StoreMgr &Mgr,
                           std::unique_ptr<ModuleInstance> &ModInst) {
  Executor::ErrCode Status = Executor::ErrCode::Success;

  /// Recursively call memory types' instantiation.
  unsigned int Idx = 0;
  for (auto it = Content.begin(); it != Content.end(); it++) {
    /// Make a new memory instance.
    auto NewMemInst = std::make_unique<MemoryInstance>();
    unsigned int NewMemInstId = 0;
    /// Set memory instance data.
    if ((Status = (*it)->instantiate(Mgr, NewMemInst)) !=
        Executor::ErrCode::Success)
      return Status;
    /// Insert table instance to store manager.
    if ((Status = Mgr.queryMemoryEntry(NewMemInstId)) !=
        Executor::ErrCode::Success)
      return Status;
    NewMemInst->Id = NewMemInstId;
    if ((Status = Mgr.insertMemoryInst(NewMemInstId, std::move(NewMemInst))) !=
        Executor::ErrCode::Success)
      return Status;
    /// Set external value (memory address) to module instance.
    if ((Status = ModInst->setTableAddr(Idx, NewMemInstId)) !=
        Executor::ErrCode::Success)
      return Status;

    Idx++;
  }
  Content.clear();
  return Status;
}

/// Instantiation of global section. See "include/ast/section.h".
Executor::ErrCode
GlobalSection::instantiate(StoreMgr &Mgr,
                           std::unique_ptr<ModuleInstance> &ModInst) {
  Executor::ErrCode Status = Executor::ErrCode::Success;

  /// Recursively call global types' instantiation.
  unsigned int Idx = 0;
  for (auto it = Content.begin(); it != Content.end(); it++) {
    /// Make a new function instance.
    auto NewGlobInst = std::make_unique<GlobalInstance>();
    unsigned int NewGlobInstId = 0;
    /// Set global instance data.
    if ((Status = (*it)->instantiate(Mgr, NewGlobInst)) !=
        Executor::ErrCode::Success)
      return Status;
    /// Insert global instance to store manager.
    if ((Status = Mgr.queryGlobalEntry(NewGlobInstId)) !=
        Executor::ErrCode::Success)
      return Status;
    NewGlobInst->Id = NewGlobInstId;
    if ((Status =
             Mgr.insertGlobalInst(NewGlobInstId, std::move(NewGlobInst))) !=
        Executor::ErrCode::Success)
      return Status;
    /// Set external value (global address) to module instance.
    if ((Status = ModInst->setFuncAddr(Idx, NewGlobInstId)) !=
        Executor::ErrCode::Success)
      return Status;

    Idx++;
  }
  Content.clear();
  return Status;
}

/// Instantiate function instances. See "include/ast/section.h".
Executor::ErrCode
CodeSection::instantiate(StoreMgr &Mgr,
                         std::unique_ptr<ModuleInstance> &ModInst,
                         std::unique_ptr<AST::FunctionSection> &FuncSec) {
  Executor::ErrCode Status = Executor::ErrCode::Success;

  /// Get the function type indices.
  std::vector<unsigned int> TypeIdx;
  if ((Status = FuncSec->instantiate(Mgr, TypeIdx)) !=
      Executor::ErrCode::Success)
    return Status;

  /// Iterate through code segments to make function instances.
  auto itCode = Content.begin();
  auto itType = TypeIdx.begin();
  unsigned int Idx = 0;
  while (itCode != Content.end() && itType != TypeIdx.end()) {
    /// Make a new function instance.
    auto NewFuncInst = std::make_unique<FunctionInstance>();
    unsigned int NewFuncInstId = 0;
    /// Set function instance data.
    if ((Status = NewFuncInst->setModuleIdx(ModInst->Id)) !=
        Executor::ErrCode::Success)
      return Status;
    if ((Status = NewFuncInst->setTypeIdx(*itType)) !=
        Executor::ErrCode::Success)
      return Status;
    if ((Status = (*itCode)->instantiate(Mgr, NewFuncInst)) !=
        Executor::ErrCode::Success)
      return Status;
    /// Insert function instance to store manager.
    if ((Status = Mgr.queryFunctionEntry(NewFuncInstId)) !=
        Executor::ErrCode::Success)
      return Status;
    NewFuncInst->Id = NewFuncInstId;
    if ((Status =
             Mgr.insertFunctionInst(NewFuncInstId, std::move(NewFuncInst))) !=
        Executor::ErrCode::Success)
      return Status;
    /// Set external value (function address) to module instance.
    if ((Status = ModInst->setFuncAddr(Idx, NewFuncInstId)) !=
        Executor::ErrCode::Success)
      return Status;

    itCode++;
    itType++;
    Idx++;
  }
  Content.clear();
  return Status;
}

} // namespace AST