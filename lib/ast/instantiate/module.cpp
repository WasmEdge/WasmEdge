#include "ast/module.h"
#include "executor/moduleinst.h"

namespace AST {

/// Instantiate Module Instance. See "include/ast/module.h".
Executor::ErrCode Module::instantiate(StoreMgr &Store, StackMgr &Stack) {
  Executor::ErrCode Status = Executor::ErrCode::Success;
  auto ModInst = std::make_unique<ModuleInstance>();
  unsigned int ModInstId = 0;

  /// Insert the module instance to store manager.
  if ((Status = Store.insertModuleInst(ModInst, ModInstId)) !=
      Executor::ErrCode::Success)
    return Status;

  /// Instantiate ImportSection and do import matching. (ImportSec)
  if (ImportSec != nullptr) {
    if ((Status = ImportSec->instantiate(Store, ModInstId)) !=
        Executor::ErrCode::Success)
      return Status;
    ImportSec.reset();
  }

  /// Instantiate Function Types in Module Instance. (TypeSec)
  if (TypeSec != nullptr) {
    if ((Status = TypeSec->instantiate(Store, ModInstId)) !=
        Executor::ErrCode::Success)
      return Status;
    TypeSec.reset();
  }

  /// Instantiate Functions in module. (FuncionSec, CodeSec)
  if (CodeSec != nullptr && FunctionSec != nullptr) {
    if ((Status = CodeSec->instantiate(Store, ModInstId, FunctionSec)) !=
        Executor::ErrCode::Success)
      return Status;
    CodeSec.reset();
    FunctionSec.reset();
  }

  /// Instantiate GlobalSection (GlobalSec)
  if (GlobalSec != nullptr) {
    if ((Status = GlobalSec->instantiate(Store, Stack, ModInstId)) !=
        Executor::ErrCode::Success)
      return Status;
    GlobalSec.reset();
  }

  /// TODO: Initializa the tables and memories
  /// Push Frame {ModInst, local:none}

  /// Instantiate TableSection (TableSec)
  if (TableSec != nullptr) {
    if ((Status = TableSec->instantiate(Store, ModInstId)) !=
        Executor::ErrCode::Success)
      return Status;
    TableSec.reset();
  }

  /// Instantiate MemorySection (MemorySec)
  if (MemorySec != nullptr) {
    if ((Status = MemorySec->instantiate(Store, ModInstId)) !=
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
    if ((Status = ExportSec->instantiate(Store, ModInstId)) !=
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