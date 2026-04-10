// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "llvm/jit.h"
#include "common/spdlog.h"

#include "data.h"
#include "llvm.h"
#include "spdlog/spdlog.h"

#include <atomic>
#include <llvm-c/Error.h>
#include <llvm-c/LLJIT.h>
#include <llvm-c/Orc.h>
#include <llvm/ExecutionEngine/Orc/Core.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/Support/Error.h>

namespace LLVM = WasmEdge::LLVM;
using namespace std::literals;

namespace {

llvm::orc::LLJIT *unwrapLLJIT(LLVMOrcLLJITRef R) noexcept {
  return reinterpret_cast<llvm::orc::LLJIT *>(R);
}

llvm::orc::JITDylib *unwrapJD(LLVMOrcJITDylibRef R) noexcept {
  return reinterpret_cast<llvm::orc::JITDylib *>(R);
}

void *lookupCodeSymbol(llvm::orc::LLJIT *LJ, llvm::orc::JITDylib *MainJD,
                       const std::vector<void *> &LazyDylibs,
                       llvm::StringRef Name) noexcept {
  for (auto It = LazyDylibs.rbegin(); It != LazyDylibs.rend(); ++It) {
    auto AddrOrErr =
        LJ->lookup(*unwrapJD(static_cast<LLVMOrcJITDylibRef>(*It)), Name);
    if (AddrOrErr) {
      return (*AddrOrErr).toPtr<void *>();
    }
    llvm::consumeError(AddrOrErr.takeError());
  }
  auto AddrOrErr = LJ->lookup(*MainJD, Name);
  if (!AddrOrErr) {
    llvm::consumeError(AddrOrErr.takeError());
    return nullptr;
  }
  return (*AddrOrErr).toPtr<void *>();
}

} // namespace

namespace WasmEdge::LLVM {

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
    if (!LazyIRDylibs.empty()) {
      Addr = lookupCodeSymbol(LJ, MainJD, LazyIRDylibs, Name);
    } else if (auto Symbol = J->lookup<void>(Name.c_str())) {
      Addr = *Symbol;
    }
    if (Addr) {
      Result.push_back(createSymbol<void>(Addr));
    } else {
      if (IsLazy) {
        spdlog::debug("[lazy-jit]: function {} not yet compiled"sv, I + Offset);
      } else {
        auto LookupRes = J->lookup<void>(Name.c_str());
        if (!LookupRes) {
          spdlog::error("{}"sv, LookupRes.error().message().string_view());
        }
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

Expect<WasmFunctionCodeAddress>
JIT::add(Executable &Exec, Data &D, uint32_t GlobalFuncIndex) noexcept {
  auto *Lib = static_cast<JITLibrary *>(&Exec);
  auto &LLModule = D.extract().LLModule;
  auto &TSContext = D.extract().getTSContext();

  LLVMOrcExecutionSessionRef ES =
      LLVMOrcLLJITGetExecutionSession(Lib->J->unwrap());
  static std::atomic<unsigned> LazyJDSeq{0};
  const std::string JDName =
      fmt::format("wasmedge.lazy.{}"sv, LazyJDSeq.fetch_add(1));
  LLVMOrcJITDylibRef LazyJD = nullptr;
  if (LLVMErrorRef DylibErr =
          LLVMOrcExecutionSessionCreateJITDylib(ES, &LazyJD, JDName.c_str())) {
    char *ErrMsg = LLVMGetErrorMessage(DylibErr);
    spdlog::error("{}"sv, ErrMsg);
    LLVMDisposeErrorMessage(ErrMsg);
    LLVMConsumeError(DylibErr);
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  auto *MainJD = unwrapJD(LLVMOrcLLJITGetMainJITDylib(Lib->J->unwrap()));
  auto *LazyJDCpp = unwrapJD(LazyJD);
  MainJD->addToLinkOrder(
      *LazyJDCpp, llvm::orc::JITDylibLookupFlags::MatchExportedSymbolsOnly);
  LazyJDCpp->addToLinkOrder(
      *MainJD, llvm::orc::JITDylibLookupFlags::MatchExportedSymbolsOnly);

  if (auto Err = Lib->J->addLLVMIRModule(
          OrcJITDylib(LazyJD),
          OrcThreadSafeModule(LLModule.release(), TSContext))) {
    spdlog::error("{}"sv, Err.message().string_view());
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  Lib->LazyIRDylibs.push_back(static_cast<void *>(LazyJD));

  auto *LJ = unwrapLLJIT(Lib->J->unwrap());
  const std::string SymName =
      fmt::format("{}f{}"sv, D.getPrefix(), GlobalFuncIndex);

  auto AddrOrErr = LJ->lookup(*LazyJDCpp, SymName);
  if (!AddrOrErr) {
    spdlog::error("{}"sv, llvm::toString(AddrOrErr.takeError()));
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  return (*AddrOrErr).toPtr<void *>();
}
} // namespace WasmEdge::LLVM
