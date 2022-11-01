// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "executor/executor.h"

#include "common/errinfo.h"
#include "common/log.h"

#include <cstdint>
#include <string_view>
#include <utility>

namespace WasmEdge {
namespace Executor {

namespace {
template <typename... Args>
auto logMatchError(std::string_view ModName, std::string_view ExtName,
                   ExternalType ExtType, Args &&...Values) {
  spdlog::error(ErrCode::Value::IncompatibleImportType);
  spdlog::error(ErrInfo::InfoMismatch(std::forward<Args>(Values)...));
  spdlog::error(ErrInfo::InfoLinking(ModName, ExtName, ExtType));
  spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Desc_Import));
  return Unexpect(ErrCode::Value::IncompatibleImportType);
}

auto logUnknownError(std::string_view ModName, std::string_view ExtName,
                     ExternalType ExtType) {
  spdlog::error(ErrCode::Value::UnknownImport);
  spdlog::error(ErrInfo::InfoLinking(ModName, ExtName, ExtType));
  spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Desc_Import));
  return Unexpect(ErrCode::Value::UnknownImport);
}

bool isLimitMatched(const AST::Limit &Lim1, const AST::Limit &Lim2) {
  if (Lim1.isShared() != Lim2.isShared()) {
    return false;
  }
  if ((Lim1.getMin() < Lim2.getMin()) || (!Lim1.hasMax() && Lim2.hasMax())) {
    return false;
  }
  if (Lim1.hasMax() && Lim2.hasMax() && Lim1.getMax() > Lim2.getMax()) {
    return false;
  }
  return true;
}

Expect<void>
checkImportMatched(std::string_view ModName, std::string_view ExtName,
                   const ExternalType ExtType,
                   const Runtime::Instance::ModuleInstance &ModInst) {
  switch (ExtType) {
  case ExternalType::Function:
    if (auto Res = ModInst.findFuncExports(ExtName); likely(Res != nullptr)) {
      return {};
    }
    break;
  case ExternalType::Table:
    if (auto Res = ModInst.findTableExports(ExtName); likely(Res != nullptr)) {
      return {};
    }
    break;
  case ExternalType::Memory:
    if (auto Res = ModInst.findMemoryExports(ExtName); likely(Res != nullptr)) {
      return {};
    }
    break;
  case ExternalType::Global:
    if (auto Res = ModInst.findGlobalExports(ExtName); likely(Res != nullptr)) {
      return {};
    }
    break;
  default:
    return logUnknownError(ModName, ExtName, ExtType);
  }

  // Check is error external type or unknown imports.
  if (ModInst.findFuncExports(ExtName)) {
    return logMatchError(ModName, ExtName, ExtType, ExtType,
                         ExternalType::Function);
  }
  if (ModInst.findTableExports(ExtName)) {
    return logMatchError(ModName, ExtName, ExtType, ExtType,
                         ExternalType::Table);
  }
  if (ModInst.findMemoryExports(ExtName)) {
    return logMatchError(ModName, ExtName, ExtType, ExtType,
                         ExternalType::Memory);
  }
  if (ModInst.findGlobalExports(ExtName)) {
    return logMatchError(ModName, ExtName, ExtType, ExtType,
                         ExternalType::Global);
  }

  return logUnknownError(ModName, ExtName, ExtType);
}
} // namespace

// Instantiate imports. See "include/executor/executor.h".
Expect<void> Executor::instantiate(Runtime::StoreManager &StoreMgr,
                                   Runtime::Instance::ModuleInstance &ModInst,
                                   const AST::ImportSection &ImportSec) {
  // Iterate and instantiate import descriptions.
  for (const auto &ImpDesc : ImportSec.getContent()) {
    // Get data from import description and find import module.
    auto ExtType = ImpDesc.getExternalType();
    auto ModName = ImpDesc.getModuleName();
    auto ExtName = ImpDesc.getExternalName();
    const auto *TargetModInst = StoreMgr.findModule(ModName);
    if (unlikely(TargetModInst == nullptr)) {
      auto Res = logUnknownError(ModName, ExtName, ExtType);
      if (ModName == "wasi_snapshot_preview1") {
        spdlog::error("    This is a WASI related import. Please ensure that "
                      "you've turned on the WASI configuration.");
      } else if (ModName == "wasi_nn") {
        spdlog::error("    This is a WASI-NN related import. Please ensure "
                      "that you've turned on the WASI-NN configuration and "
                      "installed the WASI-NN plug-in.");
      } else if (ModName == "wasi_crypto_common" ||
                 ModName == "wasi_crypto_asymmetric_common" ||
                 ModName == "wasi_crypto_kx" ||
                 ModName == "wasi_crypto_signatures" ||
                 ModName == "wasi_crypto_symmetric") {
        spdlog::error("    This is a WASI-Crypto related import. Please ensure "
                      "that you've turned on the WASI-Crypto configuration and "
                      "installed the WASI-Crypto plug-in.");
      } else if (ModName == "env") {
        spdlog::error(
            "    This may be the import of host environment like JavaScript or "
            "Golang. Please check that you've registered the necessary host "
            "modules from the host programming language.");
      }
      return Res;
    }
    if (auto Res =
            checkImportMatched(ModName, ExtName, ExtType, *TargetModInst);
        unlikely(!Res)) {
      return Unexpect(Res);
    }

    // Add the imports into module istance.
    switch (ExtType) {
    case ExternalType::Function: {
      // Get function type index. External type checked in validation.
      uint32_t TypeIdx = ImpDesc.getExternalFuncTypeIdx();
      // Import matching.
      auto *TargetInst = TargetModInst->findFuncExports(ExtName);
      const auto &TargetType = TargetInst->getFuncType();
      const auto *FuncType = *ModInst.getFuncType(TypeIdx);
      if (TargetType != *FuncType) {
        return logMatchError(
            ModName, ExtName, ExtType, FuncType->getParamTypes(),
            FuncType->getReturnTypes(), TargetType.getParamTypes(),
            TargetType.getReturnTypes());
      }
      // Set the matched function address to module instance.
      ModInst.importFunction(TargetInst);
      break;
    }
    case ExternalType::Table: {
      // Get table type. External type checked in validation.
      const auto &TabType = ImpDesc.getExternalTableType();
      const auto &TabLim = TabType.getLimit();
      // Import matching.
      auto *TargetInst = TargetModInst->findTableExports(ExtName);
      const auto &TargetType = TargetInst->getTableType();
      const auto &TargetLim = TargetType.getLimit();
      if (TargetType.getRefType() != TabType.getRefType() ||
          !isLimitMatched(TargetLim, TabLim)) {
        return logMatchError(ModName, ExtName, ExtType, TabType.getRefType(),
                             TabLim.hasMax(), TabLim.getMin(), TabLim.getMax(),
                             TargetType.getRefType(), TargetLim.hasMax(),
                             TargetLim.getMin(), TargetLim.getMax());
      }
      // Set the matched table address to module instance.
      ModInst.importTable(TargetInst);
      break;
    }
    case ExternalType::Memory: {
      // Get memory type. External type checked in validation.
      const auto &MemType = ImpDesc.getExternalMemoryType();
      const auto &MemLim = MemType.getLimit();
      // Import matching.
      auto *TargetInst = TargetModInst->findMemoryExports(ExtName);
      const auto &TargetLim = TargetInst->getMemoryType().getLimit();
      if (!isLimitMatched(TargetLim, MemLim)) {
        return logMatchError(ModName, ExtName, ExtType, MemLim.hasMax(),
                             MemLim.getMin(), MemLim.getMax(),
                             TargetLim.hasMax(), TargetLim.getMin(),
                             TargetLim.getMax());
      }
      // Set the matched memory address to module instance.
      ModInst.importMemory(TargetInst);
      break;
    }
    case ExternalType::Global: {
      // Get global type. External type checked in validation.
      const auto &GlobType = ImpDesc.getExternalGlobalType();
      // Import matching.
      auto *TargetInst = TargetModInst->findGlobalExports(ExtName);
      const auto &TargetType = TargetInst->getGlobalType();
      if (TargetType != GlobType) {
        return logMatchError(ModName, ExtName, ExtType, GlobType.getValType(),
                             GlobType.getValMut(), TargetType.getValType(),
                             TargetType.getValMut());
      }
      // Set the matched global address to module instance.
      ModInst.importGlobal(TargetInst);
      break;
    }
    default:
      break;
    }
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
