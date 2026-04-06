// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "llvm/jit.h"
#include "common/spdlog.h"

#include "data.h"
#include "llvm.h"
#include "spdlog/spdlog.h"

namespace LLVM = WasmEdge::LLVM;
using namespace std::literals;

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
  for (size_t I = 0; I < Size; ++I) {
    const std::string Name = fmt::format("{}f{}"sv, Prefix, I + Offset);
    if (auto Symbol = J->lookup<void>(Name.c_str())) {
      Result.push_back(createSymbol<void>(*Symbol));
    } else {
      if (IsLazy) {
        // in lazy JIT mode, not finding a function symbol is expected
        // since functions are compiled on demand
        spdlog::debug("[lazy-jit]: function {} not yet compiled"sv, I + Offset);
      } else {
        spdlog::error("{}"sv, Symbol.error().message().string_view());
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

Expect<void> JIT::add(Executable &Exec, Data &D) noexcept {
  auto *Lib = dynamic_cast<JITLibrary *>(&Exec);
  auto &LLModule = D.extract().LLModule;
  auto &TSContext = D.extract().getTSContext();

  auto MainJD = Lib->J->getMainJITDylib();
  if (auto Err = Lib->J->addLLVMIRModule(
          MainJD, OrcThreadSafeModule(LLModule.release(), TSContext))) {
    spdlog::error("{}"sv, Err.message().string_view());
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  return {};
}
} // namespace WasmEdge::LLVM
