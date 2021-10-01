// SPDX-License-Identifier: Apache-2.0

#include "vm/vm.h"

#include "host/wasi/wasimodule.h"
#include "host/wasmedge_process/processmodule.h"

namespace WasmEdge {
namespace VM {

VM::VM(const Configure &Conf)
    : Conf(Conf), Stage(VMStage::Inited),
      LoaderEngine(Conf, &Interpreter::Interpreter::Intrinsics),
      ValidatorEngine(Conf), InterpreterEngine(Conf, &Stat),
      Store(std::make_unique<Runtime::StoreManager>()), StoreRef(*Store.get()) {
  initVM();
}

VM::VM(const Configure &Conf, Runtime::StoreManager &S)
    : Conf(Conf), Stage(VMStage::Inited), LoaderEngine(Conf),
      ValidatorEngine(Conf), InterpreterEngine(Conf, &Stat), StoreRef(S) {
  initVM();
}

void VM::initVM() {
  /// Create import modules from configuration.
  if (Conf.hasHostRegistration(HostRegistration::Wasi)) {
    std::unique_ptr<Runtime::ImportObject> WasiMod =
        std::make_unique<Host::WasiModule>();
    InterpreterEngine.registerModule(StoreRef, *WasiMod.get());
    ImpObjs.insert({HostRegistration::Wasi, std::move(WasiMod)});
  }
  if (Conf.hasHostRegistration(HostRegistration::WasmEdge_Process)) {
    std::unique_ptr<Runtime::ImportObject> ProcMod =
        std::make_unique<Host::WasmEdgeProcessModule>();
    InterpreterEngine.registerModule(StoreRef, *ProcMod.get());
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
  return InterpreterEngine.registerModule(StoreRef, Obj);
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
  return InterpreterEngine.registerModule(StoreRef, Module, Name);
}

Expect<std::vector<ValVariant>>
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

Expect<std::vector<ValVariant>>
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

Expect<std::vector<ValVariant>>
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
  if (auto Res = InterpreterEngine.instantiateModule(StoreRef, Module); !Res) {
    return Unexpect(Res);
  }
  const auto FuncExp = StoreRef.getFuncExports();
  if (FuncExp.find(Func) == FuncExp.cend()) {
    spdlog::error(ErrCode::FuncNotFound);
    spdlog::error(ErrInfo::InfoExecuting("", Func));
    return Unexpect(ErrCode::FuncNotFound);
  }
  if (auto Res = InterpreterEngine.invoke(StoreRef, FuncExp.find(Func)->second,
                                          Params, ParamTypes)) {
    return *Res;
  } else {
    return Unexpect(Res);
  }
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
  if (auto Res = InterpreterEngine.instantiateModule(StoreRef, *Mod.get())) {
    Stage = VMStage::Instantiated;
    return {};
  } else {
    return Unexpect(Res);
  }
}

Expect<std::vector<ValVariant>> VM::execute(std::string_view Func,
                                            Span<const ValVariant> Params,
                                            Span<const ValType> ParamTypes) {
  /// Check exports for finding function address.
  const auto FuncExp = StoreRef.getFuncExports();
  const auto FuncIter = FuncExp.find(Func);
  if (FuncIter == FuncExp.cend()) {
    spdlog::error(ErrCode::FuncNotFound);
    spdlog::error(ErrInfo::InfoExecuting("", Func));
    return Unexpect(ErrCode::FuncNotFound);
  }

  /// Execute function.
  if (auto Res = InterpreterEngine.invoke(StoreRef, FuncIter->second, Params,
                                          ParamTypes)) {
    return Res;
  } else {
    spdlog::error(ErrInfo::InfoExecuting("", Func));
    return Unexpect(Res);
  }
}

Expect<std::vector<ValVariant>> VM::execute(std::string_view ModName,
                                            std::string_view Func,
                                            Span<const ValVariant> Params,
                                            Span<const ValType> ParamTypes) {
  /// Get module instance.
  Runtime::Instance::ModuleInstance *ModInst;
  if (auto Res = StoreRef.findModule(ModName)) {
    ModInst = *Res;
  } else {
    spdlog::error(Res.error());
    spdlog::error(ErrInfo::InfoExecuting(ModName, Func));
    return Unexpect(Res);
  }

  /// Get exports and find function.
  const auto FuncExp = ModInst->getFuncExports();
  const auto FuncIter = FuncExp.find(Func);
  if (FuncIter == FuncExp.cend()) {
    spdlog::error(ErrCode::FuncNotFound);
    spdlog::error(ErrInfo::InfoExecuting(ModName, Func));
    return Unexpect(ErrCode::FuncNotFound);
  }

  /// Execute function.
  if (auto Res = InterpreterEngine.invoke(StoreRef, FuncIter->second, Params,
                                          ParamTypes)) {
    return Res;
  } else {
    spdlog::error(ErrInfo::InfoExecuting(ModName, Func));
    return Unexpect(Res);
  }
}

void VM::cleanup() {
  Mod.reset();
  StoreRef.reset();
  Stat.clear();
  Stage = VMStage::Inited;
}

std::vector<std::pair<std::string, FunctionType>> VM::getFunctionList() const {
  std::vector<std::pair<std::string, FunctionType>> Res;
  for (auto &&Func : StoreRef.getFuncExports()) {
    const auto *FuncInst = *StoreRef.getFunction(Func.second);
    const auto &FuncType = FuncInst->getFuncType();
    Res.push_back({Func.first, FuncType});
  }
  return Res;
}

Runtime::ImportObject *VM::getImportModule(const HostRegistration Type) {
  if (ImpObjs.find(Type) != ImpObjs.cend()) {
    return ImpObjs[Type].get();
  }
  return nullptr;
}

} // namespace VM
} // namespace WasmEdge
