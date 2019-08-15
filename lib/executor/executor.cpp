#include "executor/executor.h"
#include "ast/module.h"
#include "ast/section.h"
#include "executor/instance/module.h"

namespace SSVM {
namespace Executor {

/// Set AST Module node to executor. See "include/executor/executor.h".
ErrCode Executor::setModule(std::unique_ptr<AST::Module> &Module) {
  Mod = std::move(Module);
  return ErrCode::Success;
}

/// Instantiate module. See "include/executor/executor.h".
ErrCode Executor::instantiate() { return instantiate(Mod.get()); }

/// Instantiate module instance. See "include/executor/executor.h".
ErrCode Executor::instantiate(AST::Module *Mod) {
  if (Mod == nullptr)
    return ErrCode::Success;
  ErrCode Status = ErrCode::Success;
  auto ModInst = std::make_unique<Instance::ModuleInstance>();

  /// Insert the module instance to store manager.
  if ((Status = StoreMgr.insertModuleInst(ModInst, ModInstId)) !=
      ErrCode::Success)
    return Status;

  /// Instantiate ImportSection and do import matching. (ImportSec)
  AST::ImportSection *ImportSec = Mod->getImportSection();
  if ((Status = instantiate(ImportSec)) != ErrCode::Success)
    return Status;

  /// Instantiate Function Types in Module Instance. (TypeSec)
  AST::TypeSection *TypeSec = Mod->getTypeSection();
  if ((Status = instantiate(TypeSec)) != ErrCode::Success)
    return Status;

  /// Instantiate Functions in module. (FuncionSec, CodeSec)
  AST::FunctionSection *FuncSec = Mod->getFunctionSection();
  AST::CodeSection *CodeSec = Mod->getCodeSection();
  if ((Status = instantiate(FuncSec, CodeSec)) != ErrCode::Success)
    return Status;

  /// Instantiate GlobalSection (GlobalSec)
  AST::GlobalSection *GlobSec = Mod->getGlobalSection();
  if ((Status = instantiate(GlobSec)) != ErrCode::Success)
    return Status;

  /// TODO: Initializa the tables and memories
  /// Push Frame {ModInst, local:none}

  /// Instantiate TableSection (TableSec)
  AST::TableSection *TabSec = Mod->getTableSection();
  if ((Status = instantiate(TabSec)) != ErrCode::Success)
    return Status;

  /// Instantiate MemorySection (MemorySec)
  AST::MemorySection *MemSec = Mod->getMemorySection();
  if ((Status = instantiate(MemSec)) != ErrCode::Success)
    return Status;

  /// Instantiate ElementSection TODO
  ///   evaluate instrs in element segments

  /// Instantiate DataSection TODO
  ///   evaluate instrs in data segments

  /// Pop Frame.
  /// Replace data in table instance.
  /// Replace data in memory instance.

  /// Instantiate ExportSection (ExportSec)
  AST::ExportSection *ExportSec = Mod->getExportSection();
  if ((Status = instantiate(ExportSec)) != ErrCode::Success)
    return Status;

  /// Instantiate StartSection (StartSec)
  /// In e-wasm, the start section will always be "main" function.
  /// Therefore, the start function index will be find in export section.

  return Status;
}

/// Instantiate imports. See "include/executor/executor.h".
ErrCode Executor::instantiate(AST::ImportSection *ImportSec) {
  if (ImportSec == nullptr)
    return ErrCode::Success;
  ErrCode Status = ErrCode::Success;

  /// Get the module instance from ID.
  Instance::ModuleInstance *ModInst = nullptr;
  if ((Status = StoreMgr.getModule(ModInstId, ModInst)) != ErrCode::Success)
    return Status;

  /// Iterate and instantiate import descriptions.
  auto &Content = ImportSec->getContent();
  for (auto Desc = Content.begin(); Desc != Content.end(); Desc++) {
    /// Get data from import description.
    auto ExtType = (*Desc)->getExternalType();
    const std::string &ModName = (*Desc)->getModuleName();
    const std::string &ExtName = (*Desc)->getExternalName();

    /// Add the imports into module istance.
    switch (ExtType) {
    case AST::Desc::ExternalType::Function: /// Function type index
    {
      /// Find the function instance in Store.
      Instance::FunctionInstance *FuncInst = nullptr;
      if ((Status = StoreMgr.findFunction(ModName, ExtName, FuncInst)) !=
          ErrCode::Success)
        return Status;
      /// Set the function type index.
      unsigned int *TypeIdx = nullptr;
      if ((Status = (*Desc)->getExternalContent(TypeIdx)) != ErrCode::Success)
        return Status;
      if ((Status = FuncInst->setTypeIdx(*TypeIdx)) != ErrCode::Success)
        return Status;
      /// Set the function address to module instance.
      if ((Status = ModInst->addFuncAddr(FuncInst->Addr)) != ErrCode::Success)
        return Status;
      break;
    }
    case AST::Desc::ExternalType::Table:  /// Table type TODO
    case AST::Desc::ExternalType::Memory: /// Memory type TODO
    case AST::Desc::ExternalType::Global: /// Global type TODO
    default:
      break;
    }
  }
  return Status;
}

/// Instantiate types in module instance. See "include/executor/executor.h".
ErrCode Executor::instantiate(AST::TypeSection *TypeSec) {
  if (TypeSec == nullptr)
    return ErrCode::Success;
  ErrCode Status = ErrCode::Success;

  /// Get the module instance from ID.
  Instance::ModuleInstance *ModInst = nullptr;
  if ((Status = StoreMgr.getModule(ModInstId, ModInst)) != ErrCode::Success)
    return Status;

  /// Iterate and instantiate types.
  auto &Content = TypeSec->getContent();
  for (auto Func = Content.begin(); Func != Content.end(); Func++) {
    /// Copy param and return lists to module instance.
    auto &Param = (*Func)->getParamTypes();
    auto &Return = (*Func)->getParamTypes();
    if ((Status = ModInst->addFuncType(Param, Return)) != ErrCode::Success)
      return Status;
  }
  return Status;
}

/// Instantiate function instance. See "include/executor/executor.h".
ErrCode Executor::instantiate(AST::FunctionSection *FuncSec,
                              AST::CodeSection *CodeSec) {
  if (FuncSec == nullptr || CodeSec == nullptr)
    return ErrCode::Success;
  ErrCode Status = ErrCode::Success;

  /// Get the module instance from ID.
  Instance::ModuleInstance *ModInst = nullptr;
  if ((Status = StoreMgr.getModule(ModInstId, ModInst)) != ErrCode::Success)
    return Status;

  /// Get the function type indices.
  auto &TypeIdx = FuncSec->getContent();

  /// Iterate through code segments to make function instances.
  auto &Content = CodeSec->getContent();
  auto ItCode = Content.begin();
  auto ItType = TypeIdx.begin();
  while (ItCode != Content.end() && ItType != TypeIdx.end()) {
    /// Make a new function instance.
    auto NewFuncInst = std::make_unique<Instance::FunctionInstance>();
    unsigned int NewFuncInstId = 0;
    auto &Locals = (*ItCode)->getLocals();
    auto &Instrs = (*ItCode)->getExpression();

    /// Set function instance data.
    if ((Status = NewFuncInst->setModuleAddr(ModInst->Addr)) !=
        ErrCode::Success)
      return Status;
    if ((Status = NewFuncInst->setTypeIdx(*ItType)) != ErrCode::Success)
      return Status;
    if ((Status = NewFuncInst->setLocals(Locals)) != ErrCode::Success)
      return Status;
    if ((Status = NewFuncInst->setExpression(Instrs)) != ErrCode::Success)
      return Status;

    /// Insert function instance to store manager.
    if ((Status = StoreMgr.insertFunctionInst(NewFuncInst, NewFuncInstId)) !=
        ErrCode::Success)
      return Status;

    /// Set external value (function address) to module instance.
    if ((Status = ModInst->addFuncAddr(NewFuncInstId)) != ErrCode::Success)
      return Status;

    ItCode++;
    ItType++;
  }
  return Status;
}

/// Instantiate global instance. See "include/executor/executor.h".
ErrCode Executor::instantiate(AST::GlobalSection *GlobSec) {
  if (GlobSec == nullptr)
    return ErrCode::Success;
  ErrCode Status = ErrCode::Success;

  /// Get the module instance from ID.
  Instance::ModuleInstance *ModInst = nullptr;
  if ((Status = StoreMgr.getModule(ModInstId, ModInst)) != ErrCode::Success)
    return Status;

  /// Add a temp module to Store for initialization
  auto TmpMod = std::make_unique<Instance::ModuleInstance>();

  /// Iterate and instantiate global segments.
  auto &Content = GlobSec->getContent();
  for (auto Seg = Content.begin(); Seg != Content.end(); Seg++) {
    /// Make a new function instance.
    auto NewGlobInst = std::make_unique<Instance::GlobalInstance>();
    unsigned int NewGlobInstId = 0;

    /// Set global instance data.
    auto &GlobType = (*Seg)->getGlobalType();
    auto Type = GlobType->getValueType();
    auto Mut = GlobType->getValueMutation();
    if ((Status = NewGlobInst->setGlobalType(Type, Mut)) != ErrCode::Success)
      return Status;

    /// Insert global instance to store manager.
    if ((Status = StoreMgr.insertGlobalInst(NewGlobInst, NewGlobInstId)) !=
        ErrCode::Success)
      return Status;

    /// Set external value (global address) to module instance.
    if ((Status = ModInst->addGlobalAddr(NewGlobInstId)) != ErrCode::Success)
      return Status;

    /// Set external value (global address) to temp module instance.
    if ((Status = TmpMod->addGlobalAddr(NewGlobInstId)) != ErrCode::Success)
      return Status;
  }

  /// Initialize the globals
  /// Insert the temp. module instance to Store
  unsigned int TmpModInstId = 0;
  if ((Status = StoreMgr.insertModuleInst(TmpMod, TmpModInstId)) !=
      ErrCode::Success)
    return Status;
  Instance::ModuleInstance *TmpModInst = nullptr;
  if ((Status = StoreMgr.getModule(TmpModInstId, TmpModInst)) !=
      ErrCode::Success)
    return Status;

  /// Make a new frame {NewModInst:{globaddrs}, locals:none} and push
  auto Frame = std::make_unique<Entry::FrameEntry>(TmpModInstId, 0);
  StackMgr.push(Frame);

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
    Executor::GlobalInstance *GlobInst;
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
  StackMgr.pop();

  /// TODO: Delete the temp. module instance
  return Status;
}

/// Instantiate table instance. See "include/executor/executor.h".
ErrCode Executor::instantiate(AST::TableSection *TabSec) {
  if (TabSec == nullptr)
    return ErrCode::Success;
  ErrCode Status = ErrCode::Success;

  /// Get the module instance from ID.
  Instance::ModuleInstance *ModInst = nullptr;
  if ((Status = StoreMgr.getModule(ModInstId, ModInst)) != ErrCode::Success)
    return Status;

  /// Iterate and istantiate table types.
  auto &Content = TabSec->getContent();
  for (auto TabType = Content.begin(); TabType != Content.end(); TabType++) {
    /// Make a new table instance.
    auto NewTabInst = std::make_unique<Instance::TableInstance>();
    unsigned int NewTabInstId = 0;

    /// Set table instance data.
    auto ElemType = (*TabType)->getElementType();
    auto &Limit = (*TabType)->getLimit();
    if ((Status = NewTabInst->setElemType(ElemType)) != ErrCode::Success)
      return Status;
    if ((Status = NewTabInst->setLimit(Limit->hasMax(), Limit->getMax())) !=
        ErrCode::Success)
      return Status;

    /// Insert table instance to store manager.
    if ((Status = StoreMgr.insertTableInst(NewTabInst, NewTabInstId)) !=
        ErrCode::Success)
      return Status;

    /// Set external value (table address) to module instance.
    if ((Status = ModInst->addTableAddr(NewTabInstId)) != ErrCode::Success)
      return Status;
  }
  return Status;
}

/// Instantiate memory instance. See "include/executor/executor.h".
ErrCode Executor::instantiate(AST::MemorySection *MemSec) {
  if (MemSec == nullptr)
    return ErrCode::Success;
  ErrCode Status = ErrCode::Success;

  /// Get the module instance from ID.
  Instance::ModuleInstance *ModInst = nullptr;
  if ((Status = StoreMgr.getModule(ModInstId, ModInst)) != ErrCode::Success)
    return Status;

  /// Iterate and istantiate memory types.
  auto &Content = MemSec->getContent();
  for (auto MemType = Content.begin(); MemType != Content.end(); MemType++) {
    /// Make a new memory instance.
    auto NewMemInst = std::make_unique<Instance::MemoryInstance>();
    unsigned int NewMemInstId = 0;

    /// Set memory instance data.
    auto &Limit = (*MemType)->getLimit();
    if ((Status = NewMemInst->setLimit(Limit->hasMax(), Limit->getMax())) !=
        ErrCode::Success)
      return Status;

    /// Insert memory instance to store manager.
    if ((Status = StoreMgr.insertMemoryInst(NewMemInst, NewMemInstId)) !=
        ErrCode::Success)
      return Status;

    /// Set external value (memory address) to module instance.
    if ((Status = ModInst->addMemAddr(NewMemInstId)) != ErrCode::Success)
      return Status;
  }
  return Status;
}

/// Instantiate export instance. See "include/executor/executor.h".
ErrCode Executor::instantiate(AST::ExportSection *ExportSec) {
  if (ExportSec == nullptr)
    return ErrCode::Success;
  ErrCode Status = ErrCode::Success;

  /// Get the module instance from ID.
  Instance::ModuleInstance *ModInst = nullptr;
  if ((Status = StoreMgr.getModule(ModInstId, ModInst)) != ErrCode::Success)
    return Status;

  /// Iterate and istantiate export descriptions.
  auto &Content = ExportSec->getContent();
  for (auto Desc = Content.begin(); Desc != Content.end(); Desc++) {
    /// TODO: make export instances. Only match start function now.
    /// Get data from export description.
    auto ExtType = (*Desc)->getExternalType();
    const std::string &ExtName = (*Desc)->getExternalName();
    unsigned int ExtIdx = (*Desc)->getExternalIndex();

    /// TODO: make export instance and add to module.
    /// Add the name of function to function instance.
    if (ExtType == AST::Desc::ExternalType::Function) {
      unsigned int FuncAddr = 0;
      Instance::FunctionInstance *FuncInst = nullptr;
      /// Find function address.
      if ((Status = ModInst->getFuncAddr(ExtIdx, FuncAddr)) != ErrCode::Success)
        return Status;
      /// Get function instance.
      if ((Status = StoreMgr.getFunction(FuncAddr, FuncInst)) !=
          ErrCode::Success)
        return Status;
      /// Set function name. TODO: module name
      if ((Status = FuncInst->setNames("", ExtName)) != ErrCode::Success)
        return Status;
      /// Set start function index.
      if (ExtName == "main") {
        if ((Status = ModInst->setStartIdx(ExtIdx)) != ErrCode::Success)
          return Status;
      }
    }
  }
  return Status;
}

} // namespace Executor
} // namespace SSVM
