// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/executor.h"

#include "common/errinfo.h"
#include "common/spdlog.h"
#include "runtime/hostfunc.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace std::literals;

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

/// The module names imported from each official plugin.
constexpr std::array<std::pair<std::string_view, std::string_view>, 22>
    OfficialPluginModules{{
        {"wasi_ephemeral_crypto_common"sv, "wasi_crypto"sv},
        {"wasi_ephemeral_crypto_asymmetric_common"sv, "wasi_crypto"sv},
        {"wasi_ephemeral_crypto_kx"sv, "wasi_crypto"sv},
        {"wasi_ephemeral_crypto_signatures"sv, "wasi_crypto"sv},
        {"wasi_ephemeral_crypto_symmetric"sv, "wasi_crypto"sv},
        {"wasi:logging/logging"sv, "wasi_logging"sv},
        {"wasi_ephemeral_nn"sv, "wasi_nn"sv},
        {"wasm_bpf"sv, "wasm_bpf"sv},
        {"wasmedge_ffmpeg_avcodec"sv, "wasmedge_ffmpeg"sv},
        {"wasmedge_ffmpeg_avdevice"sv, "wasmedge_ffmpeg"sv},
        {"wasmedge_ffmpeg_avfilter"sv, "wasmedge_ffmpeg"sv},
        {"wasmedge_ffmpeg_avformat"sv, "wasmedge_ffmpeg"sv},
        {"wasmedge_ffmpeg_avutil"sv, "wasmedge_ffmpeg"sv},
        {"wasmedge_ffmpeg_swresample"sv, "wasmedge_ffmpeg"sv},
        {"wasmedge_ffmpeg_swscale"sv, "wasmedge_ffmpeg"sv},
        {"wasmedge_image"sv, "wasmedge_image"sv},
        {"wasmedge_ocr"sv, "wasmedge_ocr"sv},
        {"wasmedge_opencvmini"sv, "wasmedge_opencvmini"sv},
        {"wasmedge_stablediffusion"sv, "wasmedge_stablediffusion"sv},
        {"wasmedge_tensorflow"sv, "wasmedge_tensorflow"sv},
        {"wasmedge_tensorflowlite"sv, "wasmedge_tensorflowlite"sv},
        {"wasmedge_zlib"sv, "wasmedge_zlib"sv},
    }};

/// Get the official plugin that provides the module, or an empty name.
std::string_view getOfficialPluginName(std::string_view ModName) noexcept {
  for (const auto &[Name, PluginName] : OfficialPluginModules) {
    if (Name == ModName) {
      return PluginName;
    }
  }
  return {};
}

/// Host function that stands in for an import of an uninstalled plugin, which
/// reports the plugin and traps.
class PluginMockFunction : public Runtime::HostFunctionBase {
public:
  PluginMockFunction(std::string_view PluginName, std::string_view ModName,
                     std::string_view FuncName, const AST::FunctionType &Type)
      : HostFunctionBase(0), PluginName(PluginName), ModName(ModName),
        FuncName(FuncName) {
    DefType.getCompositeType().getFuncType() = Type;
  }

  Expect<void> run(const Runtime::CallingFrame &, Span<const ValVariant>,
                   Span<ValVariant>) override {
    spdlog::error("Calling \"{}\" \"{}\" requires the {} plugin, which is not "
                  "installed. Please install the plugin and restart "
                  "WasmEdge."sv,
                  ModName, FuncName, PluginName);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

private:
  std::string_view PluginName;
  std::string ModName;
  std::string FuncName;
};

auto logUnknownError(std::string_view ModName, std::string_view ExtName,
                     ExternalType ExtType) {
  spdlog::error(ErrCode::Value::UnknownImport);
  spdlog::error(ErrInfo::InfoLinking(ModName, ExtName, ExtType));
  spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Desc_Import));
  return Unexpect(ErrCode::Value::UnknownImport);
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
    assumingUnreachable();
  }

  // Check for error external types or unknown imports.
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
Expect<void> Executor::instantiate(
    std::function<const Runtime::Instance::ModuleInstance *(std::string_view)>
        ModuleFinder,
    Runtime::Instance::ModuleInstance &ModInst,
    const AST::ImportSection &ImportSec) {
  // The plugin modules whose imports are stood in for, to warn only once.
  std::vector<std::string_view> MockedModNames;
  // Iterate and instantiate import descriptions.
  for (const auto &ImpDesc : ImportSec.getContent()) {
    // Get data from import description and find import module.
    auto ExtType = ImpDesc.getExternalType();
    auto ModName = ImpDesc.getModuleName();
    auto ExtName = ImpDesc.getExternalName();
    const auto *ImpModInst = ModuleFinder(ModName);
    if (unlikely(ImpModInst == nullptr)) {
      const auto PluginName = getOfficialPluginName(ModName);
      if (!PluginName.empty() && ExtType == ExternalType::Function) {
        // Stand in for the function of an uninstalled official plugin, and
        // remind the user once per module.
        if (std::find(MockedModNames.begin(), MockedModNames.end(), ModName) ==
            MockedModNames.end()) {
          spdlog::warn("The {} plugin, which provides module \"{}\", is not "
                       "installed, so calling the functions imported from it "
                       "will fail. Please install the plugin."sv,
                       PluginName, ModName);
          MockedModNames.push_back(ModName);
        }
        const uint32_t TypeIdx = ImpDesc.getExternalFuncTypeIdx();
        ModInst.addFunc(TypeIdx, std::unique_ptr<Runtime::HostFunctionBase>(
                                     std::make_unique<PluginMockFunction>(
                                         PluginName, ModName, ExtName,
                                         (*ModInst.getType(TypeIdx))
                                             ->getCompositeType()
                                             .getFuncType())));
        continue;
      }
      auto Res = logUnknownError(ModName, ExtName, ExtType);
      if (ModName == "wasi_snapshot_preview1"sv) {
        spdlog::error("    This is a WASI related import. Please ensure that "
                      "you've turned on the WASI configuration."sv);
      } else if (!PluginName.empty()) {
        spdlog::error("    This is an import of the {} plugin. Please install "
                      "the plugin."sv,
                      PluginName);
      } else if (ModName == "env"sv) {
        spdlog::error(
            "    This may be the import of host environment like JavaScript or "
            "Golang. Please check that you've registered the necessary host "
            "modules from the host programming language."sv);
      }
      return Res;
    }
    EXPECTED_TRY(checkImportMatched(ModName, ExtName, ExtType, *ImpModInst));

    // Add the imports to the module instance.
    switch (ExtType) {
    case ExternalType::Function: {
      // Get the function type index. The external type is checked in
      // validation.
      uint32_t TypeIdx = ImpDesc.getExternalFuncTypeIdx();
      // Import matching.
      auto *ImpInst = ImpModInst->findFuncExports(ExtName);
      // Read the type list from the function's owning module (so an alias
      // re-exporting a foreign func matches against the original's types).
      auto GetImpTypeList = [&ImpModInst](const auto *Inst) {
        return Inst->getModule() ? Inst->getModule()->getTypeList()
                                 : ImpModInst->getTypeList();
      };

      if (!AST::TypeMatcher::matchType(ModInst.getTypeList(), TypeIdx,
                                       GetImpTypeList(ImpInst),
                                       ImpInst->getTypeIndex())) {
        const auto &ExpDefType = **ModInst.getType(TypeIdx);
        bool IsMatchV2 = false;
        const auto &ExpFuncType = ExpDefType.getCompositeType().getFuncType();
        const auto &ImpFuncType = ImpInst->getFuncType();
        if (ModName == "wasi_snapshot_preview1"sv) {
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
          std::vector<std::string_view> CompatibleWASISocketAPI = {
              "sock_open"sv,         "sock_bind"sv,       "sock_connect"sv,
              "sock_listen"sv,       "sock_accept"sv,     "sock_recv"sv,
              "sock_recv_from"sv,    "sock_send"sv,       "sock_send_to"sv,
              "sock_getlocaladdr"sv, "sock_getpeeraddr"sv};
          for (auto Iter = CompatibleWASISocketAPI.begin();
               Iter != CompatibleWASISocketAPI.end(); Iter++) {
            if (ExtName == *Iter) {
              auto *ImpInstV2 =
                  ImpModInst->findFuncExports(std::string(*Iter) + "_v2");
              if (ImpInstV2 != nullptr &&
                  AST::TypeMatcher::matchType(ModInst.getTypeList(), TypeIdx,
                                              GetImpTypeList(ImpInstV2),
                                              ImpInstV2->getTypeIndex())) {
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
      // Set the matched function address in the module instance.
      ModInst.importFunction(ImpInst);

      // If the imported function is a WASI function, mark it in the module.
      if (!ModInst.getWASIModule() && ModName == "wasi_snapshot_preview1"sv) {
        ModInst.setWASIModule(ImpModInst);
      }
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
          !AST::TypeMatcher::matchLimit(TabLim, ImpLim)) {
        return logMatchError(ModName, ExtName, ExtType, TabType.getRefType(),
                             TabLim.hasMax(), TabLim.getMin(), TabLim.getMax(),
                             ImpType.getRefType(), ImpLim.hasMax(),
                             ImpLim.getMin(), ImpLim.getMax());
      }
      // Set the matched table address in the module instance.
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
      if (!AST::TypeMatcher::matchLimit(MemLim, ImpLim)) {
        return logMatchError(ModName, ExtName, ExtType, MemLim.hasMax(),
                             MemLim.getMin(), MemLim.getMax(), ImpLim.hasMax(),
                             ImpLim.getMin(), ImpLim.getMax());
      }
      // Set the matched memory address in the module instance.
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
      // Set the matched global address in the module instance.
      ModInst.importGlobal(ImpInst);
      break;
    }
    default:
      assumingUnreachable();
    }
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
