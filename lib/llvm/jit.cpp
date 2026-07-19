// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "llvm/jit.h"

#include "data.h"
#include "llvm.h"
#include "spdlog/spdlog.h"

#include <mutex>

#include <fmt/format.h>

namespace LLVM = WasmEdge::LLVM;
using namespace std::literals;

namespace {

std::string errorToString(LLVM::Error &&E) noexcept {
  auto Msg = E.message();
  return std::string(Msg.string_view());
}

#if LLVM_VERSION_MAJOR >= 13 &&                                                \
    (defined(__SANITIZE_THREAD__) ||                                           \
     (defined(__has_feature) && __has_feature(thread_sanitizer)))
// The host itself is built with ThreadSanitizer, so instrument the generated
// code too: races INSIDE compiled code (e.g. a GC root-publish store) are
// otherwise invisible, because -fsanitize=thread only instruments the C++ it
// compiles, never the code LLVM emits at runtime. LLVM's TSan pass only touches
// functions carrying the sanitize_thread attribute, so add it to every module
// function, then run the function TSan pass. The module pass is deliberately
// left out: it only appends a __tsan_init module constructor, which the host
// has already run, and every JIT'd module carrying one collides on ORC's
// generated initializer symbol.
//
// This belongs to the JIT path only. The emitted __tsan_* calls are undefined
// symbols that ORC resolves against the libtsan already linked into this host.
// An AOT artifact has no such resolution step: Loader::AOTSection copies the
// linked object's raw sections into an anonymous mapping and never processes
// relocations, so instrumented AOT code would call through an unrelocated
// PLT/GOT slot and jump to a link-time address.
void instrumentThreadSanitizer(LLVM::Module &LLModule) noexcept {
  // Pure C API to avoid any Context-wrapper lifetime concern: the module's
  // context is borrowed, not owned here.
  LLVMContextRef Cx = LLVMGetModuleContext(LLModule.unwrap());
  const unsigned int STKind =
      LLVMGetEnumAttributeKindForName("sanitize_thread", 15);
  LLVMAttributeRef STAttr = LLVMCreateEnumAttribute(Cx, STKind, 0);
  for (auto Fn = LLModule.getFirstFunction(); Fn; Fn = Fn.getNextFunction()) {
    LLVMAddAttributeAtIndex(
        Fn.unwrap(),
        static_cast<LLVMAttributeIndex>(LLVMAttributeFunctionIndex), STAttr);
  }
  auto PBO = LLVM::PassBuilderOptions::create();
  if (auto Error = PBO.runPasses(LLModule, "function(tsan)")) {
    spdlog::error("{}"sv, Error.message().string_view());
  }
}
#else
void instrumentThreadSanitizer(LLVM::Module &) noexcept {}
#endif

static WasmEdge::Expect<LLVM::OrcLLJIT> createTunedLazyLLJIT() noexcept {
  LLVMOrcLLJITBuilderRef Builder = LLVM::OrcLLJIT::getBuilder();
  if (!Builder) {
    Builder = LLVMOrcCreateLLJITBuilder();
  }

  LLVMTargetRef TheTarget = nullptr;
  LLVM::Message Triple(LLVMGetDefaultTargetTriple());
  char *TripleErr = nullptr;
  if (LLVMGetTargetFromTriple(Triple.string_view().data(), &TheTarget,
                              &TripleErr)) {
    spdlog::error("[lazy-jit]: getTargetFromTriple failed: {}"sv,
                  TripleErr ? TripleErr : "");
    LLVMDisposeMessage(TripleErr);
    LLVMOrcDisposeLLJITBuilder(Builder);
    return WasmEdge::Unexpect(WasmEdge::ErrCode::Value::LazyCompilationError);
  }

  LLVM::Message CPU(LLVMGetHostCPUName());
  LLVM::Message Features(LLVMGetHostCPUFeatures());

  // LLVMCodeGenLevelNone is used deliberately here as a performance trade-off.
  // ORC's IRCompileLayer inherits the codegen optimisation level from this
  // target machine and applies it during every lazy materialisation.
  // Note that this codegen level is independent of the optimisation
  // level passed through WasmEdge::Configure, which governs the
  // IR compilation step, this setting only controls the ORC materialisation
  // pipeline for lazy JIT.
  LLVMTargetMachineRef TM = LLVMCreateTargetMachine(
      TheTarget, Triple.string_view().data(), CPU.string_view().data(),
      Features.string_view().data(), LLVMCodeGenLevelNone, LLVMRelocDefault,
      LLVMCodeModelJITDefault);

  if (!TM) {
    spdlog::error("[lazy-jit]: createTargetMachine failed"sv);
    LLVMOrcDisposeLLJITBuilder(Builder);
    return WasmEdge::Unexpect(WasmEdge::ErrCode::Value::LazyCompilationError);
  }

  LLVMOrcJITTargetMachineBuilderRef JTMB =
      LLVMOrcJITTargetMachineBuilderCreateFromTargetMachine(TM);
  LLVMOrcLLJITBuilderSetJITTargetMachineBuilder(Builder, JTMB);

  LLVM::OrcLLJIT Result;
  if (LLVMErrorRef CreateErr = LLVMOrcCreateLLJIT(&Result.unwrap(), Builder)) {
    LLVM::ErrorMessage Msg(LLVMGetErrorMessage(CreateErr));
    spdlog::error("[lazy-jit]: LLVMOrcCreateLLJIT failed: {}"sv,
                  Msg.string_view());
    return WasmEdge::Unexpect(WasmEdge::ErrCode::Value::LazyCompilationError);
  }
  return Result;
}

} // namespace

namespace WasmEdge::LLVM {

JITLibrary::JITLibrary(std::shared_ptr<LLVM::OrcLLJIT> JIT,
                       bool IsLazy) noexcept
    : J(std::move(JIT)), IsLazy(IsLazy) {}

JITLibrary::~JITLibrary() noexcept {}

Symbol<const Executable::IntrinsicsTable *>
JITLibrary::getIntrinsics() noexcept {
  if (auto Symbol = J->lookup<const IntrinsicsTable *>("intrinsics")) {
    return createSymbol<const IntrinsicsTable *>(*Symbol);
  } else {
    spdlog::error("failed to lookup intrinsics symbol: {}"sv,
                  errorToString(std::move(Symbol.error())));
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
      spdlog::error("failed to lookup table symbol {}: {}"sv, Name,
                    errorToString(std::move(Symbol.error())));
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
    void *Addr = nullptr;
    auto AddrOrErr = J->lookup<void *>(Name.c_str());
    if (AddrOrErr) {
      Addr = *AddrOrErr;
    } else {
      auto ErrMsg = errorToString(std::move(AddrOrErr.error()));
      // consuming only symbol not found errors for lazy-jit
      if (!IsLazy || ErrMsg.find("Symbols not found"sv) == std::string::npos) {
        spdlog::error("failed to lookup function symbol {}: {}"sv, Name,
                      ErrMsg);
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

Expect<std::shared_ptr<Executable>> JIT::load(Data D) noexcept {
  return loadImpl(D, false);
}

Expect<std::shared_ptr<Executable>> JIT::loadLazy(Data &D) noexcept {
  return loadImpl(D, true);
}

Expect<std::shared_ptr<Executable>> JIT::loadImpl(Data &D,
                                                  bool IsLazy) noexcept {
  OrcLLJIT LLJITInstance;
  if (IsLazy) {
    auto R = createTunedLazyLLJIT();
    if (!R) {
      spdlog::error("[lazy-jit]: failed to create LLJIT"sv);
      return Unexpect(R.error());
    }
    LLJITInstance = std::move(*R);
  } else {
    auto R = OrcLLJIT::create();
    if (!R) {
      spdlog::error("failed to create LLJIT: {}"sv,
                    R.error().message().string_view());
      return Unexpect(ErrCode::Value::HostFuncError);
    }
    LLJITInstance = std::move(*R);
  }

  auto &LLModule = D.extract().LLModule;
  auto &TSContext = D.extract().getTSContext();

  if (Conf.getCompilerConfigure().isDumpIR()) {
    const auto *Filename = IsLazy ? "wasm-lazy-jit.ll" : "wasm-jit.ll";
    if (auto ErrorMessage = LLModule.printModuleToFile(Filename)) {
      spdlog::error("printModuleToFile failed"sv);
    }
  }

  instrumentThreadSanitizer(LLModule);

  auto MainJD = LLJITInstance.getMainJITDylib();
  if (auto Err = LLJITInstance.addLLVMIRModule(
          MainJD, OrcThreadSafeModule(LLModule.release(), TSContext))) {
    spdlog::error("failed to add LLVM IR module: {}"sv,
                  Err.message().string_view());
    return Unexpect(ErrCode::Value::HostFuncError);
  }

  return std::make_shared<JITLibrary>(
      std::make_shared<OrcLLJIT>(std::move(LLJITInstance)), IsLazy);
}

Expect<std::vector<WasmFunctionCodeAddress>>
JIT::add(JITLibrary &Lib, Data &D,
         Span<const uint32_t> GlobalFuncIndices) noexcept {
  if (GlobalFuncIndices.empty()) {
    spdlog::error("JIT::add: empty function index list"sv);
    return Unexpect(ErrCode::Value::LazyCompilationError);
  }

  auto &LLModule = D.extract().LLModule;
  auto &TSContext = D.extract().getTSContext();

  instrumentThreadSanitizer(LLModule);

  auto JD = Lib.J->getMainJITDylib();
  auto RT = JD.createResourceTracker();
  if (!RT) {
    spdlog::error("[lazy-jit]: failed to create resource tracker"sv);
    return Unexpect(ErrCode::Value::LazyCompilationError);
  }

  if (auto Err = Lib.J->addLLVMIRModuleWithRT(
          RT, OrcThreadSafeModule(LLModule.release(), TSContext))) {
    spdlog::error("[lazy-jit]: failed to add LLVM IR module: {}"sv,
                  Err.message().string_view());
    return Unexpect(ErrCode::Value::LazyCompilationError);
  }

  std::vector<WasmFunctionCodeAddress> Addresses;
  Addresses.reserve(GlobalFuncIndices.size());
  for (uint32_t GlobalFuncIndex : GlobalFuncIndices) {
    const std::string SymName = fmt::format("f{}"sv, GlobalFuncIndex);
    auto AddrOrErr = Lib.J->lookup<void *>(SymName.c_str());
    if (!AddrOrErr) {
      spdlog::error("[lazy-jit]: failed to lookup function symbol {}: {}"sv,
                    SymName, errorToString(std::move(AddrOrErr.error())));
      if (auto RemoveErr = RT.remove()) {
        spdlog::error(
            "[lazy-jit]: failed to remove failed module from tracker: {}"sv,
            RemoveErr.message().string_view());
      }
      return Unexpect(ErrCode::Value::LazyCompilationError);
    }
    Addresses.push_back(*AddrOrErr);
  }
  return Addresses;
}
} // namespace WasmEdge::LLVM
