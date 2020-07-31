// SPDX-License-Identifier: Apache-2.0
#include "common/ast/section.h"
#include "common/types.h"
#include "interpreter/interpreter.h"
#include "runtime/instance/function.h"
#include "runtime/instance/global.h"
#include "runtime/instance/memory.h"
#include "runtime/instance/module.h"
#include "runtime/instance/table.h"
#include "support/log.h"

namespace SSVM {
namespace Interpreter {

namespace {
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

Expect<uint32_t> getImportAddr(std::string_view Name,
                               const ExternalType ExtType,
                               Runtime::Instance::ModuleInstance &ModInst) {
  const auto &FuncList = ModInst.getFuncExports();
  const auto &TabList = ModInst.getTableExports();
  const auto &MemList = ModInst.getMemExports();
  const auto &GlobList = ModInst.getGlobalExports();

  switch (ExtType) {
  case ExternalType::Function:
    if (FuncList.find(Name) != FuncList.cend()) {
      return FuncList.find(Name)->second;
    }
    break;
  case ExternalType::Table:
    if (TabList.find(Name) != TabList.cend()) {
      return TabList.find(Name)->second;
    }
    break;
  case ExternalType::Memory:
    if (MemList.find(Name) != MemList.cend()) {
      return MemList.find(Name)->second;
    }
    break;
  case ExternalType::Global:
    if (GlobList.find(Name) != GlobList.cend()) {
      return GlobList.find(Name)->second;
    }
    break;
  default:
    LOG(ERROR) << ErrCode::UnknownImport;
    return Unexpect(ErrCode::UnknownImport);
  }

  /// Check is error external type or unknown imports.
  if (FuncList.find(Name) != FuncList.cend()) {
    LOG(ERROR) << ErrCode::IncompatibleImportType;
    LOG(ERROR) << ErrInfo::InfoMismatch(ExtType, ExternalType::Function);
    return Unexpect(ErrCode::IncompatibleImportType);
  }
  if (TabList.find(Name) != TabList.cend()) {
    LOG(ERROR) << ErrCode::IncompatibleImportType;
    LOG(ERROR) << ErrInfo::InfoMismatch(ExtType, ExternalType::Table);
    return Unexpect(ErrCode::IncompatibleImportType);
  }
  if (MemList.find(Name) != MemList.cend()) {
    LOG(ERROR) << ErrCode::IncompatibleImportType;
    LOG(ERROR) << ErrInfo::InfoMismatch(ExtType, ExternalType::Memory);
    return Unexpect(ErrCode::IncompatibleImportType);
  }
  if (GlobList.find(Name) != GlobList.cend()) {
    LOG(ERROR) << ErrCode::IncompatibleImportType;
    LOG(ERROR) << ErrInfo::InfoMismatch(ExtType, ExternalType::Global);
    return Unexpect(ErrCode::IncompatibleImportType);
  }

  LOG(ERROR) << ErrCode::UnknownImport;
  return Unexpect(ErrCode::UnknownImport);
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
    auto ExtType = ImpDesc->getExternalType();
    auto ModName = ImpDesc->getModuleName();
    auto ExtName = ImpDesc->getExternalName();
    Runtime::Instance::ModuleInstance *TargetModInst;
    uint32_t TargetAddr;
    if (auto Res = StoreMgr.findModule(ModName)) {
      TargetModInst = *Res;
    } else {
      LOG(ERROR) << ErrCode::UnknownImport;
      LOG(ERROR) << ErrInfo::InfoLinking(ModName, ExtName, ExtType);
      LOG(ERROR) << ErrInfo::InfoAST(ImpDesc->NodeAttr);
      return Unexpect(ErrCode::UnknownImport);
    }
    if (auto Res = getImportAddr(ExtName, ExtType, *TargetModInst)) {
      TargetAddr = *Res;
    } else {
      LOG(ERROR) << ErrInfo::InfoLinking(ModName, ExtName, ExtType);
      LOG(ERROR) << ErrInfo::InfoAST(ImpDesc->NodeAttr);
      return Unexpect(Res);
    }

    /// Add the imports into module istance.
    switch (ExtType) {
    case ExternalType::Function: {
      /// Get function type index. External type checked in validation.
      uint32_t *TypeIdx = *ImpDesc->getExternalContent<uint32_t>();
      /// Import matching.
      const auto *TargetInst = *StoreMgr.getFunction(TargetAddr);
      const auto &TargetType = TargetInst->getFuncType();
      const auto *FuncType = *ModInst.getFuncType(*TypeIdx);
      if (TargetType.Params != FuncType->Params ||
          TargetType.Returns != FuncType->Returns) {
        LOG(ERROR) << ErrCode::IncompatibleImportType;
        LOG(ERROR) << ErrInfo::InfoMismatch(FuncType->Params, FuncType->Returns,
                                            TargetType.Params,
                                            TargetType.Returns);
        LOG(ERROR) << ErrInfo::InfoLinking(ModName, ExtName, ExtType);
        LOG(ERROR) << ErrInfo::InfoAST(ImpDesc->NodeAttr);
        return Unexpect(ErrCode::IncompatibleImportType);
      }
      /// Set the matched function address to module instance.
      ModInst.addFuncAddr(TargetAddr);
      break;
    }
    case ExternalType::Table: {
      /// Get table type. External type checked in validation.
      AST::TableType *TabType = *ImpDesc->getExternalContent<AST::TableType>();
      /// Import matching.
      const auto *TargetInst = *StoreMgr.getTable(TargetAddr);
      const auto *TabLim = TabType->getLimit();
      if (TargetInst->getReferenceType() != TabType->getReferenceType() ||
          !isLimitMatched(TargetInst->getHasMax(), TargetInst->getMin(),
                          TargetInst->getMax(), TabLim->hasMax(),
                          TabLim->getMin(), TabLim->getMax())) {
        LOG(ERROR) << ErrCode::IncompatibleImportType;
        LOG(ERROR) << ErrInfo::InfoMismatch(
            TabType->getReferenceType(), TabLim->hasMax(), TabLim->getMin(),
            TabLim->getMax(), TargetInst->getReferenceType(),
            TargetInst->getHasMax(), TargetInst->getMin(),
            TargetInst->getMax());
        LOG(ERROR) << ErrInfo::InfoLinking(ModName, ExtName, ExtType);
        LOG(ERROR) << ErrInfo::InfoAST(ImpDesc->NodeAttr);
        return Unexpect(ErrCode::IncompatibleImportType);
      }
      /// Set the matched table address to module instance.
      ModInst.addTableAddr(TargetAddr);
      break;
    }
    case ExternalType::Memory: {
      /// Get memory type. External type checked in validation.
      AST::MemoryType *MemType =
          *ImpDesc->getExternalContent<AST::MemoryType>();
      /// Import matching.
      const auto *TargetInst = *StoreMgr.getMemory(TargetAddr);
      const auto *MemLim = MemType->getLimit();
      if (!isLimitMatched(TargetInst->getHasMax(), TargetInst->getMin(),
                          TargetInst->getMax(), MemLim->hasMax(),
                          MemLim->getMin(), MemLim->getMax())) {
        LOG(ERROR) << ErrCode::IncompatibleImportType;
        LOG(ERROR) << ErrInfo::InfoMismatch(
            MemLim->hasMax(), MemLim->getMin(), MemLim->getMax(),
            TargetInst->getHasMax(), TargetInst->getMin(),
            TargetInst->getMax());
        LOG(ERROR) << ErrInfo::InfoLinking(ModName, ExtName, ExtType);
        LOG(ERROR) << ErrInfo::InfoAST(ImpDesc->NodeAttr);
        return Unexpect(ErrCode::IncompatibleImportType);
      }
      /// Set the matched memory address to module instance.
      ModInst.addMemAddr(TargetAddr);
      break;
    }
    case ExternalType::Global: {
      /// Get global type. External type checked in validation.
      AST::GlobalType *GlobType =
          *ImpDesc->getExternalContent<AST::GlobalType>();
      /// Import matching.
      const auto *TargetInst = *StoreMgr.getGlobal(TargetAddr);
      if (TargetInst->getValType() != GlobType->getValueType() ||
          TargetInst->getValMut() != GlobType->getValueMutation()) {
        LOG(ERROR) << ErrCode::IncompatibleImportType;
        LOG(ERROR) << ErrInfo::InfoMismatch(
            GlobType->getValueType(), GlobType->getValueMutation(),
            TargetInst->getValType(), TargetInst->getValMut());
        LOG(ERROR) << ErrInfo::InfoLinking(ModName, ExtName, ExtType);
        LOG(ERROR) << ErrInfo::InfoAST(ImpDesc->NodeAttr);
        return Unexpect(ErrCode::IncompatibleImportType);
      }
      /// Set the matched global address to module instance.
      ModInst.addGlobalAddr(TargetAddr);
      break;
    }
    default:
      break;
    }
  }
  return {};
}

} // namespace Interpreter
} // namespace SSVM
