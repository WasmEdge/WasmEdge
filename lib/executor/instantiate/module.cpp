#include "ast/module.h"
#include "ast/section.h"
#include "executor/executor.h"
#include "executor/instance/module.h"

namespace SSVM {
namespace Executor {

/// Instantiate module instance. See "include/executor/executor.h".
ErrCode Executor::instantiate(AST::Module *Mod) {
  if (Mod == nullptr) {
    return ErrCode::Success;
  }
  ErrCode Status = ErrCode::Success;
  auto NewModInst = std::make_unique<Instance::ModuleInstance>();

  /// Insert the module instance to store manager and retieve instance.
  unsigned int ModInstAddr;
  if ((Status = StoreMgr.insertModuleInst(NewModInst, ModInstAddr)) !=
      ErrCode::Success) {
    return Status;
  }
  if ((Status = StoreMgr.getModule(ModInstAddr, ModInst)) != ErrCode::Success) {
    return Status;
  }

  /// Instantiate ImportSection and do import matching. (ImportSec)
  AST::ImportSection *ImportSec = Mod->getImportSection();
  if ((Status = instantiate(ImportSec)) != ErrCode::Success) {
    return Status;
  }

  /// Instantiate Function Types in Module Instance. (TypeSec)
  AST::TypeSection *TypeSec = Mod->getTypeSection();
  if ((Status = instantiate(TypeSec)) != ErrCode::Success) {
    return Status;
  }

  /// Instantiate Functions in module. (FuncionSec, CodeSec)
  AST::FunctionSection *FuncSec = Mod->getFunctionSection();
  AST::CodeSection *CodeSec = Mod->getCodeSection();
  if ((Status = instantiate(FuncSec, CodeSec)) != ErrCode::Success) {
    return Status;
  }

  /// Instantiate GlobalSection (GlobalSec)
  AST::GlobalSection *GlobSec = Mod->getGlobalSection();
  if ((Status = instantiate(GlobSec)) != ErrCode::Success) {
    return Status;
  }

  /// Initializa the tables and memories
  /// Make a new frame {ModInst, locals:none} and push
  auto Frame = std::make_unique<FrameEntry>(ModInst->Addr, 0);
  StackMgr.push(Frame);

  /// Instantiate TableSection (TableSec, ElemSec)
  AST::TableSection *TabSec = Mod->getTableSection();
  AST::ElementSection *ElemSec = Mod->getElementSection();
  if ((Status = instantiate(TabSec, ElemSec)) != ErrCode::Success) {
    return Status;
  }

  /// Instantiate MemorySection (MemorySec, DataSec)
  AST::MemorySection *MemSec = Mod->getMemorySection();
  AST::DataSection *DataSec = Mod->getDataSection();
  if ((Status = instantiate(MemSec, DataSec)) != ErrCode::Success) {
    return Status;
  }

  /// Pop Frame.
  StackMgr.pop();

  /// Instantiate ExportSection (ExportSec)
  AST::ExportSection *ExportSec = Mod->getExportSection();
  if ((Status = instantiate(ExportSec)) != ErrCode::Success) {
    return Status;
  }

  /// Instantiate StartSection (StartSec)
  AST::StartSection *StartSec = Mod->getStartSection();
  if (StartSec != nullptr) {
    /// Get the module instance from ID.
    ModInst->setStartIdx(StartSec->getContent());
  }

  /// In e-wasm, the start section will always be "main" function.
  /// Therefore, the start function index will be find in export section.

  return Status;
}

} // namespace Executor
} // namespace SSVM