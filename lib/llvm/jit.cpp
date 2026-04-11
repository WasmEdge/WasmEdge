// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "llvm/jit.h"
#include "common/spdlog.h"

#include "data.h"
#include "llvm.h"
#include "spdlog/spdlog.h"

#include <mutex>
#include <llvm-c/Core.h>
#include <llvm-c/Error.h>
#include <llvm-c/LLJIT.h>
#include <llvm-c/Orc.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/Config/llvm-config.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/Linker/Linker.h>
#include <llvm/Support/Error.h>
#include <llvm/Transforms/Utils/Cloning.h>

#include <fmt/format.h>

namespace LLVM = WasmEdge::LLVM;
using namespace std::literals;

namespace {

llvm::orc::LLJIT *unwrapLLJIT(LLVMOrcLLJITRef R) noexcept {
  return reinterpret_cast<llvm::orc::LLJIT *>(R);
}

llvm::orc::JITDylib *unwrapJD(LLVMOrcJITDylibRef R) noexcept {
  return reinterpret_cast<llvm::orc::JITDylib *>(R);
}

void prepareLazyModuleForLink(llvm::Module &Cumulative,
                              llvm::Module &Lazy) noexcept {
  Lazy.setDataLayout(Cumulative.getDataLayout());
  Lazy.setTargetTriple(Cumulative.getTargetTriple());

  if (llvm::NamedMDNode *Old = Lazy.getModuleFlagsMetadata()) {
    Lazy.eraseNamedMetadata(Old);
  }
}

void pruneLazyModuleForCumulativeLink(llvm::Module &Cumulative,
                                      llvm::Module &Lazy,
                                      std::string_view Prefix,
                                      uint32_t GlobalFuncIndex,
                                      bool &NeedOverrideFromSrc) noexcept {
  const std::string KeepFn = fmt::format("{}f{}"sv, Prefix, GlobalFuncIndex);
  NeedOverrideFromSrc = false;
  if (llvm::GlobalValue *GV = Cumulative.getNamedValue(KeepFn)) {
    if (auto *OldF = llvm::dyn_cast<llvm::Function>(GV)) {
      if (OldF->use_empty()) {
        OldF->eraseFromParent();
      } else {
        NeedOverrideFromSrc = true;
      }
    }
  }
  llvm::SmallVector<llvm::Function *, 64> Remove;
  for (llvm::Function &F : llvm::make_early_inc_range(Lazy)) {
    if (!F.hasName()) {
      continue;
    }
    if (F.getName() == KeepFn) {
      continue;
    }
    if (Cumulative.getNamedValue(F.getName())) {
      Remove.push_back(&F);
    }
  }
  for (llvm::Function *F : Remove) {
    F->eraseFromParent();
  }
}

} // namespace

namespace WasmEdge::LLVM {

LazyJITState::~LazyJITState() = default;

std::unique_ptr<llvm::Module> cloneModuleForLazyJIT(Data &D) noexcept {
  auto &LM = D.extract().LLModule;
  if (!LM) {
    return nullptr;
  }
  return llvm::CloneModule(*llvm::unwrap(LM.unwrap()));
}

bool linkLazyModuleIntoCumulative(llvm::Module &Cumulative,
                                  std::unique_ptr<llvm::Module> Lazy,
                                  unsigned LinkFlags) noexcept {
  llvm::Linker L(Cumulative);
  return L.linkInModule(std::move(Lazy), LinkFlags);
}

Expect<std::shared_ptr<Executable>>
lazyJITReloadAfterLink(llvm::Module &Cumulative, Data &LazyData,
                       const Configure &Conf,
                       uint32_t GlobalFuncIndex) noexcept {
  auto &TS = LazyData.extract().getTSContext();
  const std::string_view Prefix = LazyData.getPrefix();
  LLVMModuleRef Released = LazyData.extract().LLModule.release();
  std::unique_ptr<llvm::Module> LazyMod(llvm::unwrap(Released));
  if (!LazyMod) {
    spdlog::error("[lazyjit]: missing lazy LLVM module"sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  prepareLazyModuleForLink(Cumulative, *LazyMod);
  bool NeedOverrideFromSrc = false;
  pruneLazyModuleForCumulativeLink(Cumulative, *LazyMod, Prefix,
                                   GlobalFuncIndex, NeedOverrideFromSrc);
  unsigned LinkFlags = NeedOverrideFromSrc ? llvm::Linker::OverrideFromSrc : 0;
  if (linkLazyModuleIntoCumulative(Cumulative, std::move(LazyMod), LinkFlags)) {
    spdlog::error("[lazyjit]: link lazy IR into cumulative module failed"sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  LazyData.resetModule();

  auto MergedForJit = llvm::CloneModule(Cumulative);
  LLVM::Module WMod(llvm::wrap(MergedForJit.release()));
  JIT JITEngine(Conf);
  return JITEngine.loadModule(TS, std::move(WMod), Prefix, true);
}

JITLibrary::JITLibrary(std::shared_ptr<LLVM::OrcLLJIT> JIT, std::string P,
                       bool IsLazy) noexcept
    : J(std::move(JIT)), Prefix(std::move(P)), IsLazy(IsLazy) {}

JITLibrary::~JITLibrary() noexcept {}

Symbol<const Executable::IntrinsicsTable *>
JITLibrary::getIntrinsics() noexcept {
  if (auto Symbol = J->lookup<const IntrinsicsTable *>(
          fmt::format("{}intrinsics"sv, Prefix).c_str())) {
    return createSymbol<const IntrinsicsTable *>(*Symbol);
  } else {
    spdlog::error("{}"sv, Symbol.error().message().string_view());
    return {};
  }
}

std::vector<Symbol<Executable::Wrapper>>
JITLibrary::getTypes(size_t Size) noexcept {
  std::vector<Symbol<Wrapper>> Result;
  Result.reserve(Size);
  for (size_t I = 0; I < Size; ++I) {
    const std::string Name = fmt::format("{}t{}"sv, Prefix, I);
    if (auto Symbol = J->lookup<Wrapper>(Name.c_str())) {
      Result.push_back(createSymbol<Wrapper>(*Symbol));
    } else {
      spdlog::error("{}"sv, Symbol.error().message().string_view());
      Result.emplace_back();
    }
  }

  return Result;
}

std::vector<Symbol<void>> JITLibrary::getCodes(size_t Offset,
                                               size_t Size) noexcept {
  std::vector<Symbol<void>> Result;
  Result.reserve(Size);
  auto *LJ = unwrapLLJIT(J->unwrap());
  auto *MainJD = unwrapJD(LLVMOrcLLJITGetMainJITDylib(J->unwrap()));

  for (size_t I = 0; I < Size; ++I) {
    const std::string Name = fmt::format("{}f{}"sv, Prefix, I + Offset);
    void *Addr = nullptr;
    auto AddrOrErr = LJ->lookup(*MainJD, Name);
    if (AddrOrErr) {
      Addr = (*AddrOrErr).toPtr<void *>();
    } else {
      if (!IsLazy) {
        spdlog::error("{}"sv, llvm::toString(AddrOrErr.takeError()));
      } else {
        llvm::consumeError(AddrOrErr.takeError());
      }
    }
    if (Addr) {
      Result.push_back(createSymbol<void>(Addr));
    } else {
      if (IsLazy) {
        spdlog::debug("[lazy-jit]: function {} not yet compiled"sv, I + Offset);
      }
      Result.emplace_back();
    }
  }

  return Result;
}

Expect<std::shared_ptr<Executable>> JIT::load(Data &D, bool IsLazy) noexcept {
  auto Result = OrcLLJIT::create();
  if (!Result) {
    spdlog::error("{}"sv, Result.error().message().string_view());
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto &LLModule = D.extract().LLModule;
  auto &TSContext = D.extract().getTSContext();

  if (Conf.getCompilerConfigure().isDumpIR()) {
    auto Filename = IsLazy ? "wasm-lazy-jit.ll" : "wasm-jit.ll";
    if (auto ErrorMessage = LLModule.printModuleToFile(Filename)) {
      spdlog::error("printModuleToFile failed"sv);
    }
  }

  auto MainJD = Result->getMainJITDylib();
  if (auto Err = Result->addLLVMIRModule(
          MainJD, OrcThreadSafeModule(LLModule.release(), TSContext))) {
    spdlog::error("{}"sv, Err.message().string_view());
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  return std::make_shared<JITLibrary>(
      std::make_shared<OrcLLJIT>(std::move(*Result)),
      std::string(D.getPrefix()), IsLazy);
}

Expect<std::shared_ptr<Executable>>
JIT::loadModule(OrcThreadSafeContext &TSContext, Module &&LLModule,
                std::string_view Prefix, bool IsLazy) noexcept {
  auto Result = OrcLLJIT::create();
  if (!Result) {
    spdlog::error("{}"sv, Result.error().message().string_view());
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  if (Conf.getCompilerConfigure().isDumpIR()) {
    auto Filename = IsLazy ? "wasm-lazy-jit.ll" : "wasm-jit.ll";
    if (auto ErrorMessage = LLModule.printModuleToFile(Filename)) {
      spdlog::error("printModuleToFile failed"sv);
    }
  }

  auto MainJD = Result->getMainJITDylib();
  if (auto Err = Result->addLLVMIRModule(
          MainJD, OrcThreadSafeModule(std::move(LLModule), TSContext))) {
    spdlog::error("{}"sv, Err.message().string_view());
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  return std::make_shared<JITLibrary>(
      std::make_shared<OrcLLJIT>(std::move(*Result)), std::string(Prefix),
      IsLazy);
}

Expect<WasmFunctionCodeAddress> JIT::add(Executable &Exec, Data &D,
                                         uint32_t GlobalFuncIndex) noexcept {
  auto *Lib = dynamic_cast<JITLibrary *>(&Exec);
  if (!Lib) {
    spdlog::error("JIT::add: executable is not a JITLibrary"sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  std::lock_guard<std::mutex> Lock(Lib->LazyAddMutex);
  auto &LLModule = D.extract().LLModule;
  auto &TSContext = D.extract().getTSContext();

  LLVMOrcJITDylibRef MainJDRef = LLVMOrcLLJITGetMainJITDylib(Lib->J->unwrap());
  auto *MainJD = unwrapJD(MainJDRef);
  auto *LJ = unwrapLLJIT(Lib->J->unwrap());

  if (auto Err = Lib->J->addLLVMIRModule(
          OrcJITDylib(MainJDRef),
          OrcThreadSafeModule(LLModule.release(), TSContext))) {
    spdlog::error("{}"sv, Err.message().string_view());
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const std::string SymName =
      fmt::format("{}f{}"sv, D.getPrefix(), GlobalFuncIndex);

  auto AddrOrErr = LJ->lookup(*MainJD, SymName);
  if (!AddrOrErr) {
    spdlog::error("{}"sv, llvm::toString(AddrOrErr.takeError()));
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  return (*AddrOrErr).toPtr<void *>();
}
} // namespace WasmEdge::LLVM
