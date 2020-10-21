// SPDX-License-Identifier: Apache-2.0
#include "vm/vm.h"
#include "common/log.h"
#include "host/ssvm_process/processmodule.h"
#include "host/wasi/wasimodule.h"

namespace SSVM {
namespace VM {

VM::VM(Configure &InputConfig)
    : Config(InputConfig), Stage(VMStage::Inited), InterpreterEngine(&Stat),
      Store(std::make_unique<Runtime::StoreManager>()), StoreRef(*Store.get()) {
  initVM();
}

VM::VM(Configure &InputConfig, Runtime::StoreManager &S)
    : Config(InputConfig), Stage(VMStage::Inited), InterpreterEngine(&Stat),
      StoreRef(S) {
  initVM();
}

void VM::initVM() {
  /// Set cost table and create import modules from configure.
  CostTab.setCostTable(Configure::VMType::Wasm);
  Stat.setCostTable(CostTab.getCostTable(Configure::VMType::Wasm));
  if (Config.hasVMType(Configure::VMType::Wasi)) {
    /// 2nd priority of cost table: Wasi
    std::unique_ptr<Runtime::ImportObject> WasiMod =
        std::make_unique<Host::WasiModule>();
    InterpreterEngine.registerModule(StoreRef, *WasiMod.get());
    ImpObjs.insert({Configure::VMType::Wasi, std::move(WasiMod)});
    CostTab.setCostTable(Configure::VMType::Wasi);
    Stat.setCostTable(CostTab.getCostTable(Configure::VMType::Wasi));
  }
  if (Config.hasVMType(Configure::VMType::SSVM_Process)) {
    /// 1st priority of cost table: SSVM_Process
    std::unique_ptr<Runtime::ImportObject> ProcMod =
        std::make_unique<Host::SSVMProcessModule>();
    InterpreterEngine.registerModule(StoreRef, *ProcMod.get());
    ImpObjs.insert({Configure::VMType::SSVM_Process, std::move(ProcMod)});
    CostTab.setCostTable(Configure::VMType::SSVM_Process);
    Stat.setCostTable(CostTab.getCostTable(Configure::VMType::SSVM_Process));
  }
}

Expect<void> VM::registerModule(std::string_view Name, std::string_view Path) {
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

Expect<void> VM::registerModule(std::string_view Name,
                                const AST::Module &Module) {
  /// Validate module.
  if (auto Res = ValidatorEngine.validate(Module); !Res) {
    return Unexpect(Res);
  }
  return InterpreterEngine.registerModule(StoreRef, Module, Name);
}

Expect<std::vector<ValVariant>> VM::runWasmFile(std::string_view Path,
                                                std::string_view Func,
                                                Span<const ValVariant> Params) {
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

Expect<std::vector<ValVariant>> VM::runWasmFile(Span<const Byte> Code,
                                                std::string_view Func,
                                                Span<const ValVariant> Params) {
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

Expect<std::vector<ValVariant>> VM::runWasmFile(const AST::Module &Module,
                                                std::string_view Func,
                                                Span<const ValVariant> Params) {
  if (auto Res = ValidatorEngine.validate(Module); !Res) {
    return Unexpect(Res);
  }
  if (auto Res = InterpreterEngine.instantiateModule(StoreRef, Module); !Res) {
    return Unexpect(Res);
  }
  const auto FuncExp = StoreRef.getFuncExports();
  if (FuncExp.find(Func) == FuncExp.cend()) {
    LOG(ERROR) << ErrCode::FuncNotFound;
    LOG(ERROR) << ErrInfo::InfoExecuting("", Func);
    return Unexpect(ErrCode::FuncNotFound);
  }
  if (auto Res = InterpreterEngine.invoke(StoreRef, FuncExp.find(Func)->second,
                                          Params)) {
    return *Res;
  } else {
    return Unexpect(Res);
  }
}

Expect<void> VM::loadWasm(std::string_view Path) {
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

Expect<void> VM::validate() {
  if (Stage < VMStage::Loaded) {
    /// When module is not loaded, not validate.
    LOG(ERROR) << ErrCode::WrongVMWorkflow;
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
    LOG(ERROR) << ErrCode::WrongVMWorkflow;
    return Unexpect(ErrCode::WrongVMWorkflow);
  }
  if (auto Res =
          InterpreterEngine.instantiateModule(StoreRef, *Mod.get(), "")) {
    Stage = VMStage::Instantiated;
    return {};
  } else {
    return Unexpect(Res);
  }
}

Expect<std::vector<ValVariant>> VM::execute(std::string_view Func,
                                            Span<const ValVariant> Params) {
  /// Check exports for finding function address.
  const auto FuncExp = StoreRef.getFuncExports();
  if (FuncExp.find(Func) == FuncExp.cend()) {
    LOG(ERROR) << ErrCode::FuncNotFound;
    LOG(ERROR) << ErrInfo::InfoExecuting("", Func);
    return Unexpect(ErrCode::FuncNotFound);
  }

  /// Execute function.
  if (auto Res = InterpreterEngine.invoke(StoreRef, FuncExp.find(Func)->second,
                                          Params)) {
    return Res;
  } else {
    LOG(ERROR) << ErrInfo::InfoExecuting("", Func);
    return Unexpect(Res);
  }
}

Expect<std::vector<ValVariant>> VM::execute(std::string_view Mod,
                                            std::string_view Func,
                                            Span<const ValVariant> Params) {
  /// Get module instance.
  Runtime::Instance::ModuleInstance *ModInst;
  if (auto Res = StoreRef.findModule(Mod)) {
    ModInst = *Res;
  } else {
    LOG(ERROR) << Res.error();
    LOG(ERROR) << ErrInfo::InfoExecuting(Mod, Func);
    return Unexpect(Res);
  }

  /// Get exports and find function.
  const auto FuncExp = ModInst->getFuncExports();
  auto FuncIter = FuncExp.find(Func);
  if (FuncIter == FuncExp.cend()) {
    LOG(ERROR) << ErrCode::FuncNotFound;
    LOG(ERROR) << ErrInfo::InfoExecuting(Mod, Func);
    return Unexpect(ErrCode::FuncNotFound);
  }

  /// Execute function.
  if (auto Res = InterpreterEngine.invoke(StoreRef, FuncIter->second, Params)) {
    return Res;
  } else {
    LOG(ERROR) << ErrInfo::InfoExecuting(Mod, Func);
    return Unexpect(Res);
  }
}

void VM::cleanup() {
  Mod.reset();
  StoreRef.reset();
  Stat.clear();
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
