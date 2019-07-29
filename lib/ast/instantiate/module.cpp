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

  /// Instantiate TableSection TODO
  /// Instantiate MemorySection TODO
  /// Instantiate GlobalSection TODO
  /// Instantiate ExportSection TODO
  /// Instantiate StartSection TODO
  /// Instantiate ElementSection TODO
  /// Instantiate DataSection TODO

  return Mgr.insertModuleInst(Id, std::move(ModInst));
}

} // namespace AST