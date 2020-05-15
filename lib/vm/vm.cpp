// SPDX-License-Identifier: Apache-2.0
#include "vm/vm.h"
#include "host/wasi/wasimodule.h"

#ifdef ONNC_WASM
#include "host/onnc/onncmodule.h"
#endif

namespace SSVM {
namespace VM {

VM::VM(Configure &InputConfig)
    : Config(InputConfig), Stage(VMStage::Inited), InterpreterEngine(&Measure),
      Store(std::make_unique<Runtime::StoreManager>()), StoreRef(*Store.get()) {
  initVM();
}

VM::VM(Configure &InputConfig, Runtime::StoreManager &S)
    : Config(InputConfig), Stage(VMStage::Inited), InterpreterEngine(&Measure),
      StoreRef(S) {
  initVM();
}

void VM::initVM() {
  /// Set cost table and create import modules from configure.
  CostTab.setCostTable(Configure::VMType::Wasm);
  Measure.setCostTable(CostTab.getCostTable(Configure::VMType::Wasm));
  if (Config.hasVMType(Configure::VMType::Wasi)) {
    /// 2nd priority of cost table: Wasi
    std::unique_ptr<Runtime::ImportObject> WasiMod =
        std::make_unique<Host::WasiModule>();
    InterpreterEngine.registerModule(StoreRef, *WasiMod.get());
    ImpObjs.insert({Configure::VMType::Wasi, std::move(WasiMod)});
    CostTab.setCostTable(Configure::VMType::Wasi);
    Measure.setCostTable(CostTab.getCostTable(Configure::VMType::Wasi));
  }
#ifdef ONNC_WASM
  if (Config.hasVMType(Configure::VMType::ONNC)) {
    std::unique_ptr<Runtime::ImportObject> ONNCMod =
        std::make_unique<Host::ONNCModule>();
    InterpreterEngine.registerModule(StoreRef, *ONNCMod.get());
    ImpObjs.insert({Configure::VMType::ONNC, std::move(ONNCMod)});
  }
#endif
}

Expect<void> VM::registerModule(const std::string &Name,
                                const std::string &Path) {
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

Expect<void> VM::registerModule(const std::string &Name, const Bytes &Code) {
  if (Stage == VMStage::Instantiated) {
    /// When registering module, instantiated module in store will be reset.
    /// Therefore the instantiation should restart.
    Stage = VMStage::Validated;
  }
  /// Load module.
  std::unique_ptr<AST::Module> LoadedMod;
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

Expect<void> VM::registerModule(const std::string &Name,
                                const AST::Module &Module) {
  /// Validate module.
  if (auto Res = ValidatorEngine.validate(Module); !Res) {
    return Unexpect(Res);
  }
  return InterpreterEngine.registerModule(StoreRef, Module, Name);
}

Expect<std::vector<ValVariant>>
VM::runWasmFile(const std::string &Path, const std::string &Func,
                const std::vector<ValVariant> &Params) {
  if (Stage == VMStage::Instantiated) {
    /// When running another module, instantiated module in store will be reset.
    /// Therefore the instantiation should restart.
    Stage = VMStage::Validated;
  }
  /// Load module.
  if (auto Res = LoaderEngine.parseModule(Path)) {
    return runWasmFile(*(*Res).get(), Func, Params);
  } else {
    return Unexpect(Res);
  }
}

Expect<std::vector<ValVariant>>
VM::runWasmFile(const Bytes &Code, const std::string &Func,
                const std::vector<ValVariant> &Params) {
  if (Stage == VMStage::Instantiated) {
    /// When running another module, instantiated module in store will be reset.
    /// Therefore the instantiation should restart.
    Stage = VMStage::Validated;
  }
  /// Load module.
  if (auto Res = LoaderEngine.parseModule(Code)) {
    return runWasmFile(*(*Res).get(), Func, Params);
  } else {
    return Unexpect(Res);
  }
}

Expect<std::vector<ValVariant>>
VM::runWasmFile(const AST::Module &Module, const std::string &Func,
                const std::vector<ValVariant> &Params) {
  if (auto Res = ValidatorEngine.validate(Module); !Res) {
    return Unexpect(Res);
  }
  if (auto Res = InterpreterEngine.instantiateModule(StoreRef, Module); !Res) {
    return Unexpect(Res);
  }
  const auto FuncExp = StoreRef.getFuncExports();
  if (FuncExp.find(Func) == FuncExp.cend()) {
    return Unexpect(ErrCode::WrongInstanceAddress);
  }
  if (auto Res = InterpreterEngine.invoke(StoreRef, FuncExp.find(Func)->second,
                                          Params)) {
    return *Res;
  } else {
    return Unexpect(Res);
  }
}

Expect<void> VM::loadWasm(const std::string &Path) {
  /// If not load successfully, the previous status will be reserved.
  if (auto Res = LoaderEngine.parseModule(Path)) {
    Mod = std::move(*Res);
    Stage = VMStage::Loaded;
  } else {
    return Unexpect(Res);
  }
  return {};
}

Expect<void> VM::loadWasm(const Bytes &Code) {
  /// If not load successfully, the previous status will be reserved.
  if (auto Res = LoaderEngine.parseModule(Code)) {
    Mod = std::move(*Res);
    Stage = VMStage::Loaded;
  } else {
    return Unexpect(Res);
  }
  return {};
}

Expect<void> VM::validate() {
  if (Stage < VMStage::Loaded) {
    /// When module is not loaded, not validate.
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
    return Unexpect(ErrCode::ValidationFailed);
  }
  if (auto Res =
          InterpreterEngine.instantiateModule(StoreRef, *Mod.get(), "")) {
    Stage = VMStage::Instantiated;
    return {};
  } else {
    return Unexpect(Res);
  }
}

Expect<std::vector<ValVariant>>
VM::execute(const std::string &Func, const std::vector<ValVariant> &Params) {
  /// Check exports for finding function address.
  const auto FuncExp = StoreRef.getFuncExports();
  if (FuncExp.find(Func) == FuncExp.cend()) {
    return Unexpect(ErrCode::WrongInstanceAddress);
  }
  return InterpreterEngine.invoke(StoreRef, FuncExp.find(Func)->second, Params);
}

Expect<std::vector<ValVariant>>
VM::execute(const std::string &Mod, const std::string &Func,
            const std::vector<ValVariant> &Params) {
  /// Get module instance.
  Runtime::Instance::ModuleInstance *ModInst;
  if (auto Res = StoreRef.findModule(Mod)) {
    ModInst = *Res;
  } else {
    return Unexpect(Res);
  }

  /// Get exports and fund function
  const auto FuncExp = ModInst->getFuncExports();
  if (FuncExp.find(Func) == FuncExp.cend()) {
    return Unexpect(ErrCode::WrongInstanceAddress);
  }
  return InterpreterEngine.invoke(StoreRef, FuncExp.find(Func)->second, Params);
}

void VM::cleanup() {
  Mod.reset();
  StoreRef.reset();
  Measure.clear();
  Stage = VMStage::Inited;
}

std::vector<std::pair<std::string, Runtime::Instance::FType>>
VM::getFunctionList() const {
  std::vector<std::pair<std::string, Runtime::Instance::FType>> Res;
  for (auto &&Func : StoreRef.getFuncExports()) {
    const auto *FuncInst = *StoreRef.getFunction(Func.second);
    const auto &FuncType = FuncInst->getFuncType();
    Res.push_back({Func.first, FuncType});
  }
  return Res;
}

Runtime::ImportObject *VM::getImportModule(const Configure::VMType Type) {
  if (ImpObjs.find(Type) != ImpObjs.cend()) {
    return ImpObjs[Type].get();
  }
  return nullptr;
}

} // namespace VM
} // namespace SSVM
