#include "ast/section.h"
#include "executor/storemgr.h"

namespace AST {

/// Instantiate function types in Module Instance. See "include/ast/section.h".
Executor::ErrCode
TypeSection::instantiate(StoreMgr &Mgr,
                         std::unique_ptr<ModuleInstance> &ModInst) {
  Executor::ErrCode Status = Executor::ErrCode::Success;

  /// Move function types into module instance.
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
    itCode++;
    itType++;
  }
  Content.clear();
  return Status;
}

} // namespace AST