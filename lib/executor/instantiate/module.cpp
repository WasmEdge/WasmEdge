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
Expect<std::unique_ptr<Runtime::Instance::ModuleInstance>>
Executor::instantiate(Runtime::StoreManager &StoreMgr, const AST::Module &Mod,
                      std::optional<std::string_view> Name) {
  // Check the module is validated.
  if (unlikely(!Mod.getIsValidated())) {
    spdlog::error(ErrCode::Value::NotValidated);
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    return Unexpect(ErrCode::Value::NotValidated);
  }

  // Create the stack manager.
  Runtime::StackManager StackMgr;

  // Check is module name duplicated when trying to registration.
  if (Name.has_value()) {
    const auto *FindModInst = StoreMgr.findModule(Name.value());
    if (FindModInst != nullptr) {
      spdlog::error(ErrCode::Value::ModuleNameConflict);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
      return Unexpect(ErrCode::Value::ModuleNameConflict);
    }
  }

  // Insert the module instance to store manager and retrieve instance.
  std::unique_ptr<Runtime::Instance::ModuleInstance> ModInst;
  if (Name.has_value()) {
    ModInst = std::make_unique<Runtime::Instance::ModuleInstance>(Name.value());
  } else {
    ModInst = std::make_unique<Runtime::Instance::ModuleInstance>("");
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
    StoreMgr.recycleModule(std::move(ModInst));
    return Unexpect(Res);
  }

  // Instantiate Functions in module. (FunctionSec, CodeSec)
  const AST::FunctionSection &FuncSec = Mod.getFunctionSection();
  const AST::CodeSection &CodeSec = Mod.getCodeSection();
  // This function will always success.
  instantiate(*ModInst, FuncSec, CodeSec);

  // Instantiate TableSection (TableSec)
  const AST::TableSection &TabSec = Mod.getTableSection();
  // This function will always success.
  instantiate(*ModInst, TabSec);

  // Instantiate MemorySection (MemorySec)
  const AST::MemorySection &MemSec = Mod.getMemorySection();
  // This function will always success.
  instantiate(*ModInst, MemSec);

  // Add a temp module to Store with only imported globals for initialization.
  std::unique_ptr<Runtime::Instance::ModuleInstance> TmpModInst =
      std::make_unique<Runtime::Instance::ModuleInstance>("");
  for (uint32_t I = 0; I < ModInst->getGlobalImportNum(); ++I) {
    TmpModInst->importGlobal(*(ModInst->getGlobal(I)));
  }
  for (uint32_t I = 0; I < ModInst->getFuncNum(); ++I) {
    TmpModInst->importFunction(*(ModInst->getFunc(I)));
  }

  // Push a new frame {TmpModInst:{globaddrs}, locals:none}
  StackMgr.pushFrame(TmpModInst.get(), AST::InstrView::iterator(), 0, 0);

  // Instantiate GlobalSection (GlobalSec)
  const AST::GlobalSection &GlobSec = Mod.getGlobalSection();
  if (auto Res = instantiate(StackMgr, *ModInst, GlobSec); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sec_Global));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    StoreMgr.recycleModule(std::move(ModInst));
    return Unexpect(Res);
  }

  // Pop frame with the temp. module.
  StackMgr.popFrame();

  // Instantiate ExportSection (ExportSec)
  const AST::ExportSection &ExportSec = Mod.getExportSection();
  // This function will always success.
  instantiate(*ModInst, ExportSec);

  // Push a new frame {ModInst, locals:none}
  StackMgr.pushFrame(ModInst.get(), AST::InstrView::iterator(), 0, 0);

  // Instantiate ElementSection (ElemSec)
  const AST::ElementSection &ElemSec = Mod.getElementSection();
  if (auto Res = instantiate(StackMgr, *ModInst, ElemSec); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sec_Element));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    StoreMgr.recycleModule(std::move(ModInst));
    return Unexpect(Res);
  }

  // Instantiate DataSection (DataSec)
  const AST::DataSection &DataSec = Mod.getDataSection();
  if (auto Res = instantiate(StackMgr, *ModInst, DataSec); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sec_Data));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    StoreMgr.recycleModule(std::move(ModInst));
    return Unexpect(Res);
  }

  // Initialize table instances
  if (auto Res = initTable(StackMgr, ElemSec); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sec_Element));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    StoreMgr.recycleModule(std::move(ModInst));
    return Unexpect(Res);
  }

  // Initialize memory instances
  if (auto Res = initMemory(StackMgr, DataSec); !Res) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Sec_Data));
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
    StoreMgr.recycleModule(std::move(ModInst));
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
    if (auto Res = runFunction(StackMgr, *FuncInst, {}); unlikely(!Res)) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Module));
      StoreMgr.recycleModule(std::move(ModInst));
      return Unexpect(Res);
    }
  }

  // Pop Frame.
  StackMgr.popFrame();

  // For the named modules, register it into the store.
  if (Name.has_value()) {
    StoreMgr.registerModule(ModInst.get());
  }

  return ModInst;
}

} // namespace Executor
} // namespace WasmEdge
