// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "executor/executor.h"

#include "common/errinfo.h"
#include "common/log.h"

#include <cstdint>
#include <string_view>

namespace WasmEdge {
namespace Executor {

// Instantiate module instance. See "include/executor/Executor.h".
Expect<void> Executor::instantiate(Runtime::StoreManager &StoreMgr,
                                   const AST::Module &Mod,
                                   std::string_view Name) {
  // Reset store manager and stack manager.
  StoreMgr.reset();
  Runtime::StackManager StackMgr;

  // Check is module name duplicated.
  if (auto Res = StoreMgr.findModule(Name)) {
    spdlog::error(ErrCode::ModuleNameConflict);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(ErrCode::ModuleNameConflict);
  }

  // Insert the module instance to store manager and retrieve instance.
  Runtime::Instance::ModuleInstance *ModInst = nullptr;
  if (InsMode == InstantiateMode::Instantiate) {
    ModInst = StoreMgr.pushModule(Name);
  } else {
    ModInst = StoreMgr.importModule(Name);
  }

  // Instantiate Function Types in Module Instance. (TypeSec)
  for (auto &FuncType : Mod.getTypeSection().getContent()) {
    // Copy param and return lists to module instance.
    ModInst->addFuncType(FuncType);
  }

  // Instantiate ImportSection and do import matching. (ImportSec)
  const AST::ImportSection &ImportSec = Mod.getImportSection();
  if (auto Res = instantiate(StoreMgr, *ModInst, ImportSec); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sec_Import));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(Res);
  }

  // Instantiate Functions in module. (FunctionSec, CodeSec)
  const AST::FunctionSection &FuncSec = Mod.getFunctionSection();
  const AST::CodeSection &CodeSec = Mod.getCodeSection();
  if (auto Res = instantiate(StoreMgr, *ModInst, FuncSec, CodeSec); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sec_Function));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(Res);
  }

  // Instantiate TableSection (TableSec)
  const AST::TableSection &TabSec = Mod.getTableSection();
  if (auto Res = instantiate(StoreMgr, *ModInst, TabSec); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sec_Table));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(Res);
  }

  // Instantiate MemorySection (MemorySec)
  const AST::MemorySection &MemSec = Mod.getMemorySection();
  if (auto Res = instantiate(StoreMgr, *ModInst, MemSec); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sec_Memory));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(Res);
  }

  // Add a temp module to Store with only imported globals for initialization.
  Runtime::Instance::ModuleInstance *TmpModInst = StoreMgr.pushModule("");
  for (uint32_t I = 0; I < ModInst->getGlobalImportNum(); ++I) {
    TmpModInst->importGlobal(*(ModInst->getGlobal(I)));
  }
  for (uint32_t I = 0; I < ModInst->getFuncNum(); ++I) {
    TmpModInst->importFunction(*(ModInst->getFunc(I)));
  }

  // Push a new frame {TmpModInst:{globaddrs}, locals:none}
  StackMgr.pushFrame(TmpModInst, AST::InstrView::iterator(), 0, 0);

  // Instantiate GlobalSection (GlobalSec)
  const AST::GlobalSection &GlobSec = Mod.getGlobalSection();
  if (auto Res = instantiate(StoreMgr, StackMgr, *ModInst, GlobSec); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sec_Global));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(Res);
  }

  // Pop frame with temp. module.
  StackMgr.popFrame();

  // Pop the added temp. module.
  StoreMgr.popModule();

  // Instantiate ExportSection (ExportSec)
  const AST::ExportSection &ExportSec = Mod.getExportSection();
  if (auto Res = instantiate(*ModInst, ExportSec); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sec_Export));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(Res);
  }

  // Push a new frame {ModInst, locals:none}
  StackMgr.pushFrame(ModInst, AST::InstrView::iterator(), 0, 0);

  // Instantiate ElementSection (ElemSec)
  const AST::ElementSection &ElemSec = Mod.getElementSection();
  if (auto Res = instantiate(StoreMgr, StackMgr, *ModInst, ElemSec); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sec_Element));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(Res);
  }

  // Instantiate DataSection (DataSec)
  const AST::DataSection &DataSec = Mod.getDataSection();
  if (auto Res = instantiate(StoreMgr, StackMgr, *ModInst, DataSec); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sec_Data));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(Res);
  }

  // Initialize table instances
  if (auto Res = initTable(StackMgr, *ModInst, ElemSec); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sec_Element));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(Res);
  }

  // Initialize memory instances
  if (auto Res = initMemory(StackMgr, *ModInst, DataSec); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sec_Data));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(Res);
  }

  // Instantiate StartSection (StartSec)
  const AST::StartSection &StartSec = Mod.getStartSection();
  if (StartSec.getContent()) {
    // Get the module instance from ID.
    ModInst->setStartIdx(*StartSec.getContent());

    // Get function instance.
    const auto *FuncInst = ModInst->getStartFunc();

    // Execute instruction.
    if (auto Res = runFunction(StoreMgr, StackMgr, *FuncInst, {});
        unlikely(!Res)) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
      return Unexpect(Res);
    }
  }

  // Pop Frame.
  StackMgr.popFrame();

  return {};
}

} // namespace Executor
} // namespace WasmEdge
