// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "vm/component_vm.h"

#include "common/errinfo.h"
#include "common/spdlog.h"
#include "host/wasi/component/hosts.h"
#include "plugin/plugin.h"

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

using namespace std::literals;

namespace WasmEdge {
namespace VM {

ComponentVM::ComponentVM(const Configure &Conf)
    : Conf(Conf), Stage(VMStage::Inited), LoaderEngine(Conf),
      ValidatorEngine(Conf), CoreExecutorEngine(Conf, &Stat),
      Store(std::make_unique<Runtime::Component::StoreManager>()),
      StoreRef(*Store), ExecutorEngine(CoreExecutorEngine) {
  unsafeInitVM();
}

ComponentVM::ComponentVM(const Configure &Conf,
                         Runtime::Component::StoreManager &S)
    : Conf(Conf), Stage(VMStage::Inited), LoaderEngine(Conf),
      ValidatorEngine(Conf), CoreExecutorEngine(Conf, &Stat), StoreRef(S),
      ExecutorEngine(CoreExecutorEngine) {
  unsafeInitVM();
}

ComponentVM::~ComponentVM() { unsafeTerminateInstances(); }

void ComponentVM::unsafeTerminateInstances() {
  if (auto *CompInst = ActiveCompInst.release()) {
    CompInst->terminate();
  }
  cleanupCompInstContainer(RegCompInsts);
  cleanupCompInstContainer(BuiltInCompInsts);
  cleanupCompInstContainer(PlugInCompInsts);
  cleanupCompInstContainer(BuiltInHostInsts);
}

Runtime::Instance::ComponentInstance *ComponentVM::unsafeGetImportComponent(
    const HostRegistration Type) const noexcept {
  if (auto Iter = BuiltInHostInsts.find(Type);
      Iter != BuiltInHostInsts.cend()) {
    return Iter->second.get();
  }
  return nullptr;
}

void ComponentVM::unsafeInitVM() {
  unsafeLoadBuiltInHosts();
  unsafeLoadPlugInHosts();
  unsafeRegisterBuiltInHosts();
  unsafeRegisterPlugInHosts();
}

void ComponentVM::unsafeLoadBuiltInHosts() {
  // The WASI hosts of one VM share one environment.
  cleanupCompInstContainer(BuiltInCompInsts);
  cleanupCompInstContainer(BuiltInHostInsts);
  if (Conf.hasHostRegistration(HostRegistration::Wasi)) {
    auto Hosts = std::make_unique<Host::WasiComponent::WasiHosts>();
    BuiltInCompInsts = Hosts->newInstances();
    BuiltInHostInsts.insert_or_assign(HostRegistration::Wasi, std::move(Hosts));
  }
}

void ComponentVM::unsafeLoadPlugInHosts() {
  // Load the component instances of the plug-ins.
  cleanupCompInstContainer(PlugInCompInsts);
  for (const auto &Plugin : Plugin::Plugin::plugins()) {
    if (Conf.isForbiddenPlugins(Plugin.name())) {
      continue;
    }
    for (const auto &Component : Plugin.components()) {
      PlugInCompInsts.push_back(Component.create());
    }
  }
}

void ComponentVM::unsafeRegisterBuiltInHosts() {
  for (auto &It : BuiltInCompInsts) {
    ExecutorEngine.registerComponent(StoreRef, *It);
  }
}

void ComponentVM::unsafeRegisterPlugInHosts() {
  for (auto &It : PlugInCompInsts) {
    ExecutorEngine.registerComponent(StoreRef, *It);
  }
}

Expect<void>
ComponentVM::unsafeRegisterComponent(std::string_view Name,
                                     const std::filesystem::path &Path) {
  EXPECTED_TRY(auto CompAST, LoaderEngine.parseComponent(Path));
  return unsafeRegisterComponent(Name, std::move(CompAST));
}

Expect<void> ComponentVM::unsafeRegisterComponent(std::string_view Name,
                                                  Span<const Byte> Code) {
  EXPECTED_TRY(auto CompAST, LoaderEngine.parseComponent(Code));
  return unsafeRegisterComponent(Name, std::move(CompAST));
}

Expect<void> ComponentVM::unsafeRegisterComponent(
    std::string_view Name, std::unique_ptr<AST::Component::Component> CompAST) {
  Comps.push_back(std::move(CompAST));
  return unsafeRegisterComponent(Name, *Comps.back());
}

Expect<void>
ComponentVM::unsafeRegisterComponent(std::string_view Name,
                                     const AST::Component::Component &CompAST) {
  EXPECTED_TRY(ValidatorEngine.validate(CompAST));
  EXPECTED_TRY(auto CompInst,
               ExecutorEngine.registerComponent(StoreRef, CompAST, Name));
  RegCompInsts.push_back(std::move(CompInst));
  return {};
}

Expect<void> ComponentVM::unsafeRegisterComponent(
    const Runtime::Instance::ComponentInstance &CompInst) {
  return ExecutorEngine.registerComponent(StoreRef, CompInst);
}

Expect<void> ComponentVM::unsafeUnregisterComponent(std::string_view Name) {
  EXPECTED_TRY(StoreRef.unregisterInstance(Name));
  auto Iter = std::find_if(
      RegCompInsts.begin(), RegCompInsts.end(),
      [Name](const std::unique_ptr<Runtime::Instance::ComponentInstance>
                 &CompInst) {
        return CompInst && CompInst->getComponentName() == Name;
      });
  // An instance its user owns was only named in the store.
  if (Iter != RegCompInsts.end()) {
    auto *CompInst = Iter->release();
    RegCompInsts.erase(Iter);
    CompInst->terminate();
  }
  return {};
}

Expect<void> ComponentVM::unsafeLoadWasm(const std::filesystem::path &Path) {
  EXPECTED_TRY(auto CompAST, LoaderEngine.parseComponent(Path));
  return unsafeLoadWasm(std::move(CompAST));
}

Expect<void> ComponentVM::unsafeLoadWasm(Span<const Byte> Code) {
  EXPECTED_TRY(auto CompAST, LoaderEngine.parseComponent(Code));
  return unsafeLoadWasm(std::move(CompAST));
}

Expect<void> ComponentVM::unsafeLoadWasm(
    std::unique_ptr<AST::Component::Component> CompAST) {
  Comps.push_back(std::move(CompAST));
  Comp = Comps.back().get();
  Stage = VMStage::Loaded;
  return {};
}

Expect<void> ComponentVM::unsafeValidate() {
  if (Stage < VMStage::Loaded || Comp == nullptr) {
    spdlog::error(ErrCode::Value::WrongVMWorkflow);
    return Unexpect(ErrCode::Value::WrongVMWorkflow);
  }
  EXPECTED_TRY(ValidatorEngine.validate(*Comp));
  Stage = VMStage::Validated;
  return {};
}

Expect<void> ComponentVM::unsafeInstantiate() {
  if (Stage < VMStage::Validated || Comp == nullptr) {
    spdlog::error(ErrCode::Value::WrongVMWorkflow);
    return Unexpect(ErrCode::Value::WrongVMWorkflow);
  }
  if (auto *CompInst = ActiveCompInst.release()) {
    CompInst->terminate();
  }
  Stage = VMStage::Validated;
  EXPECTED_TRY(ActiveCompInst,
               ExecutorEngine.instantiateComponent(StoreRef, *Comp));
  Stage = VMStage::Instantiated;
  return {};
}

Expect<std::vector<std::pair<ComponentValVariant, ComponentValType>>>
ComponentVM::unsafeExecute(std::string_view Func,
                           Span<const ComponentValVariant> Params,
                           Span<const ComponentValType> ParamTypes) {
  if (unlikely(!ActiveCompInst)) {
    spdlog::error(ErrCode::Value::WrongInstanceAddress);
    spdlog::error(ErrInfo::InfoExecuting("When invoking"sv, Func));
    return Unexpect(ErrCode::Value::WrongInstanceAddress);
  }
  return unsafeExecute(ActiveCompInst.get(), Func, Params, ParamTypes);
}

Expect<std::vector<std::pair<ComponentValVariant, ComponentValType>>>
ComponentVM::unsafeExecute(std::string_view CompName, std::string_view Func,
                           Span<const ComponentValVariant> Params,
                           Span<const ComponentValType> ParamTypes) {
  const auto *CompInst = StoreRef.findInstance(CompName);
  if (unlikely(CompInst == nullptr)) {
    spdlog::error(ErrCode::Value::WrongInstanceAddress);
    spdlog::error(ErrInfo::InfoExecuting(CompName, Func));
    return Unexpect(ErrCode::Value::WrongInstanceAddress);
  }
  return unsafeExecute(CompInst, Func, Params, ParamTypes);
}

Expect<std::vector<std::pair<ComponentValVariant, ComponentValType>>>
ComponentVM::unsafeExecute(const Runtime::Instance::ComponentInstance *CompInst,
                           std::string_view Func,
                           Span<const ComponentValVariant> Params,
                           Span<const ComponentValType> ParamTypes) {
  const auto *FuncInst = CompInst->findExportedFunction(Func);
  if (unlikely(FuncInst == nullptr)) {
    spdlog::error(ErrCode::Value::FuncNotFound);
    spdlog::error(ErrInfo::InfoExecuting(CompInst->getComponentName(), Func));
    return Unexpect(ErrCode::Value::FuncNotFound);
  }
  return ExecutorEngine.invoke(FuncInst, Params, ParamTypes)
      .map_error([CompInst, Func](auto E) {
        if (E != ErrCode::Value::Terminated) {
          spdlog::error(
              ErrInfo::InfoExecuting(CompInst->getComponentName(), Func));
        }
        return E;
      });
}

Async<Expect<std::vector<std::pair<ComponentValVariant, ComponentValType>>>>
ComponentVM::asyncExecute(std::string_view Func,
                          Span<const ComponentValVariant> Params,
                          Span<const ComponentValType> ParamTypes) {
  Expect<std::vector<std::pair<ComponentValVariant, ComponentValType>>> (
      ComponentVM::*FPtr)(std::string_view, Span<const ComponentValVariant>,
                          Span<const ComponentValType>) = &ComponentVM::execute;
  return {FPtr, *this, std::string(Func),
          std::vector(Params.begin(), Params.end()),
          std::vector(ParamTypes.begin(), ParamTypes.end())};
}

Async<Expect<std::vector<std::pair<ComponentValVariant, ComponentValType>>>>
ComponentVM::asyncExecute(std::string_view CompName, std::string_view Func,
                          Span<const ComponentValVariant> Params,
                          Span<const ComponentValType> ParamTypes) {
  Expect<std::vector<std::pair<ComponentValVariant, ComponentValType>>> (
      ComponentVM::*FPtr)(std::string_view, std::string_view,
                          Span<const ComponentValVariant>,
                          Span<const ComponentValType>) = &ComponentVM::execute;
  return {FPtr,
          *this,
          std::string(CompName),
          std::string(Func),
          std::vector(Params.begin(), Params.end()),
          std::vector(ParamTypes.begin(), ParamTypes.end())};
}

void ComponentVM::unsafeCleanup() {
  // The store entries naming the instances go first, then the importers.
  StoreRef.reset();
  if (auto *CompInst = ActiveCompInst.release()) {
    CompInst->terminate();
  }
  cleanupCompInstContainer(RegCompInsts);
  Comp = nullptr;
  Comps.clear();
  Stat.clear();
  unsafeLoadBuiltInHosts();
  unsafeLoadPlugInHosts();
  unsafeRegisterBuiltInHosts();
  unsafeRegisterPlugInHosts();
  LoaderEngine.reset();
  Stage = VMStage::Inited;
}

std::vector<std::pair<std::string, const AST::Component::FuncType &>>
ComponentVM::unsafeGetFunctionList() const {
  std::vector<std::pair<std::string, const AST::Component::FuncType &>> Map;
  if (ActiveCompInst) {
    ActiveCompInst->getFuncExports([&](const auto &FuncExports) {
      Map.reserve(FuncExports.size());
      for (const auto &[Name, Func] : FuncExports) {
        Map.emplace_back(Name, Func->getFuncType());
      }
    });
    // The functions of an exported instance list under `instance#func`.
    ActiveCompInst->getComponentInstanceExports([&](const auto &InstExports) {
      for (const auto &[InstName, Inst] : InstExports) {
        const std::string Prefix = InstName + "#";
        Inst->getFuncExports([&Map, &Prefix](const auto &FuncExports) {
          for (const auto &[Name, Func] : FuncExports) {
            Map.emplace_back(Prefix + Name, Func->getFuncType());
          }
        });
      }
    });
  }
  return Map;
}

const Runtime::Instance::ComponentInstance *
ComponentVM::unsafeGetActiveComponent() const {
  return ActiveCompInst.get();
}

} // namespace VM
} // namespace WasmEdge
