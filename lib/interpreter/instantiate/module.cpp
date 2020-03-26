// SPDX-License-Identifier: Apache-2.0
#include "common/ast/module.h"
#include "common/ast/section.h"
#include "runtime/instance/module.h"
#include "interpreter/interpreter.h"

namespace SSVM {
namespace Interpreter {

/// Instantiate module instance. See "include/executor/Interpreter.h".
Expect<void> Interpreter::instantiate(Runtime::StoreManager &StoreMgr,
                                      const AST::Module &Mod,
                                      const std::string &Name) {
  /// Check is module name duplicated.
  if (auto Res = StoreMgr.findModule(Name)) {
    return Unexpect(ErrCode::ModuleNameConflict);
  }
  auto NewModInst = std::make_unique<Runtime::Instance::ModuleInstance>(Name);

  /// Insert the module instance to store manager and retieve instance.
  uint32_t ModInstAddr;
  if (InsMode == InstantiateMode::Instantiate) {
    ModInstAddr = StoreMgr.pushModule(NewModInst);
  } else {
    ModInstAddr = StoreMgr.importModule(NewModInst);
  }
  auto *ModInst = *StoreMgr.getModule(ModInstAddr);

  /// Instantiate Function Types in Module Instance. (TypeSec)
  const AST::TypeSection *TypeSec = Mod.getTypeSection();
  if (TypeSec != nullptr) {
    auto &FuncTypes = TypeSec->getContent();
    for (auto &FuncType : FuncTypes) {
      /// Copy param and return lists to module instance.
      ModInst->addFuncType(FuncType->getParamTypes(),
                           FuncType->getReturnTypes());
    }
  }

  /// Instantiate ImportSection and do import matching. (ImportSec)
  const AST::ImportSection *ImportSec = Mod.getImportSection();
  if (ImportSec != nullptr) {
    if (auto Res = instantiate(StoreMgr, *ModInst, *ImportSec); !Res) {
      return Unexpect(Res);
    }
  }

  /// Instantiate Functions in module. (FuncionSec, CodeSec)
  const AST::FunctionSection *FuncSec = Mod.getFunctionSection();
  const AST::CodeSection *CodeSec = Mod.getCodeSection();
  if (FuncSec != nullptr && CodeSec != nullptr) {
    if (auto Res = instantiate(StoreMgr, *ModInst, *FuncSec, *CodeSec); !Res) {
      return Unexpect(Res);
    }
  }

  /// Instantiate GlobalSection (GlobalSec)
  const AST::GlobalSection *GlobSec = Mod.getGlobalSection();
  if (GlobSec != nullptr) {
    if (auto Res = instantiate(StoreMgr, *ModInst, *GlobSec); !Res) {
      return Unexpect(Res);
    }
  }

  /// Instantiate TableSection (TableSec)
  const AST::TableSection *TabSec = Mod.getTableSection();
  if (TabSec != nullptr) {
    if (auto Res = instantiate(StoreMgr, *ModInst, *TabSec); !Res) {
      return Unexpect(Res);
    }
  }

  /// Instantiate MemorySection (MemorySec)
  const AST::MemorySection *MemSec = Mod.getMemorySection();
  if (MemSec != nullptr) {
    if (auto Res = instantiate(StoreMgr, *ModInst, *MemSec); !Res) {
      return Unexpect(Res);
    }
  }

  /// Initializa the tables and memories
  /// Make a new frame {ModInst, locals:none} and push
  StackMgr.pushFrame(ModInst->Addr, /// Module address
                     0,             /// Arity
                     0              /// Coarity
  );

  /// Instantiate initialization of table instances (ElemSec)
  const AST::ElementSection *ElemSec = Mod.getElementSection();
  if (ElemSec != nullptr) {
    if (auto Res = instantiate(StoreMgr, *ModInst, *ElemSec); !Res) {
      return Unexpect(Res);
    }
  }

  /// Instantiate initialization of memory instances (DataSec)
  const AST::DataSection *DataSec = Mod.getDataSection();
  if (DataSec != nullptr) {
    if (auto Res = instantiate(StoreMgr, *ModInst, *DataSec); !Res) {
      return Unexpect(Res);
    }
  }

  /// Pop Frame.
  StackMgr.popFrame();

  /// Instantiate ExportSection (ExportSec)
  const AST::ExportSection *ExportSec = Mod.getExportSection();
  if (ExportSec != nullptr) {
    if (auto Res = instantiate(StoreMgr, *ModInst, *ExportSec); !Res) {
      return Unexpect(Res);
    }
  }

  /// Instantiate StartSection (StartSec)
  const AST::StartSection *StartSec = Mod.getStartSection();
  if (StartSec != nullptr) {
    /// Get the module instance from ID.
    ModInst->setStartIdx(StartSec->getContent());

    /// Get function instance.
    const uint32_t Addr = *ModInst->getStartAddr();
    const auto *FuncInst = *StoreMgr.getFunction(Addr);

    /// Call runFunction.
    return runFunction(StoreMgr, *FuncInst);
  }
  return {};
}

} // namespace Interpreter
} // namespace SSVM
