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
  auto NewModInst = std::make_unique<Instance::ModuleInstance>();

  /// Insert the module instance to store manager and retieve instance.
  unsigned int ModInstAddr;
  if (ErrCode Status = StoreMgr.insertModuleInst(NewModInst, ModInstAddr);
      Status != ErrCode::Success) {
    return Status;
  }
  if (ErrCode Status = StoreMgr.getModule(ModInstAddr, ModInst);
      Status != ErrCode::Success) {
    return Status;
  }

  /// Instantiate ImportSection and do import matching. (ImportSec)
  AST::ImportSection *ImportSec = Mod->getImportSection();
  if (ErrCode Status = instantiate(ImportSec); Status != ErrCode::Success) {
    return Status;
  }

  /// Instantiate Function Types in Module Instance. (TypeSec)
  AST::TypeSection *TypeSec = Mod->getTypeSection();
  if (ErrCode Status = instantiate(TypeSec); Status != ErrCode::Success) {
    return Status;
  }

  /// Instantiate Functions in module. (FuncionSec, CodeSec)
  AST::FunctionSection *FuncSec = Mod->getFunctionSection();
  AST::CodeSection *CodeSec = Mod->getCodeSection();
  if (ErrCode Status = instantiate(FuncSec, CodeSec);
      Status != ErrCode::Success) {
    return Status;
  }

  /// Instantiate GlobalSection (GlobalSec)
  AST::GlobalSection *GlobSec = Mod->getGlobalSection();
  if (ErrCode Status = instantiate(GlobSec); Status != ErrCode::Success) {
    return Status;
  }

  /// Initializa the tables and memories
  /// Make a new frame {ModInst, locals:none} and push
  StackMgr.pushFrame(ModInst->Addr, /// Module address
                     0              /// Arity
  );

  /// Instantiate TableSection (TableSec, ElemSec)
  AST::TableSection *TabSec = Mod->getTableSection();
  AST::ElementSection *ElemSec = Mod->getElementSection();
  if (ErrCode Status = instantiate(TabSec, ElemSec);
      Status != ErrCode::Success) {
    return Status;
  }

  /// Instantiate MemorySection (MemorySec, DataSec)
  AST::MemorySection *MemSec = Mod->getMemorySection();
  AST::DataSection *DataSec = Mod->getDataSection();
  if (ErrCode Status = instantiate(MemSec, DataSec);
      Status != ErrCode::Success) {
    return Status;
  }

  /// Pop Frame.
  StackMgr.popFrame();

  /// Instantiate ExportSection (ExportSec)
  AST::ExportSection *ExportSec = Mod->getExportSection();
  if (ErrCode Status = instantiate(ExportSec); Status != ErrCode::Success) {
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

  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
