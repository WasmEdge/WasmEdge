// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/executor.h"

#include "common/errinfo.h"
#include "common/spdlog.h"

#include <cstdint>
#include <string_view>
#include <utility>

using namespace std::literals;

namespace WasmEdge {
namespace Executor {

Expect<void>
Executor::instantiate(Runtime::Instance::ComponentInstance &CompInst,
                      const AST::Component::CoreModuleSection &CoreModSec) {
  CompInst.addModule(CoreModSec.getContent());
  return {};
}

Expect<std::unique_ptr<Runtime::Instance::ModuleInstance>>
Executor::instantiate(Runtime::Instance::ComponentImportManager &ImportMgr,
                      const AST::Module &Mod) {
  // Create the stack manager.
  Runtime::StackManager StackMgr(Allocator);

  // Create the module instance.
  std::unique_ptr<Runtime::Instance::ModuleInstance> ModInst =
      std::make_unique<Runtime::Instance::ModuleInstance>("");

  // Instantiate Function Types in Module Instance. (TypeSec)
  for (auto &SubType : Mod.getTypeSection().getContent()) {
    // Copy defined types to module instance.
    ModInst->addDefinedType(SubType);
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
  EXPECTED_TRY(instantiate(
                   [&ImportMgr](std::string_view Name)
                       -> const WasmEdge::Runtime::Instance::ModuleInstance * {
                     return ImportMgr.findCoreModuleInstance(Name);
                   },
                   *ModInst, ImportSec)
                   .map_error(ReportError(ASTNodeAttr::Sec_Import)));

  // Instantiate Functions in module. (FunctionSec, CodeSec)
  const AST::FunctionSection &FuncSec = Mod.getFunctionSection();
  const AST::CodeSection &CodeSec = Mod.getCodeSection();
  // This function will always success.
  instantiate(*ModInst, FuncSec, CodeSec);

  // Instantiate MemorySection (MemorySec)
  const AST::MemorySection &MemSec = Mod.getMemorySection();
  // This function will always success.
  instantiate(*ModInst, MemSec);

  // Instantiate TagSection (TagSec)
  const AST::TagSection &TagSec = Mod.getTagSection();
  // This function will always success.
  instantiate(*ModInst, TagSec);

  // Push a new frame {ModInst, locals:none}
  StackMgr.pushFrame(ModInst.get(), AST::InstrView::iterator(), 0, 0);

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

  // Instantiate StartSection (StartSec)
  const AST::StartSection &StartSec = Mod.getStartSection();
  if (StartSec.getContent()) {
    // Get the module instance from ID.
    ModInst->setStartIdx(*StartSec.getContent());

    // Get function instance.
    const auto *FuncInst = ModInst->getStartFunc();

    // Execute instruction.
    EXPECTED_TRY(runFunction(StackMgr, *FuncInst, {}).map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
      return E;
    }));
  }

  // Pop Frame.
  StackMgr.popFrame();

  return ModInst;
}

} // namespace Executor
} // namespace WasmEdge
