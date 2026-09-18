// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "vm/component_vm.h"

#include "common/errinfo.h"
#include "common/spdlog.h"
#include "plugin/plugin.h"

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

ComponentVM::~ComponentVM() { unsafeCleanup(); }

void ComponentVM::unsafeInitVM() {
  unsafeLoadPlugInHosts();
  unsafeRegisterPlugInHosts();
}

void ComponentVM::unsafeLoadPlugInHosts() {
  // Load the component instances of the plug-ins.
  for (const auto &Plugin : Plugin::Plugin::plugins()) {
    if (Conf.isForbiddenPlugins(Plugin.name())) {
      continue;
    }
    for (const auto &Component : Plugin.components()) {
      PlugInCompInsts.push_back(Component.create());
    }
  }
}

void ComponentVM::unsafeRegisterPlugInHosts() {
  for (auto &It : PlugInCompInsts) {
    static_cast<void>(ExecutorEngine.registerComponent(StoreRef, *It));
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

Expect<void> ComponentVM::unsafeLoadWasm(const std::filesystem::path &Path) {
  EXPECTED_TRY(auto CompAST, LoaderEngine.parseComponent(Path));
  return unsafeLoadWasm(std::move(CompAST));
}

Expect<void> ComponentVM::unsafeLoadWasm(Span<const Byte> Code) {
  EXPECTED_TRY(auto CompAST, LoaderEngine.parseComponent(Code));
  return unsafeLoadWasm(std::move(CompAST));
}

Expect<void>
ComponentVM::unsafeLoadWasm(std::unique_ptr<AST::Component::Component> C) {
  // The previous active component stays alive for its instance.
  Comps.push_back(std::move(C));
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
  ActiveCompInst.reset();
  RegCompInsts.clear();
  if (Store) {
    StoreRef.reset();
  }
  Comp = nullptr;
  Comps.clear();
  Stat.clear();
  Stage = VMStage::Inited;
  unsafeRegisterPlugInHosts();
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

} // namespace VM
} // namespace WasmEdge
