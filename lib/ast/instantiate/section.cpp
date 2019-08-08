#include "ast/section.h"
#include "executor/storemgr.h"

namespace AST {

/// Instantiate function types in Module Instance. See "include/ast/section.h".
Executor::ErrCode TypeSection::instantiate(StoreMgr &Mgr,
                                           unsigned int ModInstId) {
  Executor::ErrCode Status = Executor::ErrCode::Success;

  /// Recursively call function types' instantiation.
  for (auto it = Content.begin(); it != Content.end(); it++) {
    if ((Status = (*it)->instantiate(Mgr, ModInstId)) !=
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

/// Instantiation of import section. See "include/ast/section.h".
Executor::ErrCode ImportSection::instantiate(StoreMgr &Mgr,
                                             unsigned int ModInstId) {
  Executor::ErrCode Status = Executor::ErrCode::Success;

  /// Recursively call import descriptions' instantiation.
  for (auto it = Content.begin(); it != Content.end(); it++) {
    if ((Status = (*it)->instantiate(Mgr, ModInstId)) !=
        Executor::ErrCode::Success)
      return Status;
  }
  Content.clear();
  return Status;
}

/// Instantiation of table section. See "include/ast/section.h".
Executor::ErrCode TableSection::instantiate(StoreMgr &Mgr,
                                            unsigned int ModInstId) {
  Executor::ErrCode Status = Executor::ErrCode::Success;

  /// Get the module instance from ID.
  ModuleInstance *ModInst = nullptr;
  if ((Status = Mgr.getModule(ModInstId, ModInst)) !=
      Executor::ErrCode::Success)
    return Status;

  /// Recursively call table types' instantiation.
  for (auto it = Content.begin(); it != Content.end(); it++) {
    /// Make a new table instance.
    auto NewTabInst = std::make_unique<TableInstance>();
    unsigned int NewTabInstId = 0;
    /// Set table instance data.
    if ((Status = (*it)->instantiate(Mgr, NewTabInst)) !=
        Executor::ErrCode::Success)
      return Status;
    /// Insert table instance to store manager.
    if ((Status = Mgr.insertTableInst(NewTabInst, NewTabInstId)) !=
        Executor::ErrCode::Success)
      return Status;
    /// Set external value (table address) to module instance.
    if ((Status = ModInst->addTableAddr(NewTabInstId)) !=
        Executor::ErrCode::Success)
      return Status;
  }
  Content.clear();
  return Status;
}

/// Instantiation of memory section. See "include/ast/section.h".
Executor::ErrCode MemorySection::instantiate(StoreMgr &Mgr,
                                             unsigned int ModInstId) {
  Executor::ErrCode Status = Executor::ErrCode::Success;

  /// Get the module instance from ID.
  ModuleInstance *ModInst = nullptr;
  if ((Status = Mgr.getModule(ModInstId, ModInst)) !=
      Executor::ErrCode::Success)
    return Status;

  /// Recursively call memory types' instantiation.
  for (auto it = Content.begin(); it != Content.end(); it++) {
    /// Make a new memory instance.
    auto NewMemInst = std::make_unique<MemoryInstance>();
    unsigned int NewMemInstId = 0;
    /// Set memory instance data.
    if ((Status = (*it)->instantiate(Mgr, NewMemInst)) !=
        Executor::ErrCode::Success)
      return Status;
    /// Insert table instance to store manager.
    if ((Status = Mgr.insertMemoryInst(NewMemInst, NewMemInstId)) !=
        Executor::ErrCode::Success)
      return Status;
    /// Set external value (memory address) to module instance.
    if ((Status = ModInst->addMemAddr(NewMemInstId)) !=
        Executor::ErrCode::Success)
      return Status;
  }
  Content.clear();
  return Status;
}

/// Instantiation of global section. See "include/ast/section.h".
Executor::ErrCode GlobalSection::instantiate(StoreMgr &Store, StackMgr &Stack,
                                             unsigned int ModInstId) {
  Executor::ErrCode Status = Executor::ErrCode::Success;

  /// Get the module instance from ID.
  ModuleInstance *ModInst = nullptr;
  if ((Status = Store.getModule(ModInstId, ModInst)) !=
      Executor::ErrCode::Success)
    return Status;

  /// Add a temp module to Store for initialization
  auto TmpModInst = std::make_unique<ModuleInstance>();

  /// Recursively call global types' instantiation.
  for (auto it = Content.begin(); it != Content.end(); it++) {
    /// Make a new function instance.
    auto NewGlobInst = std::make_unique<GlobalInstance>();
    unsigned int NewGlobInstId = 0;
    /// Set global instance data.
    if ((Status = (*it)->instantiate(Store, NewGlobInst)) !=
        Executor::ErrCode::Success)
      return Status;
    /// Insert global instance to store manager.
    if ((Status = Store.insertGlobalInst(NewGlobInst, NewGlobInstId)) !=
        Executor::ErrCode::Success)
      return Status;
    /// Set external value (global address) to module instance.
    if ((Status = ModInst->addGlobalAddr(NewGlobInstId)) !=
        Executor::ErrCode::Success)
      return Status;
    /// Set external value (global address) to temp module instance.
    if ((Status = TmpModInst->addGlobalAddr(NewGlobInstId)) !=
        Executor::ErrCode::Success)
      return Status;
  }
  Content.clear();

  /// Initialize the globals
  /// Insert the temp. module instance to Store
  unsigned int TmpModInstId = 0;
  if ((Status = Store.insertModuleInst(TmpModInst, TmpModInstId)) !=
      Executor::ErrCode::Success)
    return Status;
  ModuleInstance *TmpModInstPtr = nullptr;
  if ((Status = Store.getModule(TmpModInstId, TmpModInstPtr)) !=
      Executor::ErrCode::Success)
    return Status;

  /// Make a new frame {NewModInst:{globaddrs}, locals:none} and push
  auto Frame = std::make_unique<FrameEntry>(TmpModInstId, 0);
  Stack.push(Frame);

  /// TODO: evaluate instrs in global instances

  /// Get the values
  /*
  unsigned int GlobalNum = 0;
  if ((Status = TmpModInstPtr->getGlobalNum(GlobalNum)) !=
      Executor::ErrCode::Success)
    return Status;
  while (GlobalNum--) {
    std::unique_ptr<ValueEntry> PopVal;
    Stack.pop(PopVal);
    unsigned int GlobalAddr = 0;
    TmpModInstPtr->getGlobalAddr(GlobalNum, GlobalAddr);
    GlobalInstance *GlobInst;
    Store.getGlobal(GlobalAddr, GlobInst);
    ValType GlobType;
    PopVal->getType(GlobType);
    switch (GlobType) {
    case ValType::I32: {
      int32_t GlobVal = 0;
      PopVal->getValue(GlobVal);
      GlobInst->setValue(GlobVal);
      break;
    }
    case ValType::I64: {
      int64_t GlobVal = 0;
      PopVal->getValue(GlobVal);
      GlobInst->setValue(GlobVal);
      break;
    }
    case ValType::F32: {
      float GlobVal = 0;
      PopVal->getValue(GlobVal);
      GlobInst->setValue(GlobVal);
      break;
    }
    case ValType::F64: {
      double GlobVal = 0;
      PopVal->getValue(GlobVal);
      GlobInst->setValue(GlobVal);
      break;
    }
    default:
      break;
    }
  }
  */

  /// Pop Frame
  Stack.pop(Frame);

  /// TODO: Delete the temp. module instance

  return Status;
}

/// Instantiation of export section. See "include/ast/section.h".
Executor::ErrCode ExportSection::instantiate(StoreMgr &Mgr,
                                             unsigned int ModInstId) {
  Executor::ErrCode Status = Executor::ErrCode::Success;

  /// Recursively call export descriptions' instantiation.
  for (auto it = Content.begin(); it != Content.end(); it++) {
    /// TODO: make export instances. Only match start function now.
    if ((Status = (*it)->instantiate(Mgr, ModInstId)) !=
        Executor::ErrCode::Success)
      return Status;
  }
  Content.clear();
  return Status;
}

/// Instantiate function instances. See "include/ast/section.h".
Executor::ErrCode
CodeSection::instantiate(StoreMgr &Mgr, unsigned int ModInstId,
                         std::unique_ptr<AST::FunctionSection> &FuncSec) {
  Executor::ErrCode Status = Executor::ErrCode::Success;

  /// Get the function type indices.
  std::vector<unsigned int> TypeIdx;
  if ((Status = FuncSec->instantiate(Mgr, TypeIdx)) !=
      Executor::ErrCode::Success)
    return Status;

  /// Get the module instance from ID.
  ModuleInstance *ModInst = nullptr;
  if ((Status = Mgr.getModule(ModInstId, ModInst)) !=
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
    if ((Status = NewFuncInst->setModuleAddr(ModInst->Addr)) !=
        Executor::ErrCode::Success)
      return Status;
    if ((Status = NewFuncInst->setTypeIdx(*itType)) !=
        Executor::ErrCode::Success)
      return Status;
    if ((Status = (*itCode)->instantiate(Mgr, NewFuncInst)) !=
        Executor::ErrCode::Success)
      return Status;
    /// Insert function instance to store manager.
    if ((Status = Mgr.insertFunctionInst(NewFuncInst, NewFuncInstId)) !=
        Executor::ErrCode::Success)
      return Status;
    /// Set external value (function address) to module instance.
    if ((Status = ModInst->addFuncAddr(NewFuncInstId)) !=
        Executor::ErrCode::Success)
      return Status;

    itCode++;
    itType++;
  }
  Content.clear();
  return Status;
}

} // namespace AST