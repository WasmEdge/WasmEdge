// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "llvm/jit.h"
#include "common/spdlog.h"

#include "data.h"
#include "llvm.h"

namespace LLVM = WasmEdge::LLVM;
using namespace std::literals;

namespace WasmEdge::LLVM {

JITLibrary::JITLibrary(OrcLLJIT JIT) noexcept
    : J(std::make_unique<OrcLLJIT>(std::move(JIT)).release()) {}

JITLibrary::~JITLibrary() noexcept {
  std::unique_ptr<OrcLLJIT> JIT(std::exchange(J, nullptr));
}

Symbol<const Executable::IntrinsicsTable *>
JITLibrary::getIntrinsics() noexcept {
  if (auto Symbol = J->lookup<const IntrinsicsTable *>("intrinsics")) {
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
    const std::string Name = fmt::format("t{}"sv, I);
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
    const std::string Name = fmt::format("f{}"sv, I + Offset);
    if (auto Symbol = J->lookup<void>(Name.c_str())) {
      Result.push_back(createSymbol<void>(*Symbol));
    } else {
      spdlog::error("{}"sv, Symbol.error().message().string_view());
      Result.emplace_back();
    }
  }

  return Result;
}

Expect<std::shared_ptr<Executable>> JIT::load(Data D) noexcept {
  OrcLLJIT J;
  if (auto Res = OrcLLJIT::create(); !Res) {
    spdlog::error("{}"sv, Res.error().message().string_view());
    return Unexpect(ErrCode::Value::HostFuncError);
  } else {
    J = std::move(*Res);
  }

  auto &LLModule = D.extract().LLModule;

  if (Conf.getCompilerConfigure().isDumpIR()) {
    if (auto ErrorMessage = LLModule.printModuleToFile("wasm-jit.ll")) {
      spdlog::error("printModuleToFile failed");
    }
  }

  auto MainJD = J.getMainJITDylib();
  if (auto Err = J.addLLVMIRModule(
          MainJD,
          OrcThreadSafeModule(LLModule.release(), D.extract().TSContext))) {
    spdlog::error("{}"sv, Err.message().string_view());
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  return std::make_shared<JITLibrary>(std::move(J));
}
} // namespace WasmEdge::LLVM
