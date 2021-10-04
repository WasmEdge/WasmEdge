// SPDX-License-Identifier: Apache-2.0

#include "interpreter/interpreter.h"

#include "common/errinfo.h"
#include "common/log.h"

namespace WasmEdge {
namespace Interpreter {

namespace {
template <typename... Args>
auto logMatchError(std::string_view ModName, std::string_view ExtName,
                   ExternalType ExtType, ASTNodeAttr Node, Args &&...Values) {
  spdlog::error(ErrCode::IncompatibleImportType);
  spdlog::error(ErrInfo::InfoMismatch(std::forward<Args>(Values)...));
  spdlog::error(ErrInfo::InfoLinking(ModName, ExtName, ExtType));
  spdlog::error(ErrInfo::InfoAST(Node));
  return Unexpect(ErrCode::IncompatibleImportType);
}

auto logUnknownError(std::string_view ModName, std::string_view ExtName,
                     ExternalType ExtType, ASTNodeAttr Node) {
  spdlog::error(ErrCode::UnknownImport);
  spdlog::error(ErrInfo::InfoLinking(ModName, ExtName, ExtType));
  spdlog::error(ErrInfo::InfoAST(Node));
  return Unexpect(ErrCode::UnknownImport);
}

bool isLimitMatched(const bool HasMax1, const uint32_t Min1,
                    const uint32_t Max1, const bool HasMax2,
                    const uint32_t Min2, const uint32_t Max2) {
  if ((Min1 < Min2) || (!HasMax1 && HasMax2)) {
    return false;
  }
  if (HasMax1 && HasMax2 && Max1 > Max2) {
    return false;
  }
  return true;
}

Expect<uint32_t> getImportAddr(std::string_view ModName,
                               std::string_view ExtName,
                               const ExternalType ExtType, ASTNodeAttr Node,
                               Runtime::Instance::ModuleInstance &ModInst) {
  const auto &FuncList = ModInst.getFuncExports();
  const auto &TabList = ModInst.getTableExports();
  const auto &MemList = ModInst.getMemExports();
  const auto &GlobList = ModInst.getGlobalExports();

  switch (ExtType) {
  case ExternalType::Function:
    if (FuncList.find(ExtName) != FuncList.cend()) {
      return FuncList.find(ExtName)->second;
    }
    break;
  case ExternalType::Table:
    if (TabList.find(ExtName) != TabList.cend()) {
      return TabList.find(ExtName)->second;
    }
    break;
  case ExternalType::Memory:
    if (MemList.find(ExtName) != MemList.cend()) {
      return MemList.find(ExtName)->second;
    }
    break;
  case ExternalType::Global:
    if (GlobList.find(ExtName) != GlobList.cend()) {
      return GlobList.find(ExtName)->second;
    }
    break;
  default:
    return logUnknownError(ModName, ExtName, ExtType, Node);
  }

  /// Check is error external type or unknown imports.
  if (FuncList.find(ExtName) != FuncList.cend()) {
    return logMatchError(ModName, ExtName, ExtType, Node, ExtType,
                         ExternalType::Function);
  }
  if (TabList.find(ExtName) != TabList.cend()) {
    return logMatchError(ModName, ExtName, ExtType, Node, ExtType,
                         ExternalType::Table);
  }
  if (MemList.find(ExtName) != MemList.cend()) {
    return logMatchError(ModName, ExtName, ExtType, Node, ExtType,
                         ExternalType::Memory);
  }
  if (GlobList.find(ExtName) != GlobList.cend()) {
    return logMatchError(ModName, ExtName, ExtType, Node, ExtType,
                         ExternalType::Global);
  }

  return logUnknownError(ModName, ExtName, ExtType, Node);
}
} // namespace

/// Instantiate imports. See "include/interpreter/interpreter.h".
Expect<void>
Interpreter::instantiate(Runtime::StoreManager &StoreMgr,
                         Runtime::Instance::ModuleInstance &ModInst,
                         const AST::ImportSection &ImportSec) {
  /// Iterate and instantiate import descriptions.
  for (const auto &ImpDesc : ImportSec.getContent()) {
    /// Get data from import description and find import module.
    auto ExtType = ImpDesc.getExternalType();
    auto ModName = ImpDesc.getModuleName();
    auto ExtName = ImpDesc.getExternalName();
    Runtime::Instance::ModuleInstance *TargetModInst;
    uint32_t TargetAddr;
    if (auto Res = StoreMgr.findModule(ModName)) {
      TargetModInst = *Res;
    } else {
      return logUnknownError(ModName, ExtName, ExtType,
                             ASTNodeAttr::Desc_Import);
    }
    if (auto Res = getImportAddr(ModName, ExtName, ExtType,
                                 ASTNodeAttr::Desc_Import, *TargetModInst)) {
      TargetAddr = *Res;
    } else {
      spdlog::error(ErrInfo::InfoLinking(ModName, ExtName, ExtType));
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Desc_Import));
      return Unexpect(Res);
    }

    /// Add the imports into module istance.
    switch (ExtType) {
    case ExternalType::Function: {
      /// Get function type index. External type checked in validation.
      uint32_t TypeIdx = ImpDesc.getExternalFuncTypeIdx();
      /// Import matching.
      const auto *TargetInst = *StoreMgr.getFunction(TargetAddr);
      const auto &TargetType = TargetInst->getFuncType();
      const auto *FuncType = *ModInst.getFuncType(TypeIdx);
      if (TargetType != *FuncType) {
        return logMatchError(
            ModName, ExtName, ExtType, ASTNodeAttr::Desc_Import,
            FuncType->getParamTypes(), FuncType->getReturnTypes(),
            TargetType.getParamTypes(), TargetType.getReturnTypes());
      }
      /// Set the matched function address to module instance.
      ModInst.importFunction(TargetAddr);
      break;
    }
    case ExternalType::Table: {
      /// Get table type. External type checked in validation.
      const auto &TabType = ImpDesc.getExternalTableType();
      /// Import matching.
      const auto *TargetInst = *StoreMgr.getTable(TargetAddr);
      const auto &TabLim = TabType.getLimit();
      if (TargetInst->getTableType().getRefType() != TabType.getRefType() ||
          !isLimitMatched(TargetInst->getTableType().getLimit().hasMax(),
                          TargetInst->getTableType().getLimit().getMin(),
                          TargetInst->getTableType().getLimit().getMax(),
                          TabLim.hasMax(), TabLim.getMin(), TabLim.getMax())) {
        return logMatchError(ModName, ExtName, ExtType,
                             ASTNodeAttr::Desc_Import, TabType.getRefType(),
                             TabLim.hasMax(), TabLim.getMin(), TabLim.getMax(),
                             TargetInst->getTableType().getRefType(),
                             TargetInst->getTableType().getLimit().hasMax(),
                             TargetInst->getTableType().getLimit().getMin(),
                             TargetInst->getTableType().getLimit().getMax());
      }
      /// Set the matched table address to module instance.
      ModInst.importTable(TargetAddr);
      break;
    }
    case ExternalType::Memory: {
      /// Get memory type. External type checked in validation.
      const auto &MemType = ImpDesc.getExternalMemoryType();
      /// Import matching.
      auto *TargetInst = *StoreMgr.getMemory(TargetAddr);
      const auto &MemLim = MemType.getLimit();
      if (!isLimitMatched(TargetInst->getMemoryType().getLimit().hasMax(),
                          TargetInst->getMemoryType().getLimit().getMin(),
                          TargetInst->getMemoryType().getLimit().getMax(),
                          MemLim.hasMax(), MemLim.getMin(), MemLim.getMax())) {
        return logMatchError(ModName, ExtName, ExtType,
                             ASTNodeAttr::Desc_Import, MemLim.hasMax(),
                             MemLim.getMin(), MemLim.getMax(),
                             TargetInst->getMemoryType().getLimit().hasMax(),
                             TargetInst->getMemoryType().getLimit().getMin(),
                             TargetInst->getMemoryType().getLimit().getMax());
      }
      /// Set the matched memory address to module instance.
      ModInst.importMemory(TargetAddr);
      break;
    }
    case ExternalType::Global: {
      /// Get global type. External type checked in validation.
      const auto &GlobType = ImpDesc.getExternalGlobalType();
      /// Import matching.
      const auto *TargetInst = *StoreMgr.getGlobal(TargetAddr);
      if (TargetInst->getGlobalType().getValType() != GlobType.getValType() ||
          TargetInst->getGlobalType().getValMut() != GlobType.getValMut()) {
        return logMatchError(ModName, ExtName, ExtType,
                             ASTNodeAttr::Desc_Import, GlobType.getValType(),
                             GlobType.getValMut(),
                             TargetInst->getGlobalType().getValType(),
                             TargetInst->getGlobalType().getValMut());
      }
      /// Set the matched global address to module instance.
      ModInst.importGlobal(TargetAddr);
      break;
    }
    default:
      break;
    }
  }
  return {};
}

} // namespace Interpreter
} // namespace WasmEdge
