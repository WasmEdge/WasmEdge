// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "vm/vm.h"
#include "vm/async.h"

#include "host/wasi/wasimodule.h"
#include "plugin/plugin.h"

#include "host/mock/wasi_crypto_module.h"
#include "host/mock/wasi_nn_module.h"
#include "host/mock/wasmedge_httpsreq_module.h"
#include "host/mock/wasmedge_process_module.h"

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
  spdlog::debug("Plugin: "sv, PName, " , module name: "sv, MName,
                "not found. Mock instead."sv);
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

  // Register all module instances.
  unsafeRegisterBuiltInHosts();
  unsafeRegisterPlugInHosts();
}

void VM::unsafeLoadBuiltInHosts() {
  // Load the built-in host modules from configuration.
  // TODO: This will be extended for the versionlized WASI in the future.
  BuiltInModInsts.clear();
  if (Conf.hasHostRegistration(HostRegistration::Wasi)) {
    std::unique_ptr<Runtime::Instance::ModuleInstance> WasiMod =
        std::make_unique<Host::WasiModule>();
    BuiltInModInsts.insert({HostRegistration::Wasi, std::move(WasiMod)});
  }
}

void VM::unsafeLoadPlugInHosts() {
  // Load the plugins and mock them if not found.
  using namespace std::literals::string_view_literals;
  PlugInModInsts.clear();

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
  PlugInModInsts.push_back(createPluginModule<Host::WasmEdgeHttpsReqModuleMock>(
      "wasmedge_httpsreq"sv, "wasmedge_httpsreq"sv));

  // Load the other non-official plugins.
  for (const auto &Plugin : Plugin::Plugin::plugins()) {
    if (Conf.isForbiddenPlugins(Plugin.name())) {
      continue;
    }
    // Skip WasmEdge_Process, WasmEdge_HttpsReq, wasi_nn, and wasi_crypto.
    if (Plugin.name() == "wasmedge_process"sv ||
        Plugin.name() == "wasmedge_httpsreq"sv ||
        Plugin.name() == "wasi_nn"sv || Plugin.name() == "wasi_crypto"sv) {
      continue;
    }
    for (const auto &Module : Plugin.modules()) {
      PlugInModInsts.push_back(Module.create());
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
    RegModInsts.push_back(std::move(*Res));
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
    spdlog::error(ErrCode::Value::WrongInstanceAddress);
    spdlog::error(ErrInfo::InfoExecuting("", Func));
    return Unexpect(ErrCode::Value::WrongInstanceAddress);
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
    spdlog::error(ErrCode::Value::WrongVMWorkflow);
    return Unexpect(ErrCode::Value::WrongVMWorkflow);
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
    spdlog::error(ErrCode::Value::WrongVMWorkflow);
    return Unexpect(ErrCode::Value::WrongVMWorkflow);
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
    spdlog::error(ErrCode::Value::WrongInstanceAddress);
    spdlog::error(ErrInfo::InfoExecuting("", Func));
    return Unexpect(ErrCode::Value::WrongInstanceAddress);
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
    spdlog::error(ErrCode::Value::WrongInstanceAddress);
    spdlog::error(ErrInfo::InfoExecuting(ModName, Func));
    return Unexpect(ErrCode::Value::WrongInstanceAddress);
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
    spdlog::error(ErrCode::Value::FuncNotFound);
    spdlog::error(ErrInfo::InfoExecuting(ModInst->getModuleName(), Func));
    return Unexpect(ErrCode::Value::FuncNotFound);
  }

  // Execute function.
  if (auto Res = ExecutorEngine.invoke(*FuncInst, Params, ParamTypes);
      unlikely(!Res)) {
    if (Res.error() != ErrCode::Value::Terminated) {
      spdlog::error(ErrInfo::InfoExecuting(ModInst->getModuleName(), Func));
    }
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
  StoreRef.reset();
  RegModInsts.clear();
  Stat.clear();
  unsafeLoadBuiltInHosts();
  unsafeLoadPlugInHosts();
  unsafeRegisterBuiltInHosts();
  unsafeRegisterPlugInHosts();
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
