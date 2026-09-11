// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "vm/component_vm.h"

#include "common/component_valtype.h"
#include "common/component_variant.h"
#include "common/errcode.h"
#include "common/errinfo.h"
#include "common/spdlog.h"
#include "host/wasi/component/hosts.h"
#include "plugin/plugin.h"

#include <memory>
#include <utility>

using namespace std::literals;

namespace WasmEdge {
namespace VM {

ComponentVM::ComponentVM(const Configure &Conf)
    : Conf(Conf), Stage(VMStage::Inited),
      LoaderEngine(Conf, &Executor::Executor::Intrinsics),
      ValidatorEngine(Conf), CoreExecutorEngine(Conf, &Stat),
      Store(std::make_unique<Runtime::Component::StoreManager>()),
      StoreRef(*Store.get()) {
  unsafeInitVM();
}

ComponentVM::ComponentVM(const Configure &Conf,
                         Runtime::Component::StoreManager &S)
    : Conf(Conf), Stage(VMStage::Inited),
      LoaderEngine(Conf, &Executor::Executor::Intrinsics),
      ValidatorEngine(Conf), CoreExecutorEngine(Conf, &Stat), StoreRef(S) {
  unsafeInitVM();
}

ComponentVM::~ComponentVM() = default;

Host::WasiComponent::Env *ComponentVM::getWasiEnv() noexcept {
  return Wasi ? &Wasi->getEnv() : nullptr;
}

void ComponentVM::unsafeInitVM() {
  unsafeLoadBuiltInHosts();
  unsafeLoadPlugInHosts();
  unsafeRegisterBuiltInHosts();
  unsafeRegisterPlugInHosts();
}

void ComponentVM::unsafeLoadBuiltInHosts() {
  BuiltInCompInsts.clear();
  Wasi.reset();
  if (Conf.hasHostRegistration(HostRegistration::Wasi)) {
    Wasi = std::make_unique<Host::WasiComponent::WasiHosts>();
    BuiltInCompInsts = Wasi->makeInstances();
  }
}

void ComponentVM::unsafeLoadPlugInHosts() {
  PlugInCompInsts.clear();
  for (const auto &Plugin : Plugin::Plugin::plugins()) {
    if (Conf.isForbiddenPlugins(Plugin.name())) {
      continue;
    }
    for (const auto &CompDesc : Plugin.components()) {
      PlugInCompInsts.push_back(CompDesc.create());
    }
  }
}

void ComponentVM::unsafeRegisterBuiltInHosts() {
  for (auto &Inst : BuiltInCompInsts) {
    ExecutorEngine.registerComponent(StoreRef, *Inst);
  }
}

void ComponentVM::unsafeRegisterPlugInHosts() {
  for (auto &Inst : PlugInCompInsts) {
    ExecutorEngine.registerComponent(StoreRef, *Inst);
  }
}

Expect<void>
ComponentVM::unsafeRegisterComponent(std::string_view Name,
                                     const std::filesystem::path &Path) {
  EXPECTED_TRY(auto CompAST, LoaderEngine.parseComponent(Path));
  return unsafeRegisterComponent(Name, *CompAST);
}

Expect<void> ComponentVM::unsafeRegisterComponent(std::string_view Name,
                                                  Span<const Byte> Code) {
  EXPECTED_TRY(auto CompAST, LoaderEngine.parseComponent(Code));
  return unsafeRegisterComponent(Name, *CompAST);
}

Expect<void>
ComponentVM::unsafeRegisterComponent(std::string_view Name,
                                     const AST::Component::Component &CompAST) {
  EXPECTED_TRY(ValidatorEngine.validate(CompAST));
  EXPECTED_TRY(auto Inst,
               ExecutorEngine.registerComponent(StoreRef, CompAST, Name));
  RegCompInsts.push_back(std::move(Inst));
  return {};
}

Expect<void> ComponentVM::unsafeLoadWasm(const std::filesystem::path &Path) {
  // If loading does not succeed, the previous status is preserved.
  EXPECTED_TRY(auto CompAST, LoaderEngine.parseComponent(Path));
  Comp = std::move(CompAST);
  Stage = VMStage::Loaded;
  return {};
}

Expect<void> ComponentVM::unsafeLoadWasm(Span<const Byte> Code) {
  EXPECTED_TRY(auto CompAST, LoaderEngine.parseComponent(Code));
  Comp = std::move(CompAST);
  Stage = VMStage::Loaded;
  return {};
}

Expect<void> ComponentVM::unsafeValidate() {
  if (Stage < VMStage::Loaded) {
    // Do not validate when the component is not loaded.
    spdlog::error(ErrCode::Value::WrongVMWorkflow);
    return Unexpect(ErrCode::Value::WrongVMWorkflow);
  }
  if (!Comp) {
    spdlog::error(ErrCode::Value::WrongVMWorkflow);
    return Unexpect(ErrCode::Value::WrongVMWorkflow);
  }
  EXPECTED_TRY(ValidatorEngine.validate(*Comp.get()));
  Stage = VMStage::Validated;
  return {};
}

Expect<void> ComponentVM::unsafeInstantiate() {
  if (Stage < VMStage::Validated) {
    // Do not instantiate when the component is not validated.
    spdlog::error(ErrCode::Value::WrongVMWorkflow);
    return Unexpect(ErrCode::Value::WrongVMWorkflow);
  }
  if (!Comp) {
    spdlog::error(ErrCode::Value::WrongVMWorkflow);
    return Unexpect(ErrCode::Value::WrongVMWorkflow);
  }
  if (Conf.getRuntimeConfigure().getRunMode() != RunMode::Interpreter) {
    spdlog::warn("component model runs on the interpreter; the configured "
                 "run mode is ignored."sv);
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
  // Find the component instance by name.
  const auto *FindCompInst = StoreRef.findInstance(CompName);
  if (unlikely(!FindCompInst)) {
    spdlog::error(ErrCode::Value::WrongInstanceAddress);
    spdlog::error(ErrInfo::InfoExecuting(CompName, Func));
    return Unexpect(ErrCode::Value::WrongInstanceAddress);
  }
  return unsafeExecute(FindCompInst, Func, Params, ParamTypes);
}

Expect<std::vector<std::pair<ComponentValVariant, ComponentValType>>>
ComponentVM::unsafeExecute(const Runtime::Instance::ComponentInstance *CompInst,
                           std::string_view Func,
                           Span<const ComponentValVariant> Params,
                           Span<const ComponentValType> ParamTypes) {
  // Find exported function by name, or by `interface#func` into an instance.
  Runtime::Instance::ComponentFunctionInstance *FuncInst =
      CompInst->findExportedFunction(Func);
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
  Comp.reset();
  ActiveCompInst.reset();
  StoreRef.reset();
  RegCompInsts.clear();
  Stat.clear();
  unsafeInitVM();
  LoaderEngine.reset();
  Stage = VMStage::Inited;
}

std::vector<std::pair<std::string, const AST::Component::FuncType &>>
ComponentVM::unsafeGetFunctionList() const {
  std::vector<std::pair<std::string, const AST::Component::FuncType &>> Map;
  if (ActiveCompInst) {
    ActiveCompInst->getFuncExports([&](const auto &FuncExports) {
      Map.reserve(FuncExports.size());
      for (auto &&Func : FuncExports) {
        const auto &FuncType = (Func.second)->getFuncType();
        Map.emplace_back(Func.first, FuncType);
      }
    });
    // Functions of an exported instance are reachable as `interface#func`,
    // which is the shape a WIT world compiles to.
    ActiveCompInst->getComponentInstanceExports([&](const auto &InstExports) {
      for (auto &&Inst : InstExports) {
        (Inst.second)->getFuncExports([&](const auto &FuncExports) {
          for (auto &&Func : FuncExports) {
            Map.emplace_back(Inst.first + "#" + Func.first,
                             (Func.second)->getFuncType());
          }
        });
      }
    });
  }
  return Map;
}

} // namespace VM
} // namespace WasmEdge
