// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "vm/vm.h"

#include "ast/instruction.h"
#include "ast/module.h"
#include "common/enum_errcode.hpp"
#include "common/errcode.h"
#include "common/types.h"
#include "host/wasi/wasimodule.h"
#include "plugin/plugin.h"
#include "llvm/compiler.h"
#include "llvm/jit.h"

#include "host/mock/wasi_crypto_module.h"
#include "host/mock/wasi_logging_module.h"
#include "host/mock/wasi_nn_module.h"
#include "host/mock/wasmedge_image_module.h"
#include "host/mock/wasmedge_process_module.h"
#include "host/mock/wasmedge_stablediffusion_module.h"
#include "host/mock/wasmedge_tensorflow_module.h"
#include "host/mock/wasmedge_tensorflowlite_module.h"
#include "validator/validator.h"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <variant>
#include <vector>

#ifdef WASMEDGE_USE_LLVM
#include <llvm/IR/Module.h>
#endif

namespace WasmEdge {
namespace VM {

namespace {

#ifdef WASMEDGE_USE_LLVM
void collectLazyCallGraphBatch(uint32_t LocalSeed, const AST::Module *ModulePtr,
                               uint32_t ImportFuncCount,
                               const std::unordered_set<uint32_t> &LazyCompiled,
                               std::vector<uint32_t> &OutSortedLocals) {
  OutSortedLocals.clear();
  if (!ModulePtr) {
    return;
  }
  const auto &CodeSec = ModulePtr->getCodeSection().getContent();
  const uint32_t DefinedCount = static_cast<uint32_t>(CodeSec.size());

  if (LocalSeed >= DefinedCount || LazyCompiled.count(LocalSeed)) {
    return;
  }

  std::vector<uint8_t> Visited(DefinedCount, 0);
  std::vector<uint32_t> Stack;
  Stack.reserve(64);

  Visited[LocalSeed] = 1;
  Stack.push_back(LocalSeed);
  OutSortedLocals.push_back(LocalSeed);

  while (!Stack.empty()) {
    const uint32_t L = Stack.back();
    Stack.pop_back();

    for (const auto &Instr : CodeSec[L].getExpr().getInstrs()) {
      const auto Op = Instr.getOpCode();
      if (Op == OpCode::Call || Op == OpCode::Return_call ||
          Op == OpCode::Ref__func) {
        const uint32_t Target = Instr.getTargetIndex();
        if (Target >= ImportFuncCount) {
          const uint32_t LocalIdx = Target - ImportFuncCount;
          if (LocalIdx < DefinedCount && !Visited[LocalIdx] &&
              !LazyCompiled.count(LocalIdx)) {
            Visited[LocalIdx] = 1;
            Stack.push_back(LocalIdx);
            OutSortedLocals.push_back(LocalIdx);
          }
        }
      }
    }
  }

  std::sort(OutSortedLocals.begin(), OutSortedLocals.end());
}
#endif

template <typename T> struct VisitUnit {
  using MT = std::function<T(std::unique_ptr<AST::Module> &)>;
  using CT = std::function<T(std::unique_ptr<AST::Component::Component> &)>;

  VisitUnit(MT F, CT G) : VisitMod{F}, VisitComp{G} {}
  T operator()(std::unique_ptr<AST::Module> &Mod) const {
    return VisitMod(Mod);
  }
  T operator()(std::unique_ptr<AST::Component::Component> &Comp) const {
    return VisitComp(Comp);
  }

private:
  MT VisitMod;
  CT VisitComp;
};

template <typename T>
std::unique_ptr<Runtime::Instance::ModuleInstance>
createPluginModule(std::string_view PName, std::string_view MName) {
  using namespace std::literals::string_view_literals;
  if (const auto *Plugin = Plugin::Plugin::find(PName)) {
    if (const auto *Module = Plugin->findModule(MName)) {
      return Module->create();
    }
  }
  spdlog::debug("Plugin: {} , module name: {} not found. Mock instead."sv,
                PName, MName);
  return std::make_unique<T>();
}
} // namespace

VM::VM(const Configure &Conf)
    : Conf(Conf), Stage(VMStage::Inited),
      LoaderEngine(Conf, &Executor::Executor::Intrinsics),
      ValidatorEngine(Conf), ExecutorEngine(Conf, &Stat),
      Store(std::make_unique<Runtime::StoreManager>()), StoreRef(*Store.get()) {
  unsafeInitVM();
}

VM::VM(const Configure &Conf, Runtime::StoreManager &S)
    : Conf(Conf), Stage(VMStage::Inited),
      LoaderEngine(Conf, &Executor::Executor::Intrinsics),
      ValidatorEngine(Conf), ExecutorEngine(Conf, &Stat), StoreRef(S) {
  unsafeInitVM();
}

void VM::unsafeInitVM() {
  // Load the built-in modules and the plug-ins.
  unsafeLoadBuiltInHosts();
  unsafeLoadPlugInHosts();

  // Register the lazy compilation callback if lazy JIT mode is enabled.
#ifdef WASMEDGE_USE_LLVM
  if (Conf.getRuntimeConfigure().getRunMode() == RunMode::LazyJIT) {
    spdlog::warn(
        "Lazy JIT is an alpha and experimental feature, which is not ready for production use."sv);
    ExecutorEngine.registerLazyCompilationCallback(
        [this](const std::string &ID, const uint32_t FuncIdx) -> Expect<void> {
          return lazyCompileFunctions(ID, FuncIdx);
        });
  }
#endif

  // Register all module instances.
  unsafeRegisterBuiltInHosts();
  unsafeRegisterPlugInHosts();
}

void VM::unsafeLoadBuiltInHosts() {
  // Load the built-in host modules from configuration.
  // TODO: This will be extended for versioned WASI in the future.
  cleanupModInstContainer(BuiltInModInsts);
  if (Conf.hasHostRegistration(HostRegistration::Wasi)) {
    std::unique_ptr<Runtime::Instance::ModuleInstance> WasiMod =
        std::make_unique<Host::WasiModule>();
    BuiltInModInsts.insert({HostRegistration::Wasi, std::move(WasiMod)});
  }
}

void VM::unsafeLoadPlugInHosts() {
  // Load the plugins and mock them if not found.
  using namespace std::literals::string_view_literals;
  cleanupModInstContainer(PlugInModInsts);

  PlugInModInsts.push_back(
      createPluginModule<Host::WasiNNModuleMock>("wasi_nn"sv, "wasi_nn"sv));
  PlugInModInsts.push_back(createPluginModule<Host::WasiCryptoCommonModuleMock>(
      "wasi_crypto"sv, "wasi_crypto_common"sv));
  PlugInModInsts.push_back(
      createPluginModule<Host::WasiCryptoAsymmetricCommonModuleMock>(
          "wasi_crypto"sv, "wasi_crypto_asymmetric_common"sv));
  PlugInModInsts.push_back(createPluginModule<Host::WasiCryptoKxModuleMock>(
      "wasi_crypto"sv, "wasi_crypto_kx"sv));
  PlugInModInsts.push_back(
      createPluginModule<Host::WasiCryptoSignaturesModuleMock>(
          "wasi_crypto"sv, "wasi_crypto_signatures"sv));
  PlugInModInsts.push_back(
      createPluginModule<Host::WasiCryptoSymmetricModuleMock>(
          "wasi_crypto"sv, "wasi_crypto_symmetric"sv));
  PlugInModInsts.push_back(createPluginModule<Host::WasmEdgeProcessModuleMock>(
      "wasmedge_process"sv, "wasmedge_process"sv));
  PlugInModInsts.push_back(createPluginModule<Host::WasiLoggingModuleMock>(
      "wasi_logging"sv, "wasi:logging/logging"sv));
  PlugInModInsts.push_back(
      createPluginModule<Host::WasmEdgeTensorflowModuleMock>(
          "wasmedge_tensorflow"sv, "wasmedge_tensorflow"sv));
  PlugInModInsts.push_back(
      createPluginModule<Host::WasmEdgeTensorflowLiteModuleMock>(
          "wasmedge_tensorflowlite"sv, "wasmedge_tensorflowlite"sv));
  PlugInModInsts.push_back(createPluginModule<Host::WasmEdgeImageModuleMock>(
      "wasmedge_image"sv, "wasmedge_image"sv));
  PlugInModInsts.push_back(
      createPluginModule<Host::WasmEdgeStableDiffusionModuleMock>(
          "wasmedge_stablediffusion"sv, "wasmedge_stablediffusion"sv));

  // Load the other non-official plugins.
  for (const auto &Plugin : Plugin::Plugin::plugins()) {
    if (Conf.isForbiddenPlugins(Plugin.name())) {
      continue;
    }
    // Skip wasi_crypto, wasi_nn, wasi_logging, WasmEdge_Process,
    // WasmEdge_Tensorflow, WasmEdge_TensorflowLite, and WasmEdge_Image.
    if (Plugin.name() == "wasi_crypto"sv || Plugin.name() == "wasi_nn"sv ||
        Plugin.name() == "wasi_logging"sv ||
        Plugin.name() == "wasmedge_process"sv ||
        Plugin.name() == "wasmedge_tensorflow"sv ||
        Plugin.name() == "wasmedge_tensorflowlite"sv ||
        Plugin.name() == "wasmedge_image"sv ||
        Plugin.name() == "wasmedge_stablediffusion"sv) {
      continue;
    }
    for (const auto &Module : Plugin.modules()) {
      PlugInModInsts.push_back(Module.create());
    }
    for (const auto &Component : Plugin.components()) {
      PlugInCompInsts.push_back(Component.create());
    }
  }
}

void VM::unsafeRegisterBuiltInHosts() {
  // Register all created WASI host modules.
  for (auto &It : BuiltInModInsts) {
    ExecutorEngine.registerModule(StoreRef, *(It.second.get()));
  }
}

void VM::unsafeRegisterPlugInHosts() {
  // Register all created module instances from plugins.
  for (auto &It : PlugInModInsts) {
    ExecutorEngine.registerModule(StoreRef, *(It.get()));
  }
  for (auto &It : PlugInCompInsts) {
    ExecutorEngine.registerComponent(StoreRef, *(It.get()));
  }
}

Expect<void> VM::unsafeRegisterModule(std::string_view Name,
                                      const std::filesystem::path &Path) {
  if (Stage == VMStage::Instantiated) {
    // When registering a module, the instantiated module in the store will be
    // reset. Therefore the instantiation should restart.
    Stage = VMStage::Validated;
  }
  // Load module.
  EXPECTED_TRY(std::shared_ptr<AST::Module> Module,
               LoaderEngine.parseModule(Path));

  EXPECTED_TRY(unsafeRegisterModule(Name, *Module));

  if (!RegASTModules.empty()) {
    RegASTModules.back() = Module;
  }

  return {};
}

Expect<void> VM::unsafeRegisterModule(std::string_view Name,
                                      Span<const Byte> Code) {
  if (Stage == VMStage::Instantiated) {
    // When registering a module, the instantiated module in the store will be
    // reset. Therefore the instantiation should restart.
    Stage = VMStage::Validated;
  }
  // Load module.
  EXPECTED_TRY(std::shared_ptr<AST::Module> Module,
               LoaderEngine.parseModule(Code));

  EXPECTED_TRY(unsafeRegisterModule(Name, *Module));

  if (!RegASTModules.empty()) {
    RegASTModules.back() = Module;
  }

  return {};
}

Expect<void> VM::unsafeRegisterModule(std::string_view Name,
                                      const AST::Module &Module) {
  if (Stage == VMStage::Instantiated) {
    // When registering a module, the instantiated module in the store will be
    // reset. Therefore the instantiation should restart.
    Stage = VMStage::Validated;
  }
  // Validate module.
  EXPECTED_TRY(ValidatorEngine.validate(Module));

  std::string ID = Module.getID();

#ifdef WASMEDGE_USE_LLVM
  std::optional<WasmEdge::LLVM::LazyJITState> State;
  if (Conf.getRuntimeConfigure().getRunMode() == RunMode::LazyJIT &&
      !Module.getSymbol()) {
    EXPECTED_TRY(State, prepareLazyJIT(const_cast<AST::Module &>(Module)));
  }
#endif

  // Instantiate and register module.
  EXPECTED_TRY(auto ModInst,
               ExecutorEngine.registerModule(StoreRef, Module, Name));
  RegModInsts.push_back(std::move(ModInst));

#ifdef WASMEDGE_USE_LLVM
  if (State) {
    std::unique_lock Lock(LazyJITMutex);
    LazyJITStates.insert({ID, std::move(*State)});
  }
#endif

#ifdef WASMEDGE_USE_LLVM
  size_t InstIdx = RegModInsts.size() - 1;
  if (Conf.getRuntimeConfigure().getRunMode() == RunMode::LazyJIT) {
    auto It = RegModMap.find(ID);
    if (It != RegModMap.end()) {
      It->second[1] = InstIdx;
    } else {
      RegASTModules.push_back(std::make_shared<const AST::Module>(Module));
      RegModMap[ID] = {RegASTModules.size() - 1, InstIdx};
    }
  }
#endif

  return {};
}

Expect<void>
VM::unsafeRegisterModule(std::string_view Name,
                         const Runtime::Instance::ModuleInstance &ModInst) {
  if (Stage == VMStage::Instantiated) {
    // When registering a module, the instantiated module in the store will be
    // reset. Therefore the instantiation should restart.
    Stage = VMStage::Validated;
  }
  return ExecutorEngine.registerModule(StoreRef, ModInst, Name);
}

Expect<void> VM::unsafeUnregisterModule(std::string_view Name) {
  auto InstIt = std::find_if(
      RegModInsts.begin(), RegModInsts.end(),
      [&Name](const std::unique_ptr<Runtime::Instance::ModuleInstance> &Inst) {
        return Inst && Inst->getModuleName() == Name;
      });
  if (InstIt != RegModInsts.end()) {
    auto ModId = (*InstIt)->getID();
    auto *ModInst = (*InstIt).release();

    if (ModInst) {
      ModInst->terminate();
    }
    RegModMap.erase(ModId);
    return {};
  }
  for (auto It = BuiltInModInsts.begin(); It != BuiltInModInsts.end(); ++It) {
    if (It->second && It->second->getModuleName() == Name) {
      auto *ModInst = It->second.release();
      BuiltInModInsts.erase(It);

      if (ModInst) {
        ModInst->terminate();
      }
      return {};
    }
  }
  for (auto It = PlugInModInsts.begin(); It != PlugInModInsts.end(); ++It) {
    if (*It && (*It)->getModuleName() == Name) {
      auto *ModInst = It->release();
      PlugInModInsts.erase(It);

      if (ModInst) {
        ModInst->terminate();
      }
      return {};
    }
  }

  return {};
}

Expect<std::vector<std::pair<ValVariant, ValType>>>
VM::unsafeRunWasmFile(const std::filesystem::path &Path, std::string_view Func,
                      Span<const ValVariant> Params,
                      Span<const ValType> ParamTypes) {
  if (Stage == VMStage::Instantiated) {
    // When running another module, the instantiated module in the store will
    // be reset. Therefore the instantiation should restart.
    Stage = VMStage::Validated;
  }
  // Load wasm unit.
  EXPECTED_TRY(auto ComponentOrModule, LoaderEngine.parseWasmUnit(Path));
  return std::visit(
      VisitUnit<Expect<std::vector<std::pair<ValVariant, ValType>>>>(
          [&](auto &M) -> Expect<std::vector<std::pair<ValVariant, ValType>>> {
            Mod = std::move(M);
            return unsafeRunWasmFile(*Mod, Func, Params, ParamTypes);
          },
          [&](auto &C) -> Expect<std::vector<std::pair<ValVariant, ValType>>> {
            Comp = std::move(C);
            return unsafeRunWasmFile(*Comp, Func, Params, ParamTypes);
          }),
      ComponentOrModule);
}

Expect<std::vector<std::pair<ValVariant, ValType>>>
VM::unsafeRunWasmFile(Span<const Byte> Code, std::string_view Func,
                      Span<const ValVariant> Params,
                      Span<const ValType> ParamTypes) {
  if (Stage == VMStage::Instantiated) {
    // When running another module, the instantiated module in the store will
    // be reset. Therefore the instantiation should restart.
    Stage = VMStage::Validated;
  }
  // Load wasm unit.
  EXPECTED_TRY(auto ComponentOrModule, LoaderEngine.parseWasmUnit(Code));
  return std::visit(
      VisitUnit<Expect<std::vector<std::pair<ValVariant, ValType>>>>(
          [&](auto &M) -> Expect<std::vector<std::pair<ValVariant, ValType>>> {
            Mod = std::move(M);
            return unsafeRunWasmFile(*Mod, Func, Params, ParamTypes);
          },
          [&](auto &C) -> Expect<std::vector<std::pair<ValVariant, ValType>>> {
            Comp = std::move(C);
            return unsafeRunWasmFile(*Comp, Func, Params, ParamTypes);
          }),
      ComponentOrModule);
}

Expect<std::vector<std::pair<ValVariant, ValType>>>
VM::unsafeRunWasmFile(const AST::Component::Component &Component,
                      std::string_view, Span<const ValVariant>,
                      Span<const ValType>) {
  if (Stage == VMStage::Instantiated) {
    // When running another module, the instantiated module in the store will
    // be reset. Therefore the instantiation should restart.
    Stage = VMStage::Validated;
  }
  EXPECTED_TRY(ValidatorEngine.validate(Component));
  spdlog::error("component execution is not done yet."sv);
  return Unexpect(ErrCode::Value::RuntimeError);
}

Expect<std::vector<std::pair<ValVariant, ValType>>>
VM::unsafeRunWasmFile(const AST::Module &Module, std::string_view Func,
                      Span<const ValVariant> Params,
                      Span<const ValType> ParamTypes) {
  if (Stage == VMStage::Instantiated) {
    // When running another module, the instantiated module in the store will
    // be reset. Therefore the instantiation should restart.
    Stage = VMStage::Validated;
  }
  EXPECTED_TRY(ValidatorEngine.validate(Module));
  EXPECTED_TRY(ActiveModInst,
               ExecutorEngine.instantiateModule(StoreRef, Module));

  // Get module instance.
  if (ActiveModInst) {
    // Execute function and return values using the module instance.
    return unsafeExecute(ActiveModInst.get(), Func, Params, ParamTypes);
  }
  spdlog::error(ErrCode::Value::WrongInstanceAddress);
  spdlog::error(ErrInfo::InfoExecuting(""sv, Func));
  return Unexpect(ErrCode::Value::WrongInstanceAddress);
}

Async<Expect<std::vector<std::pair<ValVariant, ValType>>>>
VM::asyncRunWasmFile(const std::filesystem::path &Path, std::string_view Func,
                     Span<const ValVariant> Params,
                     Span<const ValType> ParamTypes) {
  Expect<std::vector<std::pair<ValVariant, ValType>>> (VM::*FPtr)(
      const std::filesystem::path &, std::string_view, Span<const ValVariant>,
      Span<const ValType>) = &VM::runWasmFile;
  return {FPtr,
          *this,
          std::filesystem::path(Path),
          std::string(Func),
          std::vector(Params.begin(), Params.end()),
          std::vector(ParamTypes.begin(), ParamTypes.end())};
}

Async<Expect<std::vector<std::pair<ValVariant, ValType>>>>
VM::asyncRunWasmFile(Span<const Byte> Code, std::string_view Func,
                     Span<const ValVariant> Params,
                     Span<const ValType> ParamTypes) {
  Expect<std::vector<std::pair<ValVariant, ValType>>> (VM::*FPtr)(
      Span<const Byte>, std::string_view, Span<const ValVariant>,
      Span<const ValType>) = &VM::runWasmFile;
  return {FPtr,
          *this,
          Code,
          std::string(Func),
          std::vector(Params.begin(), Params.end()),
          std::vector(ParamTypes.begin(), ParamTypes.end())};
}

Async<Expect<std::vector<std::pair<ValVariant, ValType>>>>
VM::asyncRunWasmFile(const AST::Module &Module, std::string_view Func,
                     Span<const ValVariant> Params,
                     Span<const ValType> ParamTypes) {
  Expect<std::vector<std::pair<ValVariant, ValType>>> (VM::*FPtr)(
      const AST::Module &, std::string_view, Span<const ValVariant>,
      Span<const ValType>) = &VM::runWasmFile;
  return {FPtr,
          *this,
          Module,
          std::string(Func),
          std::vector(Params.begin(), Params.end()),
          std::vector(ParamTypes.begin(), ParamTypes.end())};
}

Expect<void> VM::unsafeLoadWasm(const std::filesystem::path &Path) {
  // If loading does not succeed, the previous status will be preserved.
  EXPECTED_TRY(auto ComponentOrModule, LoaderEngine.parseWasmUnit(Path));

  std::visit(VisitUnit<void>([&](auto &M) -> void { Mod = std::move(M); },
                             [&](auto &C) -> void { Comp = std::move(C); }),
             ComponentOrModule);
  Stage = VMStage::Loaded;
  return {};
}

Expect<void> VM::unsafeLoadWasm(Span<const Byte> Code) {
  // If loading does not succeed, the previous status will be preserved.
  EXPECTED_TRY(auto ComponentOrModule, LoaderEngine.parseWasmUnit(Code));

  std::visit(VisitUnit<void>([&](auto &M) -> void { Mod = std::move(M); },
                             [&](auto &C) -> void { Comp = std::move(C); }),
             ComponentOrModule);
  Stage = VMStage::Loaded;
  return {};
}

Expect<void> VM::unsafeLoadWasm(const AST::Module &Module) {
  Mod = std::make_unique<AST::Module>(Module);
  Stage = VMStage::Loaded;
  return {};
}

struct Validate {
  // borrow validator to pass control to it
  Validate(Validator::Validator &Engine) : ValidatorEngine(Engine) {}
  Expect<void> operator()(const std::unique_ptr<AST::Module> &Mod) const {
    return ValidatorEngine.validate(*Mod.get());
  }
  Expect<void>
  operator()(const std::unique_ptr<AST::Component::Component> &Comp) const {
    return ValidatorEngine.validate(*Comp.get());
  }

private:
  Validator::Validator &ValidatorEngine;
};

Expect<void> VM::unsafeValidate() {
  if (Stage < VMStage::Loaded) {
    // Do not validate when the module is not loaded.
    spdlog::error(ErrCode::Value::WrongVMWorkflow);
    return Unexpect(ErrCode::Value::WrongVMWorkflow);
  }

  if (Mod) {
    EXPECTED_TRY(ValidatorEngine.validate(*Mod.get()));
  } else if (Comp) {
    EXPECTED_TRY(ValidatorEngine.validate(*Comp.get()));
  } else {
    spdlog::error(ErrCode::Value::WrongVMWorkflow);
    return Unexpect(ErrCode::Value::WrongVMWorkflow);
  }
  Stage = VMStage::Validated;
  return {};
}

Expect<void> VM::unsafeInstantiate() {
  if (Stage < VMStage::Validated) {
    // Do not instantiate when the module is not validated.
    spdlog::error(ErrCode::Value::WrongVMWorkflow);
    return Unexpect(ErrCode::Value::WrongVMWorkflow);
  }
  if (Mod) {
    std::string ID = Mod->getID();

    if ((Conf.getRuntimeConfigure().getRunMode() == RunMode::JIT ||
         Conf.getRuntimeConfigure().getRunMode() == RunMode::LazyJIT) &&
        !Mod->getSymbol()) {
#ifdef WASMEDGE_USE_LLVM
      if (Conf.getRuntimeConfigure().getRunMode() == RunMode::LazyJIT) {
        EXPECTED_TRY(auto State, prepareLazyJIT(*Mod));
        std::unique_lock Lock(LazyJITMutex);
        LazyJITStates[ID] = std::move(State);
      } else {
        LLVM::Compiler Compiler(Conf);
        Compiler.checkConfigure()
            .map_error([](uint32_t Err) {
              if (Err != ErrCode::Value::Success) {
                spdlog::error("Compiler Configure failed. Error code: {}, use "
                              "interpreter mode instead."sv,
                              Err);
              }
              return ErrCode::Value::Success;
            })
            .and_then([&]() { return Compiler.compile(*Mod); })
            .map_error([](uint32_t Err) {
              if (Err != ErrCode::Value::Success) {
                spdlog::error("Compilation failed. Error code: {}, use "
                              "interpreter mode instead."sv,
                              Err);
              }
              return ErrCode::Value::Success;
            })
            .and_then([&](auto LLModule) {
              LLVM::JIT JIT(Conf);
              return JIT.load(LLModule);
            })
            .map_error([](uint32_t Err) {
              if (Err != ErrCode::Value::Success) {
                spdlog::warn(
                    "JIT failed. Error code: {}, use interpreter mode instead."sv,
                    Err);
              }
              return ErrCode::Value::Success;
            })
            .and_then([&](auto Module) {
              return LoaderEngine.loadExecutable(*Mod, std::move(Module));
            })
            .map_error([](uint32_t Err) {
              if (Err != ErrCode::Value::Success) {
                spdlog::warn("Loader failed. Error code: {}, use interpreter "
                             "mode instead."sv,
                             Err);
              }
              return ErrCode::Value::Success;
            });
      }
#else
      spdlog::warn("JIT was requested but WasmEdge was built without LLVM, "
                   "falling back to interpreter."sv);
#endif
    }

#ifdef WASMEDGE_USE_LLVM
    if (Conf.getRuntimeConfigure().getRunMode() == RunMode::LazyJIT) {
      std::unique_lock Lock(LazyJITMutex);
      auto StateIt = LazyJITStates.find(ID);
      if (StateIt != LazyJITStates.end()) {
        size_t ImportFuncCount = 0;
        for (const auto &ImpDesc : Mod->getImportSection().getContent()) {
          if (ImpDesc.getExternalType() == ExternalType::Function) {
            ++ImportFuncCount;
          }
        }
        StateIt->second.ImportFuncCount =
            static_cast<uint32_t>(ImportFuncCount);
        StateIt->second.LazyCompiledFuncs.clear();
      }
    }
#endif

    EXPECTED_TRY(ActiveModInst,
                 ExecutorEngine.instantiateModule(StoreRef, *Mod));

    Stage = VMStage::Instantiated;
    return {};
  } else if (Comp) {
    EXPECTED_TRY(ActiveCompInst,
                 ExecutorEngine.instantiateComponent(StoreRef, *Comp));
    Stage = VMStage::Instantiated;
    return {};
  } else {
    spdlog::error(ErrCode::Value::WrongVMWorkflow);
    return Unexpect(ErrCode::Value::WrongVMWorkflow);
  }
}

Expect<std::vector<std::pair<ValVariant, ValType>>>
VM::unsafeExecute(std::string_view Func, Span<const ValVariant> Params,
                  Span<const ValType> ParamTypes) {
  if (unlikely(!ActiveModInst)) {
    spdlog::error(ErrCode::Value::WrongInstanceAddress);
    spdlog::error(ErrInfo::InfoExecuting("When invoking"sv, Func));
    return Unexpect(ErrCode::Value::WrongInstanceAddress);
  }
  // Execute function and return values using the module instance.
  return unsafeExecute(ActiveModInst.get(), Func, Params, ParamTypes);
}

Expect<std::vector<std::pair<ValVariant, ValType>>>
VM::unsafeExecute(std::string_view ModName, std::string_view Func,
                  Span<const ValVariant> Params,
                  Span<const ValType> ParamTypes) {
  // Find module instance by name.
  const auto *FindModInst = StoreRef.findModule(ModName);
  if (unlikely(!FindModInst)) {
    spdlog::error(ErrCode::Value::WrongInstanceAddress);
    spdlog::error(ErrInfo::InfoExecuting(ModName, Func));
    return Unexpect(ErrCode::Value::WrongInstanceAddress);
  }
  // Execute function and return values using the module instance.
  return unsafeExecute(FindModInst, Func, Params, ParamTypes);
}

Expect<std::vector<std::pair<ComponentValVariant, ComponentValType>>>
VM::unsafeExecuteComponent(std::string_view Func,
                           Span<const ComponentValVariant> Params,
                           Span<const ComponentValType> ParamTypes) {
  if (unlikely(!ActiveCompInst)) {
    spdlog::error(ErrCode::Value::WrongInstanceAddress);
    spdlog::error(ErrInfo::InfoExecuting("When invoking"sv, Func));
    return Unexpect(ErrCode::Value::WrongInstanceAddress);
  }
  return unsafeExecuteComponent(ActiveCompInst.get(), Func, Params, ParamTypes);
}

Expect<std::vector<std::pair<ComponentValVariant, ComponentValType>>>
VM::unsafeExecuteComponent(std::string_view CompName, std::string_view Func,
                           Span<const ComponentValVariant> Params,
                           Span<const ComponentValType> ParamTypes) {
  // Find module instance by name.
  const auto *FindCompInst = StoreRef.findComponent(CompName);
  if (unlikely(!FindCompInst)) {
    spdlog::error(ErrCode::Value::WrongInstanceAddress);
    spdlog::error(ErrInfo::InfoExecuting(CompName, Func));
    return Unexpect(ErrCode::Value::WrongInstanceAddress);
  }
  // Execute function and return values using the component instance.
  return unsafeExecuteComponent(FindCompInst, Func, Params, ParamTypes);
}

Expect<std::vector<std::pair<ValVariant, ValType>>>
VM::unsafeExecute(const Runtime::Instance::ModuleInstance *ModInst,
                  std::string_view Func, Span<const ValVariant> Params,
                  Span<const ValType> ParamTypes) {
  // Find exported function by name.
  Runtime::Instance::FunctionInstance *FuncInst =
      ModInst->findFuncExports(Func);

#ifdef WASMEDGE_USE_LLVM
  // lazy JIT: compile function on-demand if needed.
  if (Conf.getRuntimeConfigure().getRunMode() == RunMode::LazyJIT && FuncInst) {
    bool NeedsCompile = false;
    uint32_t FuncIdx = UINT32_MAX;
    {
      std::shared_lock Lock(LazyJITMutex);
      if (FuncInst->isWasmFunction()) {
        NeedsCompile = true;
        if (auto Res = ModInst->getFuncIdx(FuncInst)) {
          FuncIdx = *Res;
        }
      }
    }
    if (NeedsCompile && FuncIdx != UINT32_MAX) {
      if (auto Result = lazyCompileFunctions(ModInst->getID(), FuncIdx);
          !Result) {
        return Unexpect(Result.error());
      }
    }
  }
#endif

  // Execute function.
  return ExecutorEngine.invoke(FuncInst, Params, ParamTypes)
      .map_error([&ModInst, &Func](auto E) {
        if (E != ErrCode::Value::Terminated) {
          spdlog::error(ErrInfo::InfoExecuting(ModInst->getModuleName(), Func));
        }
        return E;
      });
}

Expect<std::vector<std::pair<ComponentValVariant, ComponentValType>>>
VM::unsafeExecuteComponent(const Runtime::Instance::ComponentInstance *CompInst,
                           std::string_view Func,
                           Span<const ComponentValVariant> Params,
                           Span<const ComponentValType> ParamTypes) {
  // Find exported function by name.
  Runtime::Instance::Component::FunctionInstance *FuncInst =
      CompInst->findFunction(Func);

  // Execute function.
  return ExecutorEngine.invoke(FuncInst, Params, ParamTypes)
      .map_error([&CompInst, &Func](auto E) {
        if (E != ErrCode::Value::Terminated) {
          spdlog::error(
              ErrInfo::InfoExecuting(CompInst->getComponentName(), Func));
        }
        return E;
      });
}

Async<Expect<std::vector<std::pair<ValVariant, ValType>>>>
VM::asyncExecute(std::string_view Func, Span<const ValVariant> Params,
                 Span<const ValType> ParamTypes) {
  Expect<std::vector<std::pair<ValVariant, ValType>>> (VM::*FPtr)(
      std::string_view, Span<const ValVariant>, Span<const ValType>) =
      &VM::execute;
  return {FPtr, *this, std::string(Func),
          std::vector(Params.begin(), Params.end()),
          std::vector(ParamTypes.begin(), ParamTypes.end())};
}

Async<Expect<std::vector<std::pair<ValVariant, ValType>>>>
VM::asyncExecute(std::string_view ModName, std::string_view Func,
                 Span<const ValVariant> Params,
                 Span<const ValType> ParamTypes) {
  Expect<std::vector<std::pair<ValVariant, ValType>>> (VM::*FPtr)(
      std::string_view, std::string_view, Span<const ValVariant>,
      Span<const ValType>) = &VM::execute;
  return {FPtr,
          *this,
          std::string(ModName),
          std::string(Func),
          std::vector(Params.begin(), Params.end()),
          std::vector(ParamTypes.begin(), ParamTypes.end())};
}

Async<Expect<std::vector<std::pair<ComponentValVariant, ComponentValType>>>>
VM::asyncExecuteComponent(std::string_view Func,
                          Span<const ComponentValVariant> Params,
                          Span<const ComponentValType> ParamTypes) {
  Expect<std::vector<std::pair<ComponentValVariant, ComponentValType>>> (
      VM::*FPtr)(std::string_view, Span<const ComponentValVariant>,
                 Span<const ComponentValType>) = &VM::executeComponent;
  return {FPtr, *this, std::string(Func),
          std::vector(Params.begin(), Params.end()),
          std::vector(ParamTypes.begin(), ParamTypes.end())};
}

Async<Expect<std::vector<std::pair<ComponentValVariant, ComponentValType>>>>
VM::asyncExecuteComponent(std::string_view CompName, std::string_view Func,
                          Span<const ComponentValVariant> Params,
                          Span<const ComponentValType> ParamTypes) {
  Expect<std::vector<std::pair<ComponentValVariant, ComponentValType>>> (
      VM::*FPtr)(std::string_view, std::string_view,
                 Span<const ComponentValVariant>,
                 Span<const ComponentValType>) = &VM::executeComponent;
  return {FPtr,
          *this,
          std::string(CompName),
          std::string(Func),
          std::vector(Params.begin(), Params.end()),
          std::vector(ParamTypes.begin(), ParamTypes.end())};
}

void VM::unsafeCleanup() {
  if (Mod) {
    Mod.reset();
  }
  if (Comp) {
    Comp.reset();
  }
  if (ActiveModInst) {
    auto *RawMod = ActiveModInst.release();
    if (RawMod) {
      RawMod->terminate();
    }
  }
  if (ActiveCompInst) {
    ActiveCompInst.reset();
  }
  StoreRef.reset();
  cleanupModInstContainer(RegModInsts);
  RegASTModules.clear();
  RegModMap.clear();
  Stat.clear();
  unsafeLoadBuiltInHosts();
  unsafeLoadPlugInHosts();
  unsafeRegisterBuiltInHosts();
  unsafeRegisterPlugInHosts();
  LoaderEngine.reset();
  Stage = VMStage::Inited;
#ifdef WASMEDGE_USE_LLVM
  // LazyJITStates.clear() will automatically clean up all unique_ptr
  // LLContexts.
  LazyJITStates.clear();
#endif
}

std::vector<std::pair<std::string, const AST::FunctionType &>>
VM::unsafeGetFunctionList() const {
  std::vector<std::pair<std::string, const AST::FunctionType &>> Map;
  if (ActiveModInst) {
    ActiveModInst->getFuncExports([&](const auto &FuncExports) {
      Map.reserve(FuncExports.size());
      for (auto &&Func : FuncExports) {
        const auto &FuncType = (Func.second)->getFuncType();
        Map.emplace_back(Func.first, FuncType);
      }
    });
  }
  return Map;
}

std::vector<std::pair<std::string, const AST::Component::FuncType &>>
VM::unsafeGetComponentFunctionList() const {
  std::vector<std::pair<std::string, const AST::Component::FuncType &>> Map;
  if (ActiveCompInst) {
    ActiveCompInst->getFuncExports([&](const auto &FuncExports) {
      Map.reserve(FuncExports.size());
      for (auto &&Func : FuncExports) {
        const auto &FuncType = (Func.second)->getFuncType();
        Map.emplace_back(Func.first, FuncType);
      }
    });
  }
  return Map;
}

Runtime::Instance::ModuleInstance *
VM::unsafeGetImportModule(const HostRegistration Type) const {
  if (auto Iter = BuiltInModInsts.find(Type); Iter != BuiltInModInsts.cend()) {
    return Iter->second.get();
  }
  return nullptr;
}

const Runtime::Instance::ModuleInstance *VM::unsafeGetActiveModule() const {
  if (ActiveModInst) {
    return ActiveModInst.get();
  }
  return nullptr;
};

#ifdef WASMEDGE_USE_LLVM
Expect<void> VM::lazyCompileFunctions(const std::string &ID, uint32_t FuncIdx) {
  std::unique_lock Lock(LazyJITMutex);
  uint32_t ImportFuncCount = 0;
  const AST::Module *ModulePtr = nullptr;
  LLVM::Data *LLDataPtr = nullptr;
  std::unique_ptr<LLVM::Compiler::CompileContext,
                  LLVM::Compiler::CompileContextDeleter> *LLContextPtr =
      nullptr;
  LLVM::LazyJITState *StatePtr = nullptr;
  const Runtime::Instance::ModuleInstance *ModInst = nullptr;

  if (ActiveModInst && ActiveModInst->getID() == ID) {
    ModInst = ActiveModInst.get();
    ModulePtr = Mod.get();
  } else {
    auto ItMap = RegModMap.find(ID);
    if (ItMap != RegModMap.end()) {
      ModInst = RegModInsts[ItMap->second[1]].get();
      ModulePtr = RegASTModules[ItMap->second[0]].get();
    }
  }

  if (!ModInst || !ModulePtr) {
    return {};
  }

  auto It = LazyJITStates.find(ID);
  if (It != LazyJITStates.end()) {
    StatePtr = &It->second;
    ImportFuncCount = StatePtr->ImportFuncCount;
    LLDataPtr = &StatePtr->LLData;
    LLContextPtr = &StatePtr->LLContext;
  } else {
    spdlog::error("[lazy-jit]: failed to find JIT state for ID: {}"sv, ID);
    return Unexpect(ErrCode::Value::WrongInstanceAddress);
  }

  if (FuncIdx < ImportFuncCount) {
    return {};
  }

  uint32_t LocalFuncIdx = FuncIdx - ImportFuncCount;

  auto FuncResult = ModInst->getFuncInst(FuncIdx);
  if (!FuncResult) {
    spdlog::error(
        "[lazy-jit]: failed to get function instance for index {}, module ID: {}"sv,
        FuncIdx, ID);
    return Unexpect(FuncResult.error());
  }
  auto *FuncInst = *FuncResult;

  // If already compiled or not a Wasm function, nothing to do.
  if (!FuncInst->isWasmFunction() || FuncInst->isCompiledFunction()) {
    return {};
  }

  if (StatePtr && StatePtr->LazyCompiledFuncs.count(LocalFuncIdx) > 0) {
    return {};
  }

  const std::unordered_set<uint32_t> EmptyCompiledSet;
  const std::unordered_set<uint32_t> &CompiledSet =
      StatePtr ? StatePtr->LazyCompiledFuncs : EmptyCompiledSet;

  std::vector<uint32_t> BatchLocals;
  collectLazyCallGraphBatch(LocalFuncIdx, ModulePtr, ImportFuncCount,
                            CompiledSet, BatchLocals);
  if (BatchLocals.empty()) {
    return {};
  }

  spdlog::debug("[lazy-jit]: Lazy compiling batch ({} local funcs) for wasm "
                "entry local "
                "{}, module ID: {}"sv,
                BatchLocals.size(), LocalFuncIdx, ID);

  LLVM::Compiler Compiler(Conf);
  auto ConfigResult = Compiler.checkConfigure();
  if (!ConfigResult) {
    spdlog::error(
        "[lazy-jit]: Lazy JIT compiler config failed: {}, module ID: {}"sv,
        ConfigResult.error(), ID);
    return Unexpect(ConfigResult.error());
  }

  if (!LLDataPtr->hasModule()) {
    LLDataPtr->resetModule();
  }
  auto CompileResult = Compiler.compileFunctions(
      std::move(*LLDataPtr),
      static_cast<LLVM::Compiler::CompileContext *>(LLContextPtr->get()),
      *ModulePtr,
      WasmEdge::Span<const uint32_t>(BatchLocals.data(), BatchLocals.size()));
  if (!CompileResult) {
    spdlog::error(
        "[lazy-jit]: Lazy JIT function compilation failed: {}, module ID: {}"sv,
        CompileResult.error(), ID);
    return Unexpect(CompileResult.error());
  }

  *LLDataPtr = std::move(*CompileResult);
  LLVM::JIT JIT(Conf);
  std::shared_ptr<LLVM::JITLibrary> JITLib;
  if (StatePtr && StatePtr->JITLib) {
    JITLib = StatePtr->JITLib;
  }

  std::vector<uint32_t> BatchGlobal;
  BatchGlobal.reserve(BatchLocals.size());
  for (uint32_t L : BatchLocals) {
    BatchGlobal.push_back(ImportFuncCount + L);
  }

  std::vector<LLVM::WasmFunctionCodeAddress> ResolvedAddresses;
  if (JITLib) {
    auto AddrRes = JIT.add(
        *JITLib, *LLDataPtr,
        WasmEdge::Span<const uint32_t>(BatchGlobal.data(), BatchGlobal.size()));
    if (!AddrRes) {
      spdlog::error("[lazy-jit]: Lazy JIT add failed: {}, module ID: {}"sv,
                    AddrRes.error(), ID);
      return Unexpect(AddrRes.error());
    }
    ResolvedAddresses = std::move(*AddrRes);
  } else {
    auto LoadResult = JIT.load(*LLDataPtr, true);
    if (!LoadResult) {
      spdlog::error("[lazy-jit]: Lazy JIT load failed: {}, module ID: {}"sv,
                    LoadResult.error(), ID);
      return Unexpect(LoadResult.error());
    }
    JITLib = std::static_pointer_cast<LLVM::JITLibrary>(*LoadResult);
    if (StatePtr) {
      StatePtr->JITLib = JITLib;
    }
    auto LkRes = JIT.lookupWasmFunctionSymbols(
        *JITLib, LLDataPtr->getPrefix(),
        WasmEdge::Span<const uint32_t>(BatchGlobal.data(), BatchGlobal.size()));
    if (!LkRes) {
      spdlog::error(
          "[lazy-jit]: Lazy JIT symbol resolution failed: {}, module ID: {}"sv,
          LkRes.error(), ID);
      return Unexpect(LkRes.error());
    }
    ResolvedAddresses = std::move(*LkRes);
  }

  if (ResolvedAddresses.size() != BatchLocals.size()) {
    spdlog::error(
        "[lazy-jit]: Lazy JIT address count mismatch, module ID: {}"sv, ID);
    return Unexpect(ErrCode::Value::LazyCompilationError);
  }

  if (auto IntrinsicsSymbol = JITLib->getIntrinsics()) {
    *IntrinsicsSymbol = &Executor::Executor::Intrinsics;
  } else {
    spdlog::error(
        "[lazy-jit]: failed to get intrinsics symbol, module ID: {}"sv, ID);
    return Unexpect(ErrCode::Value::LazyCompilationError);
  }

  for (size_t I = 0; I < BatchLocals.size(); ++I) {
    const uint32_t L = BatchLocals[I];
    const uint32_t WasmFuncIdx = ImportFuncCount + L;
    LLVM::WasmFunctionCodeAddress CompiledCodePtr = ResolvedAddresses[I];
    auto BatchFuncResult = ModInst->getFuncInst(WasmFuncIdx);
    if (!BatchFuncResult) {
      spdlog::error(
          "[lazy-jit]: failed to get function instance for index {}, module ID: {}"sv,
          WasmFuncIdx, ID);
      return Unexpect(ErrCode::Value::WrongInstanceAddress);
    }

    auto *BatchFuncInst = *BatchFuncResult;
    if (BatchFuncInst->isWasmFunction()) {
      auto CompiledSym = JITLib->createSymbol(
          reinterpret_cast<Runtime::Instance::FunctionInstance::CompiledFunction
                               *>(CompiledCodePtr));
      BatchFuncInst->unsafeUpgradeToCompiled(std::move(CompiledSym));
    }
  }

  if (StatePtr) {
    for (uint32_t L : BatchLocals) {
      StatePtr->LazyCompiledFuncs.insert(L);
    }
  }

  spdlog::debug(
      "Lazy compilation completed for batch of {} functions, total compiled: "
      "{}, module ID: {}"sv,
      BatchLocals.size(),
      StatePtr ? StatePtr->LazyCompiledFuncs.size() : BatchLocals.size(), ID);

  return {};
}
#endif

#ifdef WASMEDGE_USE_LLVM
Expect<WasmEdge::LLVM::LazyJITState> VM::prepareLazyJIT(AST::Module &Module) {
  WasmEdge::LLVM::LazyJITState State;
  LLVM::Compiler Compiler(Conf);
  EXPECTED_TRY(
      Compiler.checkConfigure().map_error([](uint32_t Err) { return Err; }));

  auto Prefix = fmt::format("m{}_"sv, Module.getID());
  EXPECTED_TRY(auto LLModule, Compiler.compileInfrastructure(Module, Prefix));

  State.LLData = std::move(LLModule.first);
  State.LLContext = std::move(LLModule.second);

  LLVM::JIT JIT(Conf);
  EXPECTED_TRY(auto Exec, JIT.load(State.LLData, true));
  State.JITLib = std::static_pointer_cast<LLVM::JITLibrary>(Exec);

  EXPECTED_TRY(LoaderEngine.loadExecutable(Module, State.JITLib));

  size_t ImportFuncCount = 0;
  for (const auto &ImpDesc : Module.getImportSection().getContent()) {
    if (ImpDesc.getExternalType() == ExternalType::Function) {
      ++ImportFuncCount;
    }
  }
  State.ImportFuncCount = static_cast<uint32_t>(ImportFuncCount);

  return State;
}
#endif

} // namespace VM
} // namespace WasmEdge
