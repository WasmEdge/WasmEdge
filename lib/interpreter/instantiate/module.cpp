// SPDX-License-Identifier: Apache-2.0
#include "ast/module.h"
#include "ast/section.h"
#include "common/log.h"
#include "interpreter/interpreter.h"
#include "runtime/instance/module.h"

namespace SSVM {
namespace Interpreter {

/// Instantiate module instance. See "include/executor/Interpreter.h".
Expect<void> Interpreter::instantiate(Runtime::StoreManager &StoreMgr,
                                      const AST::Module &Mod,
                                      std::string_view Name) {
  /// Reset store manager and stack manager.
  StoreMgr.reset();
  StackMgr.reset();

  /// Check is module name duplicated.
  if (auto Res = StoreMgr.findModule(Name)) {
    LOG(ERROR) << ErrCode::ModuleNameConflict;
    LOG(ERROR) << ErrInfo::InfoAST(Mod.NodeAttr);
    return Unexpect(ErrCode::ModuleNameConflict);
  }

  /// Insert the module instance to store manager and retrieve instance.
  uint32_t ModInstAddr;
  if (InsMode == InstantiateMode::Instantiate) {
    ModInstAddr = StoreMgr.pushModule(Name);
  } else {
    ModInstAddr = StoreMgr.importModule(Name);
  }
  auto *ModInst = *StoreMgr.getModule(ModInstAddr);

  /// Instantiate Function Types in Module Instance. (TypeSec)
  for (auto &FuncType : Mod.getTypeSection().getContent()) {
    /// Copy param and return lists to module instance.
    ModInst->addFuncType(FuncType.getParamTypes(), FuncType.getReturnTypes(),
                         FuncType.getSymbol());
  }

  /// Instantiate ImportSection and do import matching. (ImportSec)
  const AST::ImportSection &ImportSec = Mod.getImportSection();
  if (auto Res = instantiate(StoreMgr, *ModInst, ImportSec); !Res) {
    LOG(ERROR) << ErrInfo::InfoAST(ImportSec.NodeAttr);
    LOG(ERROR) << ErrInfo::InfoAST(Mod.NodeAttr);
    return Unexpect(Res);
  }

  /// Instantiate Functions in module. (FunctionSec, CodeSec)
  const AST::FunctionSection &FuncSec = Mod.getFunctionSection();
  const AST::CodeSection &CodeSec = Mod.getCodeSection();
  if (auto Res = instantiate(StoreMgr, *ModInst, FuncSec, CodeSec); !Res) {
    LOG(ERROR) << ErrInfo::InfoAST(FuncSec.NodeAttr);
    LOG(ERROR) << ErrInfo::InfoAST(Mod.NodeAttr);
    return Unexpect(Res);
  }

  /// Instantiate TableSection (TableSec)
  const AST::TableSection &TabSec = Mod.getTableSection();
  if (auto Res = instantiate(StoreMgr, *ModInst, TabSec); !Res) {
    LOG(ERROR) << ErrInfo::InfoAST(TabSec.NodeAttr);
    LOG(ERROR) << ErrInfo::InfoAST(Mod.NodeAttr);
    return Unexpect(Res);
  }

  /// Instantiate MemorySection (MemorySec)
  const AST::MemorySection &MemSec = Mod.getMemorySection();
  if (auto Res = instantiate(StoreMgr, *ModInst, MemSec); !Res) {
    LOG(ERROR) << ErrInfo::InfoAST(MemSec.NodeAttr);
    LOG(ERROR) << ErrInfo::InfoAST(Mod.NodeAttr);
    return Unexpect(Res);
  }

  /// Add a temp module to Store with only imported globals for initialization.
  uint32_t TmpModInstAddr = StoreMgr.pushModule("");
  ModInst = *StoreMgr.getModule(ModInstAddr);
  auto *TmpModInst = *StoreMgr.getModule(TmpModInstAddr);
  for (uint32_t I = 0; I < ModInst->getGlobalImportNum(); ++I) {
    TmpModInst->importGlobal(*(ModInst->getGlobalAddr(I)));
  }
  for (uint32_t I = 0; I < ModInst->getFuncNum(); ++I) {
    TmpModInst->importFunction(*(ModInst->getFuncAddr(I)));
  }

  /// Push a new frame {TmpModInst:{globaddrs}, locals:none}
  StackMgr.pushFrame(TmpModInstAddr, 0, 0);

  /// Instantiate GlobalSection (GlobalSec)
  const AST::GlobalSection &GlobSec = Mod.getGlobalSection();
  if (auto Res = instantiate(StoreMgr, *ModInst, GlobSec); !Res) {
    LOG(ERROR) << ErrInfo::InfoAST(GlobSec.NodeAttr);
    LOG(ERROR) << ErrInfo::InfoAST(Mod.NodeAttr);
    return Unexpect(Res);
  }

  /// Pop frame with temp. module.
  StackMgr.popFrame();

  /// Pop the added temp. module.
  StoreMgr.popModule();
  ModInst = *StoreMgr.getModule(ModInstAddr);

  /// Instantiate ExportSection (ExportSec)
  const AST::ExportSection &ExportSec = Mod.getExportSection();
  if (auto Res = instantiate(StoreMgr, *ModInst, ExportSec); !Res) {
    LOG(ERROR) << ErrInfo::InfoAST(ExportSec.NodeAttr);
    LOG(ERROR) << ErrInfo::InfoAST(Mod.NodeAttr);
    return Unexpect(Res);
  }

  /// Push a new frame {ModInst, locals:none}
  StackMgr.pushFrame(ModInst->Addr, 0, 0);

  /// Instantiate ElementSection (ElemSec)
  const AST::ElementSection &ElemSec = Mod.getElementSection();
  if (auto Res = instantiate(StoreMgr, *ModInst, ElemSec); !Res) {
    LOG(ERROR) << ErrInfo::InfoAST(ElemSec.NodeAttr);
    LOG(ERROR) << ErrInfo::InfoAST(Mod.NodeAttr);
    return Unexpect(Res);
  }

  /// Instantiate DataSection (DataSec)
  const AST::DataSection &DataSec = Mod.getDataSection();
  if (auto Res = instantiate(StoreMgr, *ModInst, DataSec); !Res) {
    LOG(ERROR) << ErrInfo::InfoAST(DataSec.NodeAttr);
    LOG(ERROR) << ErrInfo::InfoAST(Mod.NodeAttr);
    return Unexpect(Res);
  }

  /// Initialize table instances
  if (auto Res = initTable(StoreMgr, *ModInst, ElemSec); !Res) {
    LOG(ERROR) << ErrInfo::InfoAST(ElemSec.NodeAttr);
    LOG(ERROR) << ErrInfo::InfoAST(Mod.NodeAttr);
    return Unexpect(Res);
  }

  /// Initialize memory instances
  if (auto Res = initMemory(StoreMgr, *ModInst, DataSec); !Res) {
    LOG(ERROR) << ErrInfo::InfoAST(DataSec.NodeAttr);
    LOG(ERROR) << ErrInfo::InfoAST(Mod.NodeAttr);
    return Unexpect(Res);
  }

  /// Prepare pointers for compiled functions
  ModInst->MemoryPtr =
      ModInst->getMemAddr(0)
          .and_then([&StoreMgr](uint32_t MemAddr) {
            return StoreMgr.getMemory(MemAddr);
          })
          .map([](const Runtime::Instance::MemoryInstance *MemInst) {
            return MemInst->getDataPtr();
          })
          .value_or(nullptr);

  ModInst->GlobalsPtr.reserve(ModInst->getGlobalNum());
  for (size_t I = 0; I < ModInst->getGlobalNum(); ++I) {
    ModInst->GlobalsPtr.push_back(
        &(*StoreMgr.getGlobal(*ModInst->getGlobalAddr(I)))->getValue());
  }

  /// Instantiate StartSection (StartSec)
  const AST::StartSection &StartSec = Mod.getStartSection();
  if (StartSec.getContent()) {
    /// Get the module instance from ID.
    ModInst->setStartIdx(*StartSec.getContent());

    /// Get function instance.
    const uint32_t Addr = *ModInst->getStartAddr();
    const auto *FuncInst = *StoreMgr.getFunction(Addr);

    /// Execute instruction: call start.func
    auto Instrs = FuncInst->getInstrs();
    AST::InstrView::iterator StartIt;
    if (auto Res = enterFunction(StoreMgr, *FuncInst, Instrs.end() - 1)) {
      StartIt = *Res;
    } else {
      LOG(ERROR) << ErrInfo::InfoAST(Mod.NodeAttr);
      return Unexpect(Res);
    }
    if (auto Res = execute(StoreMgr, StartIt, Instrs.end()); unlikely(!Res)) {
      LOG(ERROR) << ErrInfo::InfoAST(Mod.NodeAttr);
      return Unexpect(Res);
    }
  }

  /// Pop Frame.
  StackMgr.popFrame();

  return {};
}

} // namespace Interpreter
} // namespace SSVM
