#include "ast/module.h"
#include "executor/moduleinst.h"

namespace AST {

/// Instantiate Module Instance. See "include/ast/module.h".
Executor::ErrCode Module::instantiate(StoreMgr &Mgr) {
  Executor::ErrCode Status = Executor::ErrCode::Success;
  auto ModInst = std::make_unique<ModuleInstance>();
  unsigned int ModInstId = 0;

  /// Insert the module instance to store manager.
  if ((Status = Mgr.insertModuleInst(ModInst, ModInstId)) !=
      Executor::ErrCode::Success)
    return Status;

  /// Instantiate ImportSection and do import matching. (ImportSec)
  if (ImportSec != nullptr) {
    if ((Status = ImportSec->instantiate(Mgr, ModInstId)) !=
        Executor::ErrCode::Success)
      return Status;
    ImportSec.reset();
  }

  /// Instantiate Function Types in Module Instance. (TypeSec)
  if (TypeSec != nullptr) {
    if ((Status = TypeSec->instantiate(Mgr, ModInstId)) !=
        Executor::ErrCode::Success)
      return Status;
    TypeSec.reset();
  }

  /// Instantiate Functions in module. (FuncionSec, CodeSec)
  if (CodeSec != nullptr && FunctionSec != nullptr) {
    if ((Status = CodeSec->instantiate(Mgr, ModInstId, FunctionSec)) !=
        Executor::ErrCode::Success)
      return Status;
    CodeSec.reset();
    FunctionSec.reset();
  }

  /// Instantiate GlobalSection (GlobalSec)
  if (GlobalSec != nullptr) {
    if ((Status = GlobalSec->instantiate(Mgr, ModInstId)) !=
        Executor::ErrCode::Success)
      return Status;
    GlobalSec.reset();
    /// TODO: Initialize the globals
    /// Push Frame {NewModInst:{globaddrs}, local:none}
    ///   evaluate instrs in global instances
    /// Pop Frame
  }

  /// TODO: Initializa the tables and memories
  /// Push Frame {ModInst, local:none}

  /// Instantiate TableSection (TableSec)
  if (TableSec != nullptr) {
    if ((Status = TableSec->instantiate(Mgr, ModInstId)) !=
        Executor::ErrCode::Success)
      return Status;
    TableSec.reset();
  }

  /// Instantiate MemorySection (MemorySec)
  if (MemorySec != nullptr) {
    if ((Status = MemorySec->instantiate(Mgr, ModInstId)) !=
        Executor::ErrCode::Success)
      return Status;
    MemorySec.reset();
  }

  /// Instantiate ElementSection TODO
  ///   evaluate instrs in element segments

  /// Instantiate DataSection TODO
  ///   evaluate instrs in data segments

  /// Pop Frame.
  /// Replace data in table instance.
  /// Replace data in memory instance.

  /// Instantiate ExportSection (ExportSec)
  if (ExportSec != nullptr) {
    if ((Status = ExportSec->instantiate(Mgr, ModInstId)) !=
        Executor::ErrCode::Success)
      return Status;
    ExportSec.reset();
  }
  /// Instantiate StartSection (StartSec)
  /// In e-wasm, the start section will always be "main" function.
  /// Therefore, the start function index will be find in export section.

  return Status;
}

} // namespace AST