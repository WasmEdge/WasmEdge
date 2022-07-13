// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "vm/vm.h"
#include "vm/async.h"

#include "host/wasi/wasimodule.h"
#ifdef WASMEDGE_BUILD_WASI_CRYPTO
#include "host/wasi_crypto/module.h"
#endif
#include "plugin/plugin.h"

namespace WasmEdge {
namespace VM {

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
  using namespace std::literals::string_view_literals;
  // Create import modules from configuration.
  if (Conf.hasHostRegistration(HostRegistration::Wasi)) {
    std::unique_ptr<Runtime::Instance::ModuleInstance> WasiMod =
        std::make_unique<Host::WasiModule>();
    ExecutorEngine.registerModule(StoreRef, *WasiMod.get());
    ImpObjs.insert({HostRegistration::Wasi, std::move(WasiMod)});
  }
  if (Conf.hasHostRegistration(HostRegistration::WasmEdge_Process)) {
    bool Founded = false;
    if (const auto *Plugin = Plugin::Plugin::find("wasmedge_process"sv)) {
      if (const auto *Module = Plugin->findModule("wasmedge_process"sv)) {
        auto ProcMod = Module->create();
        ExecutorEngine.registerModule(StoreRef, *ProcMod);
        ImpObjs.emplace(HostRegistration::WasmEdge_Process, std::move(ProcMod));
        Founded = true;
      }
    }
    if (!Founded) {
      spdlog::debug("wasmedge_process module not founded."sv);
    }
  }
  if (Conf.hasHostRegistration(HostRegistration::WasiNN)) {
    bool Founded = false;
    if (const auto *Plugin = Plugin::Plugin::find("wasi_nn"sv)) {
      if (const auto *Module = Plugin->findModule("wasi_nn"sv)) {
        auto ProcMod = Module->create();
        ExecutorEngine.registerModule(StoreRef, *ProcMod);
        ImpObjs.emplace(HostRegistration::WasiNN, std::move(ProcMod));
        Founded = true;
      }
    }
    if (!Founded) {
      spdlog::debug("wasi_nn module not founded."sv);
    }
  }
  uint8_t Index = static_cast<uint8_t>(HostRegistration::Max);
  for (const auto &Plugin : Plugin::Plugin::plugins()) {
    if (Conf.isForbiddenPlugins(Plugin.name())) {
      continue;
    }
    // skip WasmEdge_Process and wasi_nn
    if (Plugin.name() == "wasmedge_process"sv || Plugin.name() == "wasi_nn"sv) {
      continue;
    }
    for (const auto &Module : Plugin.modules()) {
      auto ModObj = Module.create();
      ExecutorEngine.registerModule(StoreRef, *ModObj);
      ImpObjs.emplace(static_cast<HostRegistration>(Index++),
                      std::move(ModObj));
    }
  }
#ifdef WASMEDGE_BUILD_WASI_CRYPTO
  if (Conf.hasHostRegistration(HostRegistration::Wasi_Crypto)) {
    std::unique_ptr<Runtime::Instance::ModuleInstance> WasiCryptoMod =
        std::make_unique<Host::WasiCryptoModule>();
    ExecutorEngine.registerModule(StoreRef, *WasiCryptoMod.get());
    ImpObjs.insert({HostRegistration::Wasi_Crypto, std::move(WasiCryptoMod)});
  }
#endif
}

Expect<void> VM::unsafeRegisterModule(std::string_view Name,
                                      const std::filesystem::path &Path) {
  if (Stage == VMStage::Instantiated) {
    // When registering module, instantiated module in store will be reset.
    // Therefore the instantiation should restart.
    Stage = VMStage::Validated;
  }
  // Load module.
  if (auto Res = LoaderEngine.parseModule(Path)) {
    return unsafeRegisterModule(Name, *(*Res).get());
  } else {
    return Unexpect(Res);
  }
}

Expect<void> VM::unsafeRegisterModule(std::string_view Name,
                                      Span<const Byte> Code) {
  if (Stage == VMStage::Instantiated) {
    // When registering module, instantiated module in store will be reset.
    // Therefore the instantiation should restart.
    Stage = VMStage::Validated;
  }
  // Load module.
  if (auto Res = LoaderEngine.parseModule(Code)) {
    return unsafeRegisterModule(Name, *(*Res).get());
  } else {
    return Unexpect(Res);
  }
}

Expect<void> VM::unsafeRegisterModule(std::string_view Name,
                                      const AST::Module &Module) {
  if (Stage == VMStage::Instantiated) {
    // When registering module, instantiated module in store will be reset.
    // Therefore the instantiation should restart.
    Stage = VMStage::Validated;
  }
  // Validate module.
  if (auto Res = ValidatorEngine.validate(Module); !Res) {
    return Unexpect(Res);
  }
  // Instantiate and register module.
  if (auto Res = ExecutorEngine.registerModule(StoreRef, Module, Name)) {
    RegModInst.push_back(std::move(*Res));
    return {};
  } else {
    return Unexpect(Res);
  }
}

Expect<void>
VM::unsafeRegisterModule(const Runtime::Instance::ModuleInstance &ModInst) {
  if (Stage == VMStage::Instantiated) {
    // When registering module, instantiated module in store will be reset.
    // Therefore the instantiation should restart.
    Stage = VMStage::Validated;
  }
  return ExecutorEngine.registerModule(StoreRef, ModInst);
}

Expect<std::vector<std::pair<ValVariant, ValType>>>
VM::unsafeRunWasmFile(const std::filesystem::path &Path, std::string_view Func,
                      Span<const ValVariant> Params,
                      Span<const ValType> ParamTypes) {
  if (Stage == VMStage::Instantiated) {
    // When running another module, instantiated module in store will be reset.
    // Therefore the instantiation should restart.
    Stage = VMStage::Validated;
  }
  // Load module.
  if (auto Res = LoaderEngine.parseModule(Path)) {
    return unsafeRunWasmFile(*(*Res).get(), Func, Params, ParamTypes);
  } else {
    return Unexpect(Res);
  }
}

Expect<std::vector<std::pair<ValVariant, ValType>>>
VM::unsafeRunWasmFile(Span<const Byte> Code, std::string_view Func,
                      Span<const ValVariant> Params,
                      Span<const ValType> ParamTypes) {
  if (Stage == VMStage::Instantiated) {
    // When running another module, instantiated module in store will be reset.
    // Therefore the instantiation should restart.
    Stage = VMStage::Validated;
  }
  // Load module.
  if (auto Res = LoaderEngine.parseModule(Code)) {
    return unsafeRunWasmFile(*(*Res).get(), Func, Params, ParamTypes);
  } else {
    return Unexpect(Res);
  }
}

Expect<std::vector<std::pair<ValVariant, ValType>>>
VM::unsafeRunWasmFile(const AST::Module &Module, std::string_view Func,
                      Span<const ValVariant> Params,
                      Span<const ValType> ParamTypes) {
  if (Stage == VMStage::Instantiated) {
    // When running another module, instantiated module in store will be reset.
    // Therefore the instantiation should restart.
    Stage = VMStage::Validated;
  }
  if (auto Res = ValidatorEngine.validate(Module); !Res) {
    return Unexpect(Res);
  }
  if (auto Res = ExecutorEngine.instantiateModule(StoreRef, Module)) {
    ActiveModInst = std::move(*Res);
  } else {
    return Unexpect(Res);
  }
  // Get module instance.
  if (ActiveModInst) {
    // Execute function and return values with the module instance.
    return unsafeExecute(ActiveModInst.get(), Func, Params, ParamTypes);
  } else {
    spdlog::error(ErrCode::WrongInstanceAddress);
    spdlog::error(ErrInfo::InfoExecuting("", Func));
    return Unexpect(ErrCode::WrongInstanceAddress);
  }
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
  // If not load successfully, the previous status will be reserved.
  if (auto Res = LoaderEngine.parseModule(Path)) {
    Mod = std::move(*Res);
    Stage = VMStage::Loaded;
  } else {
    return Unexpect(Res);
  }
  return {};
}

Expect<void> VM::unsafeLoadWasm(Span<const Byte> Code) {
  // If not load successfully, the previous status will be reserved.
  if (auto Res = LoaderEngine.parseModule(Code)) {
    Mod = std::move(*Res);
    Stage = VMStage::Loaded;
  } else {
    return Unexpect(Res);
  }
  return {};
}

Expect<void> VM::unsafeLoadWasm(const AST::Module &Module) {
  Mod = std::make_unique<AST::Module>(Module);
  Stage = VMStage::Loaded;
  return {};
}

Expect<void> VM::unsafeValidate() {
  if (Stage < VMStage::Loaded) {
    // When module is not loaded, not validate.
    spdlog::error(ErrCode::WrongVMWorkflow);
    return Unexpect(ErrCode::WrongVMWorkflow);
  }
  if (auto Res = ValidatorEngine.validate(*Mod.get())) {
    Stage = VMStage::Validated;
    return {};
  } else {
    return Unexpect(Res);
  }
}

Expect<void> VM::unsafeInstantiate() {
  if (Stage < VMStage::Validated) {
    // When module is not validated, not instantiate.
    spdlog::error(ErrCode::WrongVMWorkflow);
    return Unexpect(ErrCode::WrongVMWorkflow);
  }
  if (auto Res = ExecutorEngine.instantiateModule(StoreRef, *Mod.get())) {
    Stage = VMStage::Instantiated;
    ActiveModInst = std::move(*Res);
    return {};
  } else {
    return Unexpect(Res);
  }
}

Expect<std::vector<std::pair<ValVariant, ValType>>>
VM::unsafeExecute(std::string_view Func, Span<const ValVariant> Params,
                  Span<const ValType> ParamTypes) {
  if (ActiveModInst) {
    // Execute function and return values with the module instance.
    return unsafeExecute(ActiveModInst.get(), Func, Params, ParamTypes);
  } else {
    spdlog::error(ErrCode::WrongInstanceAddress);
    spdlog::error(ErrInfo::InfoExecuting("", Func));
    return Unexpect(ErrCode::WrongInstanceAddress);
  }
}

Expect<std::vector<std::pair<ValVariant, ValType>>>
VM::unsafeExecute(std::string_view ModName, std::string_view Func,
                  Span<const ValVariant> Params,
                  Span<const ValType> ParamTypes) {
  // Find module instance by name.
  const auto *FindModInst = StoreRef.findModule(ModName);
  if (FindModInst != nullptr) {
    // Execute function and return values with the module instance.
    return unsafeExecute(FindModInst, Func, Params, ParamTypes);
  } else {
    spdlog::error(ErrCode::WrongInstanceAddress);
    spdlog::error(ErrInfo::InfoExecuting(ModName, Func));
    return Unexpect(ErrCode::WrongInstanceAddress);
  }
}

Expect<std::vector<std::pair<ValVariant, ValType>>>
VM::unsafeExecute(const Runtime::Instance::ModuleInstance *ModInst,
                  std::string_view Func, Span<const ValVariant> Params,
                  Span<const ValType> ParamTypes) {
  // Find exported function by name.
  Runtime::Instance::FunctionInstance *FuncInst =
      ModInst->findFuncExports(Func);
  if (unlikely(FuncInst == nullptr)) {
    spdlog::error(ErrCode::FuncNotFound);
    spdlog::error(ErrInfo::InfoExecuting(ModInst->getModuleName(), Func));
    return Unexpect(ErrCode::FuncNotFound);
  }

  // Execute function.
  if (auto Res = ExecutorEngine.invoke(*FuncInst, Params, ParamTypes);
      unlikely(!Res)) {
    spdlog::error(ErrInfo::InfoExecuting(ModInst->getModuleName(), Func));
    return Unexpect(Res);
  } else {
    return Res;
  }
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

void VM::unsafeCleanup() {
  Mod.reset();
  ActiveModInst.reset();
  Stat.clear();
  Stage = VMStage::Inited;
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

Runtime::Instance::ModuleInstance *
VM::unsafeGetImportModule(const HostRegistration Type) const {
  if (auto Iter = ImpObjs.find(Type); Iter != ImpObjs.cend()) {
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
