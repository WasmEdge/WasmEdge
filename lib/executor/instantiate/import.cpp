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

bool matchLimit(const AST::Limit &Exp, const AST::Limit &Got) {
  if (Exp.isShared() != Got.isShared()) {
    return false;
  }
  if ((Got.getMin() < Exp.getMin()) || (Exp.hasMax() && !Got.hasMax())) {
    return false;
  }
  if (Exp.hasMax() && Got.hasMax() && Got.getMax() > Exp.getMax()) {
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

    // Add the imports into module instance.
    switch (ExtType) {
    case ExternalType::Function: {
      // Get function type index. External type checked in validation.
      uint32_t TypeIdx = ImpDesc.getExternalFuncTypeIdx();
      // Import matching.
      auto *TargetInst = TargetModInst->findFuncExports(ExtName);
      const auto &TargetType = TargetInst->getFuncType();
      const auto *FuncType = *ModInst.getFuncType(TypeIdx);
      // External function type should match the import function type in
      // description.
      if (!matchTypes(ModInst, FuncType->getParamTypes(), *TargetModInst,
                      TargetType.getParamTypes()) ||
          !matchTypes(ModInst, FuncType->getReturnTypes(), *TargetModInst,
                      TargetType.getReturnTypes())) {
        bool IsMatchV2 = false;
        if (ModName == "wasi_snapshot_preview1") {
          /*
           * The following functions should provide V1 and V2.
             "sock_open_v2",
             "sock_bind_v2",
             "sock_connect_v2",
             "sock_listen_v2",
             "sock_accept_v2",
             "sock_recv_v2",
             "sock_recv_from_v2",
             "sock_send_v2",
             "sock_send_to_v2",
             "sock_getlocaladdr_v2",
             "sock_getpeeraddr_v2"
             */
          std::vector<std::string> CompatibleWASISocketAPI = {
              "sock_open",         "sock_bind",       "sock_connect",
              "sock_listen",       "sock_accept",     "sock_recv",
              "sock_recv_from",    "sock_send",       "sock_send_to",
              "sock_getlocaladdr", "sock_getpeeraddr"};
          for (auto Iter = CompatibleWASISocketAPI.begin();
               Iter != CompatibleWASISocketAPI.end(); Iter++) {
            if (ExtName == *Iter) {
              auto *TargetInstV2 =
                  TargetModInst->findFuncExports(*Iter + "_v2");
              if (TargetInstV2->getFuncType() == *FuncType) {
                // Try to match the new version
                TargetInst = TargetInstV2;
                IsMatchV2 = true;
                break;
              }
            }
          }
        }
        if (!IsMatchV2) {
          return logMatchError(
              ModName, ExtName, ExtType, FuncType->getParamTypes(),
              FuncType->getReturnTypes(), TargetType.getParamTypes(),
              TargetType.getReturnTypes());
        }
      }
      // Set the matched function address to module instance.
      ModInst.importFunction(TargetInst);
      break;
    }
    case ExternalType::Table: {
      // Get table type. External type checked in validation.
      const auto &TabType = ImpDesc.getExternalTableType();
      const auto &TabLim = TabType.getLimit();
      // Import matching. External table type should match the one in import
      // description.
      auto *TargetInst = TargetModInst->findTableExports(ExtName);
      const auto &TargetType = TargetInst->getTableType();
      const auto &TargetLim = TargetType.getLimit();
      // External table reference type should match the import table reference
      // type in description, and vice versa.
      if (!matchType(ModInst, TabType.getRefType(), *TargetModInst,
                     TargetType.getRefType()) ||
          !matchType(*TargetModInst, TargetType.getRefType(), ModInst,
                     TabType.getRefType()) ||
          !matchLimit(TabLim, TargetLim)) {
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
      // Import matching. External memory type should match the one in import
      // description.
      auto *TargetInst = TargetModInst->findMemoryExports(ExtName);
      const auto &TargetLim = TargetInst->getMemoryType().getLimit();
      if (!matchLimit(MemLim, TargetLim)) {
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
      // Import matching. External global type should match the one in
      // import description.
      auto *TargetInst = TargetModInst->findGlobalExports(ExtName);
      const auto &TargetType = TargetInst->getGlobalType();
      bool IsMatch = false;
      if (TargetType.getValMut() == GlobType.getValMut()) {
        // For both const or both var: external global value type should match
        // the import global value type in description.
        IsMatch = matchType(ModInst, GlobType.getValType(), *TargetModInst,
                            TargetType.getValType());
        if (TargetType.getValMut() == ValMut::Var) {
          // If both var: import global value type in description should also
          // match the external global value type.
          IsMatch &= matchType(*TargetModInst, TargetType.getValType(), ModInst,
                               GlobType.getValType());
        }
      }
      if (!IsMatch) {
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
