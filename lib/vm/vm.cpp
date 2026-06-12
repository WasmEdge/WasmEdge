// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "vm/vm.h"

#include "ast/module.h"
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
#include <memory>
#include <variant>

namespace WasmEdge {
namespace VM {

namespace {

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

  // Set up the lazy JIT engine if lazy JIT mode is enabled.
#ifdef WASMEDGE_USE_LLVM
  if (Conf.getRuntimeConfigure().getRunMode() == RunMode::LazyJIT) {
    spdlog::warn(
        "Lazy JIT is an alpha and experimental feature, which is not ready for production use."sv);
    LazyEngine = std::make_unique<LLVM::LazyJITEngine>(Conf);
    ExecutorEngine.registerLazyCompilationCallback(
        [this](const Runtime::Instance::FunctionInstance *FuncInst)
            -> Expect<void> { return LazyEngine->compileOnDemand(FuncInst); });
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
  // Load module.
  EXPECTED_TRY(std::shared_ptr<AST::Module> Module,
               LoaderEngine.parseModule(Path));
  return unsafeRegisterModule(Name, std::move(Module));
}

Expect<void> VM::unsafeRegisterModule(std::string_view Name,
                                      Span<const Byte> Code) {
  // Load module.
  EXPECTED_TRY(std::shared_ptr<AST::Module> Module,
               LoaderEngine.parseModule(Code));
  return unsafeRegisterModule(Name, std::move(Module));
}

Expect<void> VM::unsafeRegisterModule(std::string_view Name,
                                      const AST::Module &Module) {
#ifdef WASMEDGE_USE_LLVM
  if (LazyEngine) {
    // The lazy JIT engine needs to own the AST module and hook the compiled
    // executable into it, so register an owned copy instead of mutating the
    // caller's module.
    return unsafeRegisterModule(Name, std::make_shared<AST::Module>(Module));
  }
#endif
  unsafeRevertStageToValidated();
  // Validate module.
  EXPECTED_TRY(ValidatorEngine.validate(Module));
  // Instantiate and register module.
  EXPECTED_TRY(auto ModInst,
               ExecutorEngine.registerModule(StoreRef, Module, Name));
  RegModInsts.push_back(std::move(ModInst));
  return {};
}

Expect<void> VM::unsafeRegisterModule(std::string_view Name,
                                      std::shared_ptr<AST::Module> Module) {
  unsafeRevertStageToValidated();
  // Validate module.
  EXPECTED_TRY(ValidatorEngine.validate(*Module));

#ifdef WASMEDGE_USE_LLVM
  if (LazyEngine && !Module->getSymbol()) {
    EXPECTED_TRY(auto Exec, LazyEngine->prepare(Module));
    EXPECTED_TRY(LoaderEngine.loadExecutable(*Module, std::move(Exec)));
  }
#endif

  // Instantiate and register module.
  EXPECTED_TRY(auto ModInst,
               ExecutorEngine.registerModule(StoreRef, *Module, Name));

#ifdef WASMEDGE_USE_LLVM
  if (LazyEngine) {
    LazyEngine->registerInstance(*ModInst, std::move(Module));
  }
#endif

  RegModInsts.push_back(std::move(ModInst));
  return {};
}

Expect<void>
VM::unsafeRegisterModule(std::string_view Name,
                         const Runtime::Instance::ModuleInstance &ModInst) {
  unsafeRevertStageToValidated();
  return ExecutorEngine.registerModule(StoreRef, ModInst, Name);
}

Expect<void> VM::unsafeUnregisterModule(std::string_view Name) {
  auto InstIt = std::find_if(
      RegModInsts.begin(), RegModInsts.end(),
      [&Name](const std::unique_ptr<Runtime::Instance::ModuleInstance> &Inst) {
        return Inst && Inst->getModuleName() == Name;
      });
  if (InstIt != RegModInsts.end()) {
    auto *ModInst = (*InstIt).release();

    if (ModInst) {
#ifdef WASMEDGE_USE_LLVM
      if (LazyEngine) {
        LazyEngine->unregisterInstance(*ModInst);
      }
#endif
      ModInst->terminate();
    }
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

VM::WasmUnitKind VM::unsafeStoreWasmUnit(
    std::variant<std::unique_ptr<AST::Component::Component>,
                 std::unique_ptr<AST::Module>> &&Unit) {
  if (auto *M = std::get_if<std::unique_ptr<AST::Module>>(&Unit)) {
    Mod = std::move(*M);
    return WasmUnitKind::Module;
  }
  Comp = std::move(std::get<std::unique_ptr<AST::Component::Component>>(Unit));
  return WasmUnitKind::Component;
}

Expect<std::vector<std::pair<ValVariant, ValType>>>
VM::unsafeRunWasmFile(const std::filesystem::path &Path, std::string_view Func,
                      Span<const ValVariant> Params,
                      Span<const ValType> ParamTypes) {
  unsafeRevertStageToValidated();
  // Load wasm unit.
  EXPECTED_TRY(auto ComponentOrModule, LoaderEngine.parseWasmUnit(Path));
  if (unsafeStoreWasmUnit(std::move(ComponentOrModule)) ==
      WasmUnitKind::Component) {
    return unsafeRunWasmFile(*Comp, Func, Params, ParamTypes);
  }
  return unsafeRunWasmFile(*Mod, Func, Params, ParamTypes);
}

Expect<std::vector<std::pair<ValVariant, ValType>>>
VM::unsafeRunWasmFile(Span<const Byte> Code, std::string_view Func,
                      Span<const ValVariant> Params,
                      Span<const ValType> ParamTypes) {
  unsafeRevertStageToValidated();
  // Load wasm unit.
  EXPECTED_TRY(auto ComponentOrModule, LoaderEngine.parseWasmUnit(Code));
  if (unsafeStoreWasmUnit(std::move(ComponentOrModule)) ==
      WasmUnitKind::Component) {
    return unsafeRunWasmFile(*Comp, Func, Params, ParamTypes);
  }
  return unsafeRunWasmFile(*Mod, Func, Params, ParamTypes);
}

Expect<std::vector<std::pair<ValVariant, ValType>>>
VM::unsafeRunWasmFile(const AST::Component::Component &Component,
                      std::string_view, Span<const ValVariant>,
                      Span<const ValType>) {
  unsafeRevertStageToValidated();
  EXPECTED_TRY(ValidatorEngine.validate(Component));
  spdlog::error("component execution is not done yet."sv);
  return Unexpect(ErrCode::Value::RuntimeError);
}

Expect<std::vector<std::pair<ValVariant, ValType>>>
VM::unsafeRunWasmFile(const AST::Module &Module, std::string_view Func,
                      Span<const ValVariant> Params,
                      Span<const ValType> ParamTypes) {
  unsafeRevertStageToValidated();
  EXPECTED_TRY(ValidatorEngine.validate(Module));
#ifdef WASMEDGE_USE_LLVM
  if (LazyEngine) {
    // This one-shot path takes no shared ownership of the module, so it is
    // not bound to the lazy JIT engine and executes in the interpreter.
    spdlog::debug("[lazy-jit]: runWasmFile executes in interpreter mode"sv);
  }
#endif
  EXPECTED_TRY(auto NewModInst,
               ExecutorEngine.instantiateModule(StoreRef, Module));
#ifdef WASMEDGE_USE_LLVM
  // Drop the lazy binding of the replaced instance only after the new
  // instantiation succeeds, so a failed run keeps the current instance bound.
  if (LazyEngine && ActiveModInst) {
    LazyEngine->unregisterInstance(*ActiveModInst);
  }
#endif
  ActiveModInst = std::move(NewModInst);

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
  unsafeStoreWasmUnit(std::move(ComponentOrModule));
  Stage = VMStage::Loaded;
  return {};
}

Expect<void> VM::unsafeLoadWasm(Span<const Byte> Code) {
  // If loading does not succeed, the previous status will be preserved.
  EXPECTED_TRY(auto ComponentOrModule, LoaderEngine.parseWasmUnit(Code));
  unsafeStoreWasmUnit(std::move(ComponentOrModule));
  Stage = VMStage::Loaded;
  return {};
}

Expect<void> VM::unsafeLoadWasm(const AST::Module &Module) {
  Mod = std::make_shared<AST::Module>(Module);
  Stage = VMStage::Loaded;
  return {};
}

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
    if ((Conf.getRuntimeConfigure().getRunMode() == RunMode::JIT ||
         Conf.getRuntimeConfigure().getRunMode() == RunMode::LazyJIT) &&
        !Mod->getSymbol()) {
#ifdef WASMEDGE_USE_LLVM
      if (LazyEngine) {
        EXPECTED_TRY(auto Exec, LazyEngine->prepare(Mod));
        EXPECTED_TRY(LoaderEngine.loadExecutable(*Mod, std::move(Exec)));
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
              return JIT.load(std::move(LLModule));
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

    EXPECTED_TRY(auto NewModInst,
                 ExecutorEngine.instantiateModule(StoreRef, *Mod));

#ifdef WASMEDGE_USE_LLVM
    if (LazyEngine) {
      // Rebind the lazy JIT state only after instantiation succeeds, so a
      // failed re-instantiation keeps the current instance bound.
      if (ActiveModInst) {
        LazyEngine->unregisterInstance(*ActiveModInst);
      }
      LazyEngine->registerInstance(*NewModInst, Mod);
    }
#endif
    ActiveModInst = std::move(NewModInst);

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
  // Lazy JIT: compile the function on-demand if needed.
  if (LazyEngine) {
    EXPECTED_TRY(LazyEngine->compileOnDemand(FuncInst));
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
  Stat.clear();
  unsafeLoadBuiltInHosts();
  unsafeLoadPlugInHosts();
  unsafeRegisterBuiltInHosts();
  unsafeRegisterPlugInHosts();
  LoaderEngine.reset();
  Stage = VMStage::Inited;
#ifdef WASMEDGE_USE_LLVM
  if (LazyEngine) {
    LazyEngine->clear();
  }
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

} // namespace VM
} // namespace WasmEdge
