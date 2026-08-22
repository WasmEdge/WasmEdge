// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "llvm/codegen.h"

#include "common/defines.h"
#include "data.h"
#include "linker/native_linker.h"
#include "llvm.h"

#include <fstream>

namespace LLVM = WasmEdge::LLVM;
using namespace std::literals;

namespace {

using namespace WasmEdge;

#if WASMEDGE_OS_MACOS
// Get current SDK version in pair
std::pair<uint32_t, uint32_t> getSDKVersionPair() noexcept {
  // TODO: parse SDKSettings.json to get real version
  return {UINT32_C(12), UINT32_C(1)};
}
#endif

} // namespace

namespace WasmEdge::LLVM {

Expect<void> CodeGen::codegen(Span<const Byte> WasmData, Data D,
                              std::filesystem::path OutputPath) noexcept {
  // CompileFromBuffer skips the loader, so reject empty path here.
  if (OutputPath.empty()) {
    spdlog::error("output failed: empty output path"sv);
    return Unexpect(ErrCode::Value::IllegalPath);
  }

  auto LLContext = D.extract().getLLContext();
  auto &LLModule = D.extract().LLModule;
  auto &TM = D.extract().TM;

#if WASMEDGE_OS_MACOS
  {
    const auto [Major, Minor] = getSDKVersionPair();
    LLModule.addFlag(LLVMModuleFlagBehaviorError, "SDK Version"sv,
                     LLVM::Value::getConstVector32(LLContext, {Major, Minor}));
  }
#endif

  if (Conf.getCompilerConfigure().getOutputFormat() !=
      CompilerConfigure::OutputFormat::Wasm) {
    // create wasm.code and wasm.size
    auto Int32Ty = LLContext.getInt32Ty();
    auto Content = LLVM::Value::getConstString(
        LLContext,
        {reinterpret_cast<const char *>(WasmData.data()), WasmData.size()},
        true);
    LLModule.addGlobal(Content.getType(), true, LLVMExternalLinkage, Content,
                       "wasm.code");
    LLModule.addGlobal(Int32Ty, true, LLVMExternalLinkage,
                       LLVM::Value::getConstInt(Int32Ty, WasmData.size()),
                       "wasm.size");
    for (auto Fn = LLModule.getFirstFunction(); Fn; Fn = Fn.getNextFunction()) {
      if (Fn.getLinkage() == LLVMInternalLinkage) {
        Fn.setLinkage(LLVMExternalLinkage);
        Fn.setVisibility(LLVMProtectedVisibility);
        Fn.setDSOLocal(true);
        Fn.setDLLStorageClass(LLVMDLLExportStorageClass);
      }
    }
  } else {
    for (auto Fn = LLModule.getFirstFunction(); Fn; Fn = Fn.getNextFunction()) {
      if (Fn.getLinkage() == LLVMInternalLinkage) {
        Fn.setLinkage(LLVMPrivateLinkage);
        Fn.setDSOLocal(true);
        Fn.setDLLStorageClass(LLVMDefaultStorageClass);
      }
    }
  }

  // set dllexport
  for (auto GV = LLModule.getFirstGlobal(); GV; GV = GV.getNextGlobal()) {
    if (GV.getLinkage() == LLVMExternalLinkage) {
      GV.setVisibility(LLVMProtectedVisibility);
      GV.setDSOLocal(true);
      GV.setDLLStorageClass(LLVMDLLExportStorageClass);
    }
  }

  if (Conf.getCompilerConfigure().isDumpIR()) {
    if (auto ErrorMessage = LLModule.printModuleToFile("wasm.ll");
        unlikely(ErrorMessage)) {
      spdlog::error("wasm.ll open error:{}"sv, ErrorMessage.string_view());
      return WasmEdge::Unexpect(WasmEdge::ErrCode::Value::IllegalPath);
    }
  }

  spdlog::info("codegen start"sv);
  // codegen
  {
    if (Conf.getCompilerConfigure().isDumpIR()) {
      if (auto ErrorMessage = LLModule.printModuleToFile("wasm-opt.ll")) {
        // TODO:return error
        spdlog::error("printModuleToFile failed"sv);
        return Unexpect(ErrCode::Value::IllegalPath);
      }
    }

    auto [OSVec, ErrorMessage] =
        TM.emitToMemoryBuffer(LLModule, LLVMObjectFile);
    if (ErrorMessage) {
      // TODO:return error
      spdlog::error("addPassesToEmitFile failed"sv);
      return Unexpect(ErrCode::Value::IllegalPath);
    }

    if (Conf.getCompilerConfigure().isDumpIR()) {
      std::ofstream OS("wasm.o", std::ios_base::binary);
      if (!OS) {
        return Unexpect(ErrCode::Value::IllegalPath);
      }
      OS.write(OSVec.data(), static_cast<std::streamsize>(OSVec.size()));
      OS.close();
      if (!OS) {
        return Unexpect(ErrCode::Value::IllegalPath);
      }
    }

    const auto Object = Span<const Byte>(
        reinterpret_cast<const Byte *>(OSVec.data()), OSVec.size());
    auto Kind = Linker::OutputKind::UniversalWasm;
    if (Conf.getCompilerConfigure().getOutputFormat() !=
        CompilerConfigure::OutputFormat::Wasm) {
#if WASMEDGE_OS_LINUX
      Kind = Linker::OutputKind::ELF;
#elif WASMEDGE_OS_MACOS
      Kind = Linker::OutputKind::MachO;
#elif WASMEDGE_OS_WINDOWS
      Kind = Linker::OutputKind::PE;
#endif
    }
    EXPECTED_TRY(
        Linker::NativeLinker::link(Object, WasmData, OutputPath, Kind));
  }

  return {};
}

} // namespace WasmEdge::LLVM
