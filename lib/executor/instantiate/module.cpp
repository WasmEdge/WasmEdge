// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/executor.h"

#include "common/errinfo.h"
#include "common/spdlog.h"

#include <cstdint>
#include <string_view>
#include <utility>

namespace WasmEdge {
namespace Executor {

// Instantiate core module sections. See "include/executor/executor.h".
Expect<void>
Executor::instantiateModule(const CoreModuleFinder &Finder,
                            Runtime::Instance::ModuleInstance &ModInst,
                            const AST::Module &Mod) {
  // Create the stack manager.
  Runtime::StackManager StackMgr;

  // Instantiate Function Types in Module Instance. (TypeSec)
  for (auto &SubType : Mod.getTypeSection().getContent()) {
    // Copy defined types to module instance.
    ModInst.addDefinedType(SubType);
  }

  auto ReportError = [](ASTNodeAttr Attr) {
    return [Attr](auto E) {
      spdlog::error(ErrInfo::InfoAST(Attr));
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
      return E;
    };
  };

  // Instantiate ImportSection and do import matching. (ImportSec)
  const AST::ImportSection &ImportSec = Mod.getImportSection();
  EXPECTED_TRY(instantiate(Finder, ModInst, ImportSec)
                   .map_error(ReportError(ASTNodeAttr::Sec_Import)));

  // Instantiate Functions in module. (FunctionSec, CodeSec)
  const AST::FunctionSection &FuncSec = Mod.getFunctionSection();
  const AST::CodeSection &CodeSec = Mod.getCodeSection();
  // This function will always success.
  instantiate(ModInst, FuncSec, CodeSec);

  // Instantiate MemorySection (MemorySec)
  const AST::MemorySection &MemSec = Mod.getMemorySection();
  // This function will always success.
  instantiate(ModInst, MemSec);

  // Instantiate TagSection (TagSec)
  const AST::TagSection &TagSec = Mod.getTagSection();
  // This function will always success.
  instantiate(ModInst, TagSec);

  // Push a new frame {ModInst, locals:none}
  StackMgr.pushFrame(&ModInst, AST::InstrView::iterator(), 0, 0);

  // Instantiate GlobalSection (GlobalSec)
  const AST::GlobalSection &GlobSec = Mod.getGlobalSection();
  EXPECTED_TRY(instantiate(StackMgr, ModInst, GlobSec)
                   .map_error(ReportError(ASTNodeAttr::Sec_Global)));

  // Instantiate TableSection (TableSec)
  const AST::TableSection &TabSec = Mod.getTableSection();
  EXPECTED_TRY(instantiate(StackMgr, ModInst, TabSec)
                   .map_error(ReportError(ASTNodeAttr::Sec_Table)));

  // Instantiate ExportSection (ExportSec)
  const AST::ExportSection &ExportSec = Mod.getExportSection();
  // This function will always success.
  instantiate(ModInst, ExportSec);

  // Instantiate ElementSection (ElemSec)
  const AST::ElementSection &ElemSec = Mod.getElementSection();
  EXPECTED_TRY(instantiate(StackMgr, ModInst, ElemSec)
                   .map_error(ReportError(ASTNodeAttr::Sec_Element)));

  // Instantiate DataSection (DataSec)
  const AST::DataSection &DataSec = Mod.getDataSection();
  EXPECTED_TRY(instantiate(StackMgr, ModInst, DataSec)
                   .map_error(ReportError(ASTNodeAttr::Sec_Data)));

  // Initialize table instances
  EXPECTED_TRY(initTable(StackMgr, ElemSec)
                   .map_error(ReportError(ASTNodeAttr::Sec_Element)));

  // Initialize memory instances
  EXPECTED_TRY(initMemory(StackMgr, DataSec)
                   .map_error(ReportError(ASTNodeAttr::Sec_Data)));

  ModInst.buildContext();

  // Instantiate StartSection (StartSec)
  const AST::StartSection &StartSec = Mod.getStartSection();
  if (StartSec.getContent()) {
    // Get the module instance from ID.
    ModInst.setStartIdx(*StartSec.getContent());

    // Get function instance.
    const auto *FuncInst = ModInst.getStartFunc();

    // Execute instruction.
    EXPECTED_TRY(runFunction(StackMgr, *FuncInst, {}).map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
      return E;
    }));
  }

  // Pop Frame.
  StackMgr.popFrame();

  // Instantiation done; finalize so the executor reads instances lock-free.
  ModInst.finalizeInstantiation();

  return {};
}

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

  // Imports resolve through the store, and a found instance becomes a
  // dependency of this one.
  CoreModuleFinder Finder = [&StoreMgr, &ModInst](std::string_view ModName)
      -> const WasmEdge::Runtime::Instance::ModuleInstance * {
    using WasmEdge::Runtime::Instance::ModuleInstance;
    return StoreMgr.withModuleLocked(
        ModName,
        [&ModInst](const ModuleInstance *Found) -> const ModuleInstance * {
          if (Found) {
            ModInst->addDependency(*const_cast<ModuleInstance *>(Found));
          }
          return Found;
        });
  };

  // According to the current spec, the instances of a failed instantiation
  // remain referenceable, so keep the instance in the store.
  EXPECTED_TRY(instantiateModule(Finder, *ModInst, Mod)
                   .map_error([&StoreMgr, &ModInst](auto E) {
                     StoreMgr.recycleModule(std::move(ModInst));
                     return E;
                   }));

  // For a named module, register it in the store.
  if (Name.has_value()) {
    StoreMgr.registerModule(ModInst.get());
  }

  return ModInst;
}

} // namespace Executor
} // namespace WasmEdge
