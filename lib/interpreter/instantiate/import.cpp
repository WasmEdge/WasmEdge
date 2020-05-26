// SPDX-License-Identifier: Apache-2.0
#include "common/ast/section.h"
#include "common/types.h"
#include "interpreter/interpreter.h"
#include "runtime/instance/function.h"
#include "runtime/instance/global.h"
#include "runtime/instance/memory.h"
#include "runtime/instance/module.h"
#include "runtime/instance/table.h"

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

ErrCode checkImportError(const std::string &Name,
                         const std::map<std::string, uint32_t> &Map1,
                         const std::map<std::string, uint32_t> &Map2,
                         const std::map<std::string, uint32_t> &Map3) {
  if (Map1.find(Name) != Map1.cend()) {
    return ErrCode::IncompatibleImportType;
  }
  if (Map2.find(Name) != Map2.cend()) {
    return ErrCode::IncompatibleImportType;
  }
  if (Map3.find(Name) != Map3.cend()) {
    return ErrCode::IncompatibleImportType;
  }
  return ErrCode::UnknownImport;
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
    auto &ModName = ImpDesc->getModuleName();
    auto &ExtName = ImpDesc->getExternalName();
    uint32_t TargetAddr;
    Runtime::Instance::ModuleInstance *TargetModInst;
    if (auto Res = StoreMgr.findModule(ModName)) {
      TargetModInst = *Res;
    } else {
      return Unexpect(Res);
    }
    const auto &FuncList = TargetModInst->getFuncExports();
    const auto &TabList = TargetModInst->getTableExports();
    const auto &MemList = TargetModInst->getMemExports();
    const auto &GlobList = TargetModInst->getGlobalExports();

    /// Add the imports into module istance.
    switch (ExtType) {
    case ExternalType::Function: {
      /// Find the function address in Store.
      if (FuncList.find(ExtName) != FuncList.cend()) {
        TargetAddr = FuncList.find(ExtName)->second;
      } else {
        /// Check is error external type or unknown imports.
        return Unexpect(checkImportError(ExtName, TabList, MemList, GlobList));
      }
      /// Get function type index. External type checked in validation.
      uint32_t *TypeIdx = *ImpDesc->getExternalContent<uint32_t>();
      /// Import matching.
      const auto *TargetInst = *StoreMgr.getFunction(TargetAddr);
      const auto &TargetType = TargetInst->getFuncType();
      if (auto Res = ModInst.getFuncType(*TypeIdx)) {
        const auto FuncType = *Res;
        if (TargetType.Params != FuncType->Params ||
            TargetType.Returns != FuncType->Returns) {
          return Unexpect(ErrCode::IncompatibleImportType);
        }
      } else {
        return Unexpect(Res);
      }
      /// Set the matched function address to module instance.
      ModInst.addFuncAddr(TargetAddr);
      break;
    }
    case ExternalType::Table: {
      /// Find the table address in Store.
      if (TabList.find(ExtName) != TabList.cend()) {
        TargetAddr = TabList.find(ExtName)->second;
      } else {
        /// Check is error external type or unknown imports.
        return Unexpect(checkImportError(ExtName, FuncList, MemList, GlobList));
      }
      /// Get table type. External type checked in validation.
      AST::TableType *TabType = *ImpDesc->getExternalContent<AST::TableType>();
      /// Import matching.
      const auto *TargetInst = *StoreMgr.getTable(TargetAddr);
      const auto *TabLim = TabType->getLimit();
      if (TargetInst->getElementType() != TabType->getElementType() ||
          !isLimitMatched(TargetInst->getHasMax(), TargetInst->getMin(),
                          TargetInst->getMax(), TabLim->hasMax(),
                          TabLim->getMin(), TabLim->getMax())) {
        return Unexpect(ErrCode::IncompatibleImportType);
      }
      /// Set the matched table address to module instance.
      ModInst.addTableAddr(TargetAddr);
      break;
    }
    case ExternalType::Memory: {
      /// Find the memory address in Store.
      if (MemList.find(ExtName) != MemList.cend()) {
        TargetAddr = MemList.find(ExtName)->second;
      } else {
        /// Check is error external type or unknown imports.
        return Unexpect(checkImportError(ExtName, FuncList, TabList, GlobList));
      }
      /// Get memory type. External type checked in validation.
      AST::MemoryType *MemType =
          *ImpDesc->getExternalContent<AST::MemoryType>();
      /// Import matching.
      const auto *TargetInst = *StoreMgr.getMemory(TargetAddr);
      const auto *MemLim = MemType->getLimit();
      if (!isLimitMatched(TargetInst->getHasMax(), TargetInst->getMin(),
                          TargetInst->getMax(), MemLim->hasMax(),
                          MemLim->getMin(), MemLim->getMax())) {
        return Unexpect(ErrCode::IncompatibleImportType);
      }
      /// Set the matched memory address to module instance.
      ModInst.addMemAddr(TargetAddr);
      break;
    }
    case ExternalType::Global: {
      /// Find the global address in Store.
      if (GlobList.find(ExtName) != GlobList.cend()) {
        TargetAddr = GlobList.find(ExtName)->second;
      } else {
        /// Check is error external type or unknown imports.
        return Unexpect(checkImportError(ExtName, FuncList, MemList, TabList));
      }
      /// Get global type. External type checked in validation.
      AST::GlobalType *GlobType =
          *ImpDesc->getExternalContent<AST::GlobalType>();
      /// Import matching.
      const auto *TargetInst = *StoreMgr.getGlobal(TargetAddr);
      if (TargetInst->getValType() != GlobType->getValueType() ||
          TargetInst->getValMut() != GlobType->getValueMutation()) {
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
