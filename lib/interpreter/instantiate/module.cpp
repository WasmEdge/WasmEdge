// SPDX-License-Identifier: Apache-2.0

#include "interpreter/interpreter.h"

namespace WasmEdge {
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
    spdlog::error(ErrCode::ModuleNameConflict);
    spdlog::error(ErrInfo::InfoAST(Mod.NodeAttr));
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
    spdlog::error(ErrInfo::InfoAST(ImportSec.NodeAttr));
    spdlog::error(ErrInfo::InfoAST(Mod.NodeAttr));
    return Unexpect(Res);
  }

  /// Instantiate Functions in module. (FunctionSec, CodeSec)
  const AST::FunctionSection &FuncSec = Mod.getFunctionSection();
  const AST::CodeSection &CodeSec = Mod.getCodeSection();
  if (auto Res = instantiate(StoreMgr, *ModInst, FuncSec, CodeSec); !Res) {
    spdlog::error(ErrInfo::InfoAST(FuncSec.NodeAttr));
    spdlog::error(ErrInfo::InfoAST(Mod.NodeAttr));
    return Unexpect(Res);
  }

  /// Instantiate TableSection (TableSec)
  const AST::TableSection &TabSec = Mod.getTableSection();
  if (auto Res = instantiate(StoreMgr, *ModInst, TabSec); !Res) {
    spdlog::error(ErrInfo::InfoAST(TabSec.NodeAttr));
    spdlog::error(ErrInfo::InfoAST(Mod.NodeAttr));
    return Unexpect(Res);
  }

  /// Instantiate MemorySection (MemorySec)
  const AST::MemorySection &MemSec = Mod.getMemorySection();
  if (auto Res = instantiate(StoreMgr, *ModInst, MemSec); !Res) {
    spdlog::error(ErrInfo::InfoAST(MemSec.NodeAttr));
    spdlog::error(ErrInfo::InfoAST(Mod.NodeAttr));
    return Unexpect(Res);
  }

  /// Add a temp module to Store with only imported globals for initialization.
  uint32_t TmpModInstAddr = StoreMgr.pushModule("");
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
    spdlog::error(ErrInfo::InfoAST(GlobSec.NodeAttr));
    spdlog::error(ErrInfo::InfoAST(Mod.NodeAttr));
    return Unexpect(Res);
  }

  /// Pop frame with temp. module.
  StackMgr.popFrame();

  /// Pop the added temp. module.
  StoreMgr.popModule();

  /// Instantiate ExportSection (ExportSec)
  const AST::ExportSection &ExportSec = Mod.getExportSection();
  if (auto Res = instantiate(StoreMgr, *ModInst, ExportSec); !Res) {
    spdlog::error(ErrInfo::InfoAST(ExportSec.NodeAttr));
    spdlog::error(ErrInfo::InfoAST(Mod.NodeAttr));
    return Unexpect(Res);
  }

  /// Push a new frame {ModInst, locals:none}
  StackMgr.pushFrame(ModInst->Addr, 0, 0);

  /// Instantiate ElementSection (ElemSec)
  const AST::ElementSection &ElemSec = Mod.getElementSection();
  if (auto Res = instantiate(StoreMgr, *ModInst, ElemSec); !Res) {
    spdlog::error(ErrInfo::InfoAST(ElemSec.NodeAttr));
    spdlog::error(ErrInfo::InfoAST(Mod.NodeAttr));
    return Unexpect(Res);
  }

  /// Instantiate DataSection (DataSec)
  const AST::DataSection &DataSec = Mod.getDataSection();
  if (auto Res = instantiate(StoreMgr, *ModInst, DataSec); !Res) {
    spdlog::error(ErrInfo::InfoAST(DataSec.NodeAttr));
    spdlog::error(ErrInfo::InfoAST(Mod.NodeAttr));
    return Unexpect(Res);
  }

  /// Initialize table instances
  if (auto Res = initTable(StoreMgr, *ModInst, ElemSec); !Res) {
    spdlog::error(ErrInfo::InfoAST(ElemSec.NodeAttr));
    spdlog::error(ErrInfo::InfoAST(Mod.NodeAttr));
    return Unexpect(Res);
  }

  /// Initialize memory instances
  if (auto Res = initMemory(StoreMgr, *ModInst, DataSec); !Res) {
    spdlog::error(ErrInfo::InfoAST(DataSec.NodeAttr));
    spdlog::error(ErrInfo::InfoAST(Mod.NodeAttr));
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
  for (uint32_t I = 0; I < static_cast<uint32_t>(ModInst->getGlobalNum());
       ++I) {
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
    if (auto Res = enterFunction(StoreMgr, *FuncInst, Instrs.end())) {
      StartIt = *Res;
    } else {
      spdlog::error(ErrInfo::InfoAST(Mod.NodeAttr));
      return Unexpect(Res);
    }
    if (auto Res = execute(StoreMgr, StartIt, Instrs.end()); unlikely(!Res)) {
      spdlog::error(ErrInfo::InfoAST(Mod.NodeAttr));
      return Unexpect(Res);
    }
  }

  /// Pop Frame.
  StackMgr.popFrame();

  return {};
}

} // namespace Interpreter
} // namespace WasmEdge
