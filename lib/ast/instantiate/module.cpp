#include "ast/module.h"
#include "executor/moduleinst.h"

namespace AST {

/// Instantiate Module Instance. See "include/ast/module.h".
Executor::ErrCode Module::instantiate(StoreMgr &Mgr, unsigned int Id) {
  Executor::ErrCode Status = Executor::ErrCode::Success;
  auto ModInst = std::make_unique<ModuleInstance>();

  /// Instantiate Function Types in Module Instance. (TypeSec)
  if (TypeSec != nullptr) {
    TypeSec->instantiate(Mgr, ModInst);
    TypeSec.reset();
  }

  /// Instantiate ImportSection TODO

  /// Instantiate Functions in module. (FuncionSec, CodeSec)
  if (CodeSec != nullptr && FunctionSec != nullptr) {
    CodeSec->instantiate(Mgr, ModInst, FunctionSec);
    CodeSec.reset();
    FunctionSec.reset();
  }

  /// Instantiate GlobalSection
  if (GlobalSec != nullptr) {
    GlobalSec->instantiate(Mgr, ModInst);
    GlobalSec.reset();
    /// TODO: Initialize the globals
    /// Push Frame {NewModInst:{globaddrs}, local:none}
    ///   evaluate instrs in global instances
    /// Pop Frame
  }

  /// TODO: Initializa the tables and memories
  /// Push Frame {ModInst, local:none}

  /// Instantiate TableSection
  if (TableSec != nullptr) {
    TableSec->instantiate(Mgr, ModInst);
    TableSec.reset();
  }

  /// Instantiate MemorySection
  if (MemorySec != nullptr) {
    MemorySec->instantiate(Mgr, ModInst);
    MemorySec.reset();
  }

  /// Instantiate ElementSection TODO
  ///   evaluate instrs in element segments

  /// Instantiate DataSection TODO
  ///   evaluate instrs in data segments

  /// Pop Frame.
  /// Replace data in table instance.
  /// Replace data in memory instance.

  /// Instantiate ExportSection TODO
  /// Instantiate StartSection TODO

  return Mgr.insertModuleInst(Id, std::move(ModInst));
}

} // namespace AST