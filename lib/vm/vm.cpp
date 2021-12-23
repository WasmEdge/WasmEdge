// SPDX-License-Identifier: Apache-2.0

#include "vm/vm.h"
#include "vm/async.h"

#include "host/wasi/wasimodule.h"
#include "host/wasmedge_process/processmodule.h"

namespace WasmEdge {
namespace VM {

VM::VM(const Configure &Conf)
    : Conf(Conf), Stage(VMStage::Inited),
      LoaderEngine(Conf, &Executor::Executor::Intrinsics),
      ValidatorEngine(Conf), ExecutorEngine(Conf, &Stat),
      Store(std::make_unique<Runtime::StoreManager>()), StoreRef(*Store.get()) {
  initVM();
}

VM::VM(const Configure &Conf, Runtime::StoreManager &S)
    : Conf(Conf), Stage(VMStage::Inited),
      LoaderEngine(Conf, &Executor::Executor::Intrinsics),
      ValidatorEngine(Conf), ExecutorEngine(Conf, &Stat), StoreRef(S) {
  initVM();
}

void VM::initVM() {
  /// Create import modules from configuration.
  if (Conf.hasHostRegistration(HostRegistration::Wasi)) {
    std::unique_ptr<Runtime::ImportObject> WasiMod =
        std::make_unique<Host::WasiModule>();
    ExecutorEngine.registerModule(StoreRef, *WasiMod.get());
    ImpObjs.insert({HostRegistration::Wasi, std::move(WasiMod)});
  }
  if (Conf.hasHostRegistration(HostRegistration::WasmEdge_Process)) {
    std::unique_ptr<Runtime::ImportObject> ProcMod =
        std::make_unique<Host::WasmEdgeProcessModule>();
    ExecutorEngine.registerModule(StoreRef, *ProcMod.get());
    ImpObjs.insert({HostRegistration::WasmEdge_Process, std::move(ProcMod)});
  }
}

Expect<void> VM::registerModule(std::string_view Name,
                                const std::filesystem::path &Path) {
  if (Stage == VMStage::Instantiated) {
    /// When registering module, instantiated module in store will be reset.
    /// Therefore the instantiation should restart.
    Stage = VMStage::Validated;
  }
  /// Load module.
  if (auto Res = LoaderEngine.parseModule(Path)) {
    return registerModule(Name, *(*Res).get());
  } else {
    return Unexpect(Res);
  }
}

Expect<void> VM::registerModule(std::string_view Name, Span<const Byte> Code) {
  if (Stage == VMStage::Instantiated) {
    /// When registering module, instantiated module in store will be reset.
    /// Therefore the instantiation should restart.
    Stage = VMStage::Validated;
  }
  /// Load module.
  if (auto Res = LoaderEngine.parseModule(Code)) {
    return registerModule(Name, *(*Res).get());
  } else {
    return Unexpect(Res);
  }
}

Expect<void> VM::registerModule(const Runtime::ImportObject &Obj) {
  if (Stage == VMStage::Instantiated) {
    /// When registering module, instantiated module in store will be reset.
    /// Therefore the instantiation should restart.
    Stage = VMStage::Validated;
  }
  return ExecutorEngine.registerModule(StoreRef, Obj);
}

Expect<void> VM::registerModule(std::string_view Name,
                                const AST::Module &Module) {
  if (Stage == VMStage::Instantiated) {
    /// When registering module, instantiated module in store will be reset.
    /// Therefore the instantiation should restart.
    Stage = VMStage::Validated;
  }
  /// Validate module.
  if (auto Res = ValidatorEngine.validate(Module); !Res) {
    return Unexpect(Res);
  }
  return ExecutorEngine.registerModule(StoreRef, Module, Name);
}

Expect<std::vector<std::pair<ValVariant, ValType>>>
VM::runWasmFile(const std::filesystem::path &Path, std::string_view Func,
                Span<const ValVariant> Params, Span<const ValType> ParamTypes) {
  if (Stage == VMStage::Instantiated) {
    /// When running another module, instantiated module in store will be reset.
    /// Therefore the instantiation should restart.
    Stage = VMStage::Validated;
  }
  /// Load module.
  if (auto Res = LoaderEngine.parseModule(Path)) {
    return runWasmFile(*(*Res).get(), Func, Params, ParamTypes);
  } else {
    return Unexpect(Res);
  }
}

Expect<std::vector<std::pair<ValVariant, ValType>>>
VM::runWasmFile(Span<const Byte> Code, std::string_view Func,
                Span<const ValVariant> Params, Span<const ValType> ParamTypes) {
  if (Stage == VMStage::Instantiated) {
    /// When running another module, instantiated module in store will be reset.
    /// Therefore the instantiation should restart.
    Stage = VMStage::Validated;
  }
  /// Load module.
  if (auto Res = LoaderEngine.parseModule(Code)) {
    return runWasmFile(*(*Res).get(), Func, Params, ParamTypes);
  } else {
    return Unexpect(Res);
  }
}

Expect<std::vector<std::pair<ValVariant, ValType>>>
VM::runWasmFile(const AST::Module &Module, std::string_view Func,
                Span<const ValVariant> Params, Span<const ValType> ParamTypes) {
  if (Stage == VMStage::Instantiated) {
    /// When running another module, instantiated module in store will be reset.
    /// Therefore the instantiation should restart.
    Stage = VMStage::Validated;
  }
  if (auto Res = ValidatorEngine.validate(Module); !Res) {
    return Unexpect(Res);
  }
  if (auto Res = ExecutorEngine.instantiateModule(StoreRef, Module); !Res) {
    return Unexpect(Res);
  }
  /// Get module instance.
  if (auto Res = StoreRef.getActiveModule()) {
    /// Execute function and return values with the module instance.
    return execute(*Res, Func, Params, ParamTypes);
  } else {
    spdlog::error(Res.error());
    spdlog::error(ErrInfo::InfoExecuting("", Func));
    return Unexpect(Res);
  }
}

Async<Expect<std::vector<std::pair<ValVariant, ValType>>>>
VM::asyncRunWasmFile(const std::filesystem::path &Path, std::string_view Func,
                     Span<const ValVariant> Params,
                     Span<const ValType> ParamTypes) {
  Expect<std::vector<std::pair<ValVariant, ValType>>> (VM::*FPtr)(
      const std::filesystem::path &, std::string_view, Span<const ValVariant>,
      Span<const ValType>) = &VM::runWasmFile;
  return {FPtr, *this, std::filesystem::path(Path), Func, Params, ParamTypes};
}

Async<Expect<std::vector<std::pair<ValVariant, ValType>>>>
VM::asyncRunWasmFile(Span<const Byte> Code, std::string_view Func,
                     Span<const ValVariant> Params,
                     Span<const ValType> ParamTypes) {
  Expect<std::vector<std::pair<ValVariant, ValType>>> (VM::*FPtr)(
      Span<const Byte>, std::string_view, Span<const ValVariant>,
      Span<const ValType>) = &VM::runWasmFile;
  return {FPtr, *this, Code, Func, Params, ParamTypes};
}

Async<Expect<std::vector<std::pair<ValVariant, ValType>>>>
VM::asyncRunWasmFile(const AST::Module &Module, std::string_view Func,
                     Span<const ValVariant> Params,
                     Span<const ValType> ParamTypes) {
  Expect<std::vector<std::pair<ValVariant, ValType>>> (VM::*FPtr)(
      const AST::Module &, std::string_view, Span<const ValVariant>,
      Span<const ValType>) = &VM::runWasmFile;
  return {FPtr, *this, Module, Func, Params, ParamTypes};
}

Expect<void> VM::loadWasm(const std::filesystem::path &Path) {
  /// If not load successfully, the previous status will be reserved.
  if (auto Res = LoaderEngine.parseModule(Path)) {
    Mod = std::move(*Res);
    Stage = VMStage::Loaded;
  } else {
    return Unexpect(Res);
  }
  return {};
}

Expect<void> VM::loadWasm(Span<const Byte> Code) {
  /// If not load successfully, the previous status will be reserved.
  if (auto Res = LoaderEngine.parseModule(Code)) {
    Mod = std::move(*Res);
    Stage = VMStage::Loaded;
  } else {
    return Unexpect(Res);
  }
  return {};
}

Expect<void> VM::loadWasm(const AST::Module &Module) {
  Mod = std::make_unique<AST::Module>(Module);
  Stage = VMStage::Loaded;
  return {};
}

Expect<void> VM::validate() {
  if (Stage < VMStage::Loaded) {
    /// When module is not loaded, not validate.
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

Expect<void> VM::instantiate() {
  if (Stage < VMStage::Validated) {
    /// When module is not validated, not instantiate.
    spdlog::error(ErrCode::WrongVMWorkflow);
    return Unexpect(ErrCode::WrongVMWorkflow);
  }
  if (auto Res = ExecutorEngine.instantiateModule(StoreRef, *Mod.get())) {
    Stage = VMStage::Instantiated;
    return {};
  } else {
    return Unexpect(Res);
  }
}

Expect<std::vector<std::pair<ValVariant, ValType>>>
VM::execute(std::string_view Func, Span<const ValVariant> Params,
            Span<const ValType> ParamTypes) {
  /// Get module instance.
  if (auto Res = StoreRef.getActiveModule()) {
    /// Execute function and return values with the module instance.
    return execute(*Res, Func, Params, ParamTypes);
  } else {
    spdlog::error(Res.error());
    spdlog::error(ErrInfo::InfoExecuting("", Func));
    return Unexpect(Res);
  }
}

Expect<std::vector<std::pair<ValVariant, ValType>>>
VM::execute(std::string_view ModName, std::string_view Func,
            Span<const ValVariant> Params, Span<const ValType> ParamTypes) {
  /// Get module instance.
  if (auto Res = StoreRef.findModule(ModName)) {
    /// Execute function and return values with the module instance.
    return execute(*Res, Func, Params, ParamTypes);
  } else {
    spdlog::error(Res.error());
    spdlog::error(ErrInfo::InfoExecuting(ModName, Func));
    return Unexpect(Res);
  }
}

Expect<std::vector<std::pair<ValVariant, ValType>>>
VM::execute(Runtime::Instance::ModuleInstance *ModInst, std::string_view Func,
            Span<const ValVariant> Params, Span<const ValType> ParamTypes) {
  /// Get exports and find function.
  const auto &FuncExp = ModInst->getFuncExports();
  const auto FuncIter = FuncExp.find(Func);
  if (FuncIter == FuncExp.cend()) {
    spdlog::error(ErrCode::FuncNotFound);
    spdlog::error(ErrInfo::InfoExecuting(ModInst->getModuleName(), Func));
    return Unexpect(ErrCode::FuncNotFound);
  }

  /// Execute function.
  if (auto Res = ExecutorEngine.invoke(StoreRef, FuncIter->second, Params,
                                       ParamTypes)) {
    return Res;
  } else {
    spdlog::error(ErrInfo::InfoExecuting(ModInst->getModuleName(), Func));
    return Unexpect(Res);
  }
}

Async<Expect<std::vector<std::pair<ValVariant, ValType>>>>
VM::asyncExecute(std::string_view Func, Span<const ValVariant> Params,
                 Span<const ValType> ParamTypes) {
  Expect<std::vector<std::pair<ValVariant, ValType>>> (VM::*FPtr)(
      std::string_view, Span<const ValVariant>, Span<const ValType>) =
      &VM::execute;
  return {FPtr, *this, Func, Params, ParamTypes};
}

Async<Expect<std::vector<std::pair<ValVariant, ValType>>>>
VM::asyncExecute(std::string_view ModName, std::string_view Func,
                 Span<const ValVariant> Params,
                 Span<const ValType> ParamTypes) {
  Expect<std::vector<std::pair<ValVariant, ValType>>> (VM::*FPtr)(
      std::string_view, std::string_view, Span<const ValVariant>,
      Span<const ValType>) = &VM::execute;
  return {FPtr, *this, ModName, Func, Params, ParamTypes};
}

void VM::cleanup() {
  Mod.reset();
  StoreRef.reset();
  Stat.clear();
  Stage = VMStage::Inited;
}

std::vector<std::pair<std::string, const AST::FunctionType &>>
VM::getFunctionList() const {
  std::vector<std::pair<std::string, const AST::FunctionType &>> Map;
  /// Get the active module instance.
  if (auto Res = StoreRef.getActiveModule()) {
    for (auto &&Func : (*Res)->getFuncExports()) {
      const auto *FuncInst = *StoreRef.getFunction(Func.second);
      const auto &FuncType = FuncInst->getFuncType();
      Map.push_back({Func.first, FuncType});
    }
  }
  return Map;
}

Runtime::ImportObject *VM::getImportModule(const HostRegistration Type) {
  if (ImpObjs.find(Type) != ImpObjs.cend()) {
    return ImpObjs[Type].get();
  }
  return nullptr;
}

} // namespace VM
} // namespace WasmEdge
