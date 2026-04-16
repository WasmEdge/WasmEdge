// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "llvm/jit.h"

#include "data.h"
#include "llvm.h"
#include "spdlog/spdlog.h"

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
#include <mutex>

#include <fmt/format.h>

namespace LLVM = WasmEdge::LLVM;
using namespace std::literals;

namespace {

std::string errorToString(LLVM::Error &&E) noexcept {
  auto Msg = E.message();
  return std::string(Msg.string_view());
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
    spdlog::error("{}"sv, errorToString(std::move(Symbol.error())));
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
      spdlog::error("{}"sv, errorToString(std::move(Symbol.error())));
      Result.emplace_back();
    }
  }

  return Result;
}

std::vector<Symbol<void>> JITLibrary::getCodes(size_t Offset,
                                               size_t Size) noexcept {
  std::vector<Symbol<void>> Result;
  Result.reserve(Size);
  for (size_t I = 0; I < Size; ++I) {
    const std::string Name = fmt::format("{}f{}"sv, Prefix, I + Offset);
    void *Addr = nullptr;
    auto AddrOrErr = J->lookup<void *>(Name.c_str());
    if (AddrOrErr) {
      Addr = *AddrOrErr;
    } else {
      if (!IsLazy) {
        spdlog::error("{}"sv, errorToString(std::move(AddrOrErr.error())));
      } else {
        // Just consume the error if it's lazy (it might not be compiled yet)
        (void)AddrOrErr.error();
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
    const auto *Filename = IsLazy ? "wasm-lazy-jit.ll" : "wasm-jit.ll";
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

Expect<WasmFunctionCodeAddress> JIT::add(Executable &Exec, Data &D,
                                         uint32_t GlobalFuncIndex) noexcept {
  auto *Lib = static_cast<JITLibrary *>(&Exec);
  if (!Lib) {
    spdlog::error("JIT::add: executable is not a JITLibrary"sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  std::lock_guard<std::mutex> Lock(Lib->LazyAddMutex);
  auto &LLModule = D.extract().LLModule;
  auto &TSContext = D.extract().getTSContext();

  LLVMOrcJITDylibRef MainJDRef = LLVMOrcLLJITGetMainJITDylib(Lib->J->unwrap());

  if (auto Err = Lib->J->addLLVMIRModule(
          OrcJITDylib(MainJDRef),
          OrcThreadSafeModule(LLModule.release(), TSContext))) {
    spdlog::error("{}"sv, Err.message().string_view());
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  const std::string SymName =
      fmt::format("{}f{}"sv, D.getPrefix(), GlobalFuncIndex);

  auto AddrOrErr = Lib->J->lookup<void *>(SymName.c_str());
  if (!AddrOrErr) {
    spdlog::error("{}"sv, errorToString(std::move(AddrOrErr.error())));
    return Unexpect(ErrCode::Value::HostFuncError);
  }
  return *AddrOrErr;
}
} // namespace WasmEdge::LLVM
