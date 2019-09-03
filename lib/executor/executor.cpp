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

  /// Initializa the tables and memories
  /// Make a new frame {ModInst, locals:none} and push
  auto Frame = std::make_unique<FrameEntry>(ModInstId, 0);
  StackMgr.push(Frame);

  /// Instantiate TableSection (TableSec, ElemSec)
  AST::TableSection *TabSec = Mod->getTableSection();
  AST::ElementSection *ElemSec = Mod->getElementSection();
  if ((Status = instantiate(TabSec, ElemSec)) != ErrCode::Success)
    return Status;

  /// Instantiate MemorySection (MemorySec, DataSec)
  AST::MemorySection *MemSec = Mod->getMemorySection();
  AST::DataSection *DataSec = Mod->getDataSection();
  if ((Status = instantiate(MemSec, DataSec)) != ErrCode::Success)
    return Status;

  /// Pop Frame.
  StackMgr.pop();

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
  auto &ImpDescs = ImportSec->getContent();
  for (auto ImpDesc = ImpDescs.begin(); ImpDesc != ImpDescs.end(); ImpDesc++) {
    /// Get data from import description.
    auto ExtType = (*ImpDesc)->getExternalType();
    const std::string &ModName = (*ImpDesc)->getModuleName();
    const std::string &ExtName = (*ImpDesc)->getExternalName();

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
      if ((Status = (*ImpDesc)->getExternalContent(TypeIdx)) !=
          ErrCode::Success)
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
  auto &FuncTypes = TypeSec->getContent();
  for (auto FuncType = FuncTypes.begin(); FuncType != FuncTypes.end();
       FuncType++) {
    /// Copy param and return lists to module instance.
    auto &Param = (*FuncType)->getParamTypes();
    auto &Return = (*FuncType)->getParamTypes();
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
  auto &TypeIdxs = FuncSec->getContent();

  /// Iterate through code segments to make function instances.
  auto &CodeSegs = CodeSec->getContent();
  auto CodeSeg = CodeSegs.begin();
  auto TypeIdx = TypeIdxs.begin();
  while (CodeSeg != CodeSegs.end() && TypeIdx != TypeIdxs.end()) {
    /// Make a new function instance.
    auto NewFuncInst = std::make_unique<Instance::FunctionInstance>();
    unsigned int NewFuncInstId = 0;
    auto &Locals = (*CodeSeg)->getLocals();
    auto &Instrs = (*CodeSeg)->getInstrs();

    /// Set function instance data.
    if ((Status = NewFuncInst->setModuleAddr(ModInst->Addr)) !=
        ErrCode::Success)
      return Status;
    if ((Status = NewFuncInst->setTypeIdx(*TypeIdx)) != ErrCode::Success)
      return Status;
    if ((Status = NewFuncInst->setLocals(Locals)) != ErrCode::Success)
      return Status;
    if ((Status = NewFuncInst->setInstrs(Instrs)) != ErrCode::Success)
      return Status;

    /// Insert function instance to store manager.
    if ((Status = StoreMgr.insertFunctionInst(NewFuncInst, NewFuncInstId)) !=
        ErrCode::Success)
      return Status;

    /// Set external value (function address) to module instance.
    if ((Status = ModInst->addFuncAddr(NewFuncInstId)) != ErrCode::Success)
      return Status;

    CodeSeg++;
    TypeIdx++;
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
  auto &GlobSegs = GlobSec->getContent();
  for (auto GlobSeg = GlobSegs.begin(); GlobSeg != GlobSegs.end(); GlobSeg++) {
    /// Make a new function instance.
    auto NewGlobInst = std::make_unique<Instance::GlobalInstance>();
    unsigned int NewGlobInstId = 0;

    /// Set global instance data.
    auto &GlobType = (*GlobSeg)->getGlobalType();
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
  auto Frame = std::make_unique<FrameEntry>(TmpModInstId, 0);
  StackMgr.push(Frame);

  /// Evaluate values and set to global instance.
  auto GlobSeg = GlobSegs.begin();
  for (unsigned int I = 0; I < TmpModInst->getGlobalNum(); I++, GlobSeg++) {
    /// Set init instrs to engine and run.
    if ((Status = Engine->runExpression((*GlobSeg)->getInstrs())) !=
        ErrCode::Success)
      return Status;

    /// Pop result from stack.
    std::unique_ptr<ValueEntry> PopVal;
    StackMgr.pop(PopVal);

    /// Get global instance from store.
    unsigned int GlobalAddr = 0;
    TmpModInst->getGlobalAddr(I, GlobalAddr);
    Instance::GlobalInstance *GlobInst;
    StoreMgr.getGlobal(GlobalAddr, GlobInst);

    /// Set value from value entry to global instance.
    AST::ValVariant Val;
    PopVal->getValue(Val);
    GlobInst->setValue(Val);
  }

  /// Pop Frame
  Status = StackMgr.pop();

  /// TODO: Delete the temp. module instance
  return Status;
}

/// Instantiate table instance. See "include/executor/executor.h".
ErrCode Executor::instantiate(AST::TableSection *TabSec,
                              AST::ElementSection *ElemSec) {
  if (TabSec == nullptr)
    return ErrCode::Success;
  ErrCode Status = ErrCode::Success;

  /// Get the module instance from ID.
  Instance::ModuleInstance *ModInst = nullptr;
  if ((Status = StoreMgr.getModule(ModInstId, ModInst)) != ErrCode::Success)
    return Status;

  /// Iterate and istantiate table types.
  auto &TabTypes = TabSec->getContent();
  for (auto TabType = TabTypes.begin(); TabType != TabTypes.end(); TabType++) {
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

  /// Iterate and evaluate element segments.
  auto &ElemSegs = ElemSec->getContent();
  for (auto ElemSeg = ElemSegs.begin(); ElemSeg != ElemSegs.end(); ElemSeg++) {
    /// Evaluate instrs in element segment for offset.
    if ((Status = Engine->runExpression((*ElemSeg)->getInstrs())) !=
        ErrCode::Success)
      return Status;

    /// Pop the result for offset.
    std::unique_ptr<ValueEntry> PopVal;
    StackMgr.pop(PopVal);
    int32_t Offset = 0;
    if ((Status = PopVal->getValue(Offset)) != ErrCode::Success)
      return Status;

    /// Get table instance
    Instance::TableInstance *TabInst = nullptr;
    unsigned int TabAddr = 0;
    if ((Status = ModInst->getMemAddr((*ElemSeg)->getIdx(), TabAddr)) !=
        ErrCode::Success)
      return Status;
    if ((Status = StoreMgr.getTable(TabAddr, TabInst)) != ErrCode::Success)
      return Status;

    /// Copy data to memory instance
    TabInst->setInitList(Offset, (*ElemSeg)->getFuncIdxes());
  }

  return Status;
}

/// Instantiate memory instance. See "include/executor/executor.h".
ErrCode Executor::instantiate(AST::MemorySection *MemSec,
                              AST::DataSection *DataSec) {
  if (MemSec == nullptr)
    return ErrCode::Success;
  ErrCode Status = ErrCode::Success;

  /// Get the module instance from ID.
  Instance::ModuleInstance *ModInst = nullptr;
  if ((Status = StoreMgr.getModule(ModInstId, ModInst)) != ErrCode::Success)
    return Status;

  /// Iterate and istantiate memory types.
  auto &MemTypes = MemSec->getContent();
  for (auto MemType = MemTypes.begin(); MemType != MemTypes.end(); MemType++) {
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

  /// Iterate and evaluate data segments.
  auto &DataSegs = DataSec->getContent();
  for (auto DataSeg = DataSegs.begin(); DataSeg != DataSegs.end(); DataSeg++) {
    /// Evaluate instrs in data segment for offset.
    if ((Status = Engine->runExpression((*DataSeg)->getInstrs())) !=
        ErrCode::Success)
      return Status;

    /// Pop the result for offset.
    std::unique_ptr<ValueEntry> PopVal;
    StackMgr.pop(PopVal);
    int32_t Offset = 0;
    if ((Status = PopVal->getValue(Offset)) != ErrCode::Success)
      return Status;

    /// Get memory instance
    Instance::MemoryInstance *MemInst = nullptr;
    unsigned int MemAddr = 0;
    if ((Status = ModInst->getMemAddr((*DataSeg)->getIdx(), MemAddr)) !=
        ErrCode::Success)
      return Status;
    if ((Status = StoreMgr.getMemory(MemAddr, MemInst)) != ErrCode::Success)
      return Status;

    /// Copy data to memory instance
    MemInst->setBytes((*DataSeg)->getData(), Offset,
                      (*DataSeg)->getData().size());
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
  auto &ExpDescs = ExportSec->getContent();
  for (auto ExpDesc = ExpDescs.begin(); ExpDesc != ExpDescs.end(); ExpDesc++) {
    /// TODO: make export instances. Only match start function now.
    /// Get data from export description.
    auto ExtType = (*ExpDesc)->getExternalType();
    const std::string &ExtName = (*ExpDesc)->getExternalName();
    unsigned int ExtIdx = (*ExpDesc)->getExternalIndex();

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
