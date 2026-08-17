// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/executor.h"

#include "common/errinfo.h"
#include "common/spdlog.h"

#include <cstdint>
#include <string_view>

namespace WasmEdge {
namespace Executor {

// Instantiate module instance. See "include/executor/Executor.h".
Expect<std::unique_ptr<Runtime::Instance::ModuleInstance>>
Executor::instantiate(Runtime::StoreManager &StoreMgr, const AST::Module &Mod,
                      std::optional<std::string_view> Name) {
  // Check that the module is validated.
  if (unlikely(!Mod.getIsValidated())) {
    spdlog::error(ErrCode::Value::NotValidated);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(ErrCode::Value::NotValidated);
  }

  // Operation lease across instantiation: it registers a value stack (the
  // start function may run guest code) and touches the allocator. Refused
  // once the controller is closing -- same boundary contract as
  // Executor::invoke.
  auto OpLease = getController().acquireLease();
  if (unlikely(!OpLease.valid())) {
    spdlog::error(ErrCode::Value::Interrupted);
    return Unexpect(ErrCode::Value::Interrupted);
  }

  // Create the stack manager.
  Runtime::StackManager StackMgr(getController());

  // Check whether the module name is duplicated during registration.
  if (Name.has_value()) {
    const auto *FindModInst = StoreMgr.findModule(Name.value());
    if (FindModInst != nullptr) {
      spdlog::error(ErrCode::Value::ModuleNameConflict);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
      return Unexpect(ErrCode::Value::ModuleNameConflict);
    }
  }

  // Insert the module instance into the store manager and retrieve it.
  std::unique_ptr<Runtime::Instance::ModuleInstance> ModInst;
  if (Name.has_value()) {
    ModInst = std::make_unique<Runtime::Instance::ModuleInstance>(Name.value());
  } else {
    ModInst = std::make_unique<Runtime::Instance::ModuleInstance>("");
  }
  // Runtime-instantiated modules are heap and torn down via terminate()
  // (VM cleanup / WasmEdge_ModuleInstanceDelete): an async ModulePin can
  // defer their deletion.
  ModInst->setDeferrableStorage();

  // Instantiate Function Types in Module Instance. (TypeSec)
  for (auto &SubType : Mod.getTypeSection().getContent()) {
    // Copy defined types to module instance.
    ModInst->addDefinedType(SubType);
  }

  auto ReportModuleError = [&StoreMgr, &ModInst](auto E) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    StoreMgr.recycleModule(std::move(ModInst));
    return E;
  };

  auto ReportError = [&StoreMgr, &ModInst](ASTNodeAttr Attr) {
    return [Attr, &StoreMgr, &ModInst](auto E) {
      spdlog::error(ErrInfo::InfoAST(Attr));
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
      StoreMgr.recycleModule(std::move(ModInst));
      return E;
    };
  };

  // Instantiate ImportSection and do import matching. (ImportSec)
  const AST::ImportSection &ImportSec = Mod.getImportSection();
  EXPECTED_TRY(instantiate(
                   [&StoreMgr, &ModInst](std::string_view ModName)
                       -> const WasmEdge::Runtime::Instance::ModuleInstance * {
                     using WasmEdge::Runtime::Instance::ModuleInstance;
                     return StoreMgr.withModuleLocked(
                         ModName,
                         [&ModInst](const ModuleInstance *Found)
                             -> const ModuleInstance * {
                           if (Found) {
                             ModInst->addDependency(
                                 *const_cast<ModuleInstance *>(Found));
                           }
                           return Found;
                         });
                   },
                   *ModInst, ImportSec)
                   .map_error(ReportError(ASTNodeAttr::Sec_Import)));

  // Instantiate Functions in module. (FunctionSec, CodeSec)
  const AST::FunctionSection &FuncSec = Mod.getFunctionSection();
  const AST::CodeSection &CodeSec = Mod.getCodeSection();
  // R7-M3: pass the module's GC-capability so a GC-enabled executor falls a
  // non-capable compiled module back to interpreter-mode functions. This can
  // fail when the fallback is impossible (AOT-stripped bodies), so propagate.
  EXPECTED_TRY(instantiate(*ModInst, FuncSec, CodeSec, Mod.getGCCompiled())
                   .map_error(ReportError(ASTNodeAttr::Sec_Function)));
  // R7-M3 backstop: carry the capability on the instance too. This executor
  // has already deopted if it needed to, but the instance can outlive that
  // decision -- registerModule or a direct invoke can hand these function
  // instances to a GC-enabled executor that never saw this module's AST.
  ModInst->setGCCompiled(Mod.getGCCompiled());

  // Instantiate MemorySection (MemorySec)
  const AST::MemorySection &MemSec = Mod.getMemorySection();
  // This function will always success.
  instantiate(*ModInst, MemSec);

  // Instantiate TagSection (TagSec)
  const AST::TagSection &TagSec = Mod.getTagSection();
  // This function will always success.
  instantiate(*ModInst, TagSec);

  // Push a new frame {ModInst, locals:none}
  StackMgr.pushFrame(ModInst.get(), AST::InstrView::iterator());

  // Instantiate GlobalSection (GlobalSec)
  const AST::GlobalSection &GlobSec = Mod.getGlobalSection();
  EXPECTED_TRY(instantiate(StackMgr, *ModInst, GlobSec)
                   .map_error(ReportError(ASTNodeAttr::Sec_Global)));

  // Instantiate TableSection (TableSec)
  const AST::TableSection &TabSec = Mod.getTableSection();
  EXPECTED_TRY(instantiate(StackMgr, *ModInst, TabSec)
                   .map_error(ReportError(ASTNodeAttr::Sec_Table)));

  // Instantiate ExportSection (ExportSec)
  const AST::ExportSection &ExportSec = Mod.getExportSection();
  // This function will always success.
  instantiate(*ModInst, ExportSec);

  // Instantiate ElementSection (ElemSec)
  const AST::ElementSection &ElemSec = Mod.getElementSection();
  EXPECTED_TRY(instantiate(StackMgr, *ModInst, ElemSec)
                   .map_error(ReportError(ASTNodeAttr::Sec_Element)));

  // Instantiate DataSection (DataSec)
  const AST::DataSection &DataSec = Mod.getDataSection();
  EXPECTED_TRY(instantiate(StackMgr, *ModInst, DataSec)
                   .map_error(ReportError(ASTNodeAttr::Sec_Data)));

  // Initialize table instances
  EXPECTED_TRY(initTable(StackMgr, ElemSec)
                   .map_error(ReportError(ASTNodeAttr::Sec_Element)));

  // Initialize memory instances
  EXPECTED_TRY(initMemory(StackMgr, DataSec)
                   .map_error(ReportError(ASTNodeAttr::Sec_Data)));

  ModInst->buildContext();

  // Instantiate StartSection (StartSec)
  const AST::StartSection &StartSec = Mod.getStartSection();
  if (StartSec.getContent()) {
    // Get the module instance from ID.
    ModInst->setStartIdx(*StartSec.getContent());

    // Get function instance.
    const auto *FuncInst = ModInst->getStartFunc();

    // Execute instruction.
    EXPECTED_TRY(
        runFunction(StackMgr, *FuncInst, {}).map_error(ReportModuleError));
  }

  // Pop Frame.
  StackMgr.popFrame();

  // Instantiation done; finalize so the executor reads instances lock-free.
  ModInst->finalizeInstantiation();

  // For a named module, register it in the store.
  if (Name.has_value()) {
    StoreMgr.registerModule(ModInst.get());
  }

  return ModInst;
}

} // namespace Executor
} // namespace WasmEdge
