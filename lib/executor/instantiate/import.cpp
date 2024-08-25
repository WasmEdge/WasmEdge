// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "executor/executor.h"

#include "common/errinfo.h"
#include "common/spdlog.h"

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
  case ExternalType::Tag:
    if (auto Res = ModInst.findTagExports(ExtName); likely(Res != nullptr)) {
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
  if (ModInst.findTagExports(ExtName)) {
    return logMatchError(ModName, ExtName, ExtType, ExtType, ExternalType::Tag);
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
    const auto *ImpModInst = StoreMgr.findModule(ModName);
    if (unlikely(ImpModInst == nullptr)) {
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
    if (auto Res = checkImportMatched(ModName, ExtName, ExtType, *ImpModInst);
        unlikely(!Res)) {
      return Unexpect(Res);
    }

    // Add the imports into module instance.
    switch (ExtType) {
    case ExternalType::Function: {
      // Get function type index. External type checked in validation.
      uint32_t TypeIdx = ImpDesc.getExternalFuncTypeIdx();
      // Import matching.
      auto *ImpInst = ImpModInst->findFuncExports(ExtName);
      // External function type should match the import function type in
      // description.

      if (!AST::TypeMatcher::matchType(ModInst.getTypeList(), TypeIdx,
                                       ImpModInst->getTypeList(),
                                       ImpInst->getTypeIndex())) {
        const auto &ExpDefType = **ModInst.getType(TypeIdx);
        bool IsMatchV2 = false;
        const auto &ExpFuncType = ExpDefType.getCompositeType().getFuncType();
        const auto &ImpFuncType = ImpInst->getFuncType();
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
              auto *ImpInstV2 = ImpModInst->findFuncExports(*Iter + "_v2");
              if (!AST::TypeMatcher::matchType(
                      ModInst.getTypeList(), *ExpDefType.getTypeIndex(),
                      ImpModInst->getTypeList(), ImpInst->getTypeIndex())) {
                // Try to match the new version
                ImpInst = ImpInstV2;
                IsMatchV2 = true;
                break;
              }
            }
          }
        }
        if (!IsMatchV2) {
          return logMatchError(
              ModName, ExtName, ExtType, ExpFuncType.getParamTypes(),
              ExpFuncType.getReturnTypes(), ImpFuncType.getParamTypes(),
              ImpFuncType.getReturnTypes());
        }
      }
      // Set the matched function address to module instance.
      ModInst.importFunction(ImpInst);
      break;
    }
    case ExternalType::Table: {
      // Get table type. External type checked in validation.
      const auto &TabType = ImpDesc.getExternalTableType();
      const auto &TabLim = TabType.getLimit();
      // Import matching. External table type should match the one in import
      // description.
      auto *ImpInst = ImpModInst->findTableExports(ExtName);
      const auto &ImpType = ImpInst->getTableType();
      const auto &ImpLim = ImpType.getLimit();
      // External table reference type should match the import table reference
      // type in description, and vice versa.
      if (!AST::TypeMatcher::matchType(
              ModInst.getTypeList(), TabType.getRefType(),
              ImpModInst->getTypeList(), ImpType.getRefType()) ||
          !AST::TypeMatcher::matchType(
              ImpModInst->getTypeList(), ImpType.getRefType(),
              ModInst.getTypeList(), TabType.getRefType()) ||
          !matchLimit(TabLim, ImpLim)) {
        return logMatchError(ModName, ExtName, ExtType, TabType.getRefType(),
                             TabLim.hasMax(), TabLim.getMin(), TabLim.getMax(),
                             ImpType.getRefType(), ImpLim.hasMax(),
                             ImpLim.getMin(), ImpLim.getMax());
      }
      // Set the matched table address to module instance.
      ModInst.importTable(ImpInst);
      break;
    }
    case ExternalType::Memory: {
      // Get memory type. External type checked in validation.
      const auto &MemType = ImpDesc.getExternalMemoryType();
      const auto &MemLim = MemType.getLimit();
      // Import matching. External memory type should match the one in import
      // description.
      auto *ImpInst = ImpModInst->findMemoryExports(ExtName);
      const auto &ImpLim = ImpInst->getMemoryType().getLimit();
      if (!matchLimit(MemLim, ImpLim)) {
        return logMatchError(ModName, ExtName, ExtType, MemLim.hasMax(),
                             MemLim.getMin(), MemLim.getMax(), ImpLim.hasMax(),
                             ImpLim.getMin(), ImpLim.getMax());
      }
      // Set the matched memory address to module instance.
      ModInst.importMemory(ImpInst);
      break;
    }
    case ExternalType::Tag: {
      // Get tag type. External type checked in validation.
      const auto &TagType = ImpDesc.getExternalTagType();
      // Import matching.
      auto *ImpInst = ImpModInst->findTagExports(ExtName);
      if (!AST::TypeMatcher::matchType(
              ModInst.getTypeList(), TagType.getTypeIdx(),
              ImpModInst->getTypeList(), ImpInst->getTagType().getTypeIdx())) {
        const auto &ExpFuncType =
            TagType.getDefType().getCompositeType().getFuncType();
        const auto &ImpFuncType =
            ImpInst->getTagType().getDefType().getCompositeType().getFuncType();
        return logMatchError(
            ModName, ExtName, ExtType, ExpFuncType.getParamTypes(),
            ExpFuncType.getReturnTypes(), ImpFuncType.getParamTypes(),
            ImpFuncType.getReturnTypes());
      }
      ModInst.importTag(ImpInst);
      break;
    }
    case ExternalType::Global: {
      // Get global type. External type checked in validation.
      const auto &GlobType = ImpDesc.getExternalGlobalType();
      // Import matching. External global type should match the one in
      // import description.
      auto *ImpInst = ImpModInst->findGlobalExports(ExtName);
      const auto &ImpType = ImpInst->getGlobalType();
      bool IsMatch = false;
      if (ImpType.getValMut() == GlobType.getValMut()) {
        // For both const or both var: external global value type should match
        // the import global value type in description.
        IsMatch = AST::TypeMatcher::matchType(
            ModInst.getTypeList(), GlobType.getValType(),
            ImpModInst->getTypeList(), ImpType.getValType());
        if (ImpType.getValMut() == ValMut::Var) {
          // If both var: import global value type in description should also
          // match the external global value type.
          IsMatch &= AST::TypeMatcher::matchType(
              ImpModInst->getTypeList(), ImpType.getValType(),
              ModInst.getTypeList(), GlobType.getValType());
        }
      }
      if (!IsMatch) {
        return logMatchError(ModName, ExtName, ExtType, GlobType.getValType(),
                             GlobType.getValMut(), ImpType.getValType(),
                             ImpType.getValMut());
      }
      // Set the matched global address to module instance.
      ModInst.importGlobal(ImpInst);
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
