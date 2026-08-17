// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "vm/component_vm.h"

#include "common/errcode.h"
#include "common/errinfo.h"
#include "common/spdlog.h"
#include "plugin/plugin.h"

#include <memory>
#include <utility>
#include <variant>

using namespace std::literals;

namespace WasmEdge {
namespace VM {
namespace Component {

VM::VM(const Configure &Conf)
    : Conf(Conf), Stage(VMStage::Inited),
      LoaderEngine(Conf, &Executor::Executor::Intrinsics),
      ValidatorEngine(Conf), CoreExecEngine(Conf, &Stat),
      Store(std::make_unique<Runtime::Component::StoreManager>()),
      StoreRef(*Store.get()) {
  unsafeInitVM();
}

VM::VM(const Configure &Conf, Runtime::Component::StoreManager &S)
    : Conf(Conf), Stage(VMStage::Inited),
      LoaderEngine(Conf, &Executor::Executor::Intrinsics),
      ValidatorEngine(Conf), CoreExecEngine(Conf, &Stat), StoreRef(S) {
  unsafeInitVM();
}

void VM::unsafeInitVM() {
  unsafeLoadPlugInHosts();
  unsafeRegisterPlugInHosts();
}

void VM::unsafeLoadPlugInHosts() {
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

void VM::unsafeRegisterPlugInHosts() {
  for (auto &It : PlugInCompInsts) {
    ExecEngine.registerComponent(StoreRef, *(It.get()));
  }
}

Expect<std::unique_ptr<AST::Component::Component>>
VM::unsafeTakeComponent(const std::filesystem::path &Path) {
  EXPECTED_TRY(auto Unit, LoaderEngine.parseWasmUnit(Path));
  if (!std::holds_alternative<std::unique_ptr<AST::Component::Component>>(
          Unit)) {
    spdlog::error(ErrCode::Value::MalformedVersion);
    spdlog::error("    the given file is a core module, not a component"sv);
    return Unexpect(ErrCode::Value::MalformedVersion);
  }
  return std::move(std::get<std::unique_ptr<AST::Component::Component>>(Unit));
}

Expect<void> VM::unsafeRegisterComponent(std::string_view Name,
                                         const std::filesystem::path &Path) {
  EXPECTED_TRY(auto CompAST, unsafeTakeComponent(Path));
  EXPECTED_TRY(ValidatorEngine.validate(*CompAST));
  EXPECTED_TRY(auto Inst,
               ExecEngine.registerComponent(StoreRef, *CompAST, Name));
  RegCompASTs.push_back(std::move(CompAST));
  RegCompInsts.push_back(std::move(Inst));
  return {};
}

Expect<void>
VM::unsafeRegisterComponent(std::string_view Name,
                            const AST::Component::Component &CompAST) {
  EXPECTED_TRY(auto Inst,
               ExecEngine.registerComponent(StoreRef, CompAST, Name));
  RegCompInsts.push_back(std::move(Inst));
  return {};
}

Expect<void> VM::unsafeLoadWasm(const std::filesystem::path &Path) {
  // If loading does not succeed, the previous status is preserved.
  EXPECTED_TRY(auto CompAST, unsafeTakeComponent(Path));
  Comp = std::move(CompAST);
  Stage = VMStage::Loaded;
  return {};
}

Expect<void> VM::unsafeLoadWasm(Span<const Byte> Code) {
  EXPECTED_TRY(auto Unit, LoaderEngine.parseWasmUnit(Code));
  if (!std::holds_alternative<std::unique_ptr<AST::Component::Component>>(
          Unit)) {
    spdlog::error(ErrCode::Value::MalformedVersion);
    spdlog::error("    the given bytecode is a core module, not a component"sv);
    return Unexpect(ErrCode::Value::MalformedVersion);
  }
  Comp = std::move(std::get<std::unique_ptr<AST::Component::Component>>(Unit));
  Stage = VMStage::Loaded;
  return {};
}

Expect<void> VM::unsafeValidate() {
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

Expect<void> VM::unsafeInstantiate() {
  if (Stage < VMStage::Validated) {
    // Do not instantiate when the component is not validated.
    spdlog::error(ErrCode::Value::WrongVMWorkflow);
    return Unexpect(ErrCode::Value::WrongVMWorkflow);
  }
  if (!Comp) {
    spdlog::error(ErrCode::Value::WrongVMWorkflow);
    return Unexpect(ErrCode::Value::WrongVMWorkflow);
  }
  EXPECTED_TRY(ActiveCompInst,
               ExecEngine.instantiateComponent(StoreRef, *Comp));
  Stage = VMStage::Instantiated;
  return {};
}

Expect<std::vector<std::pair<ComponentValVariant, ComponentValType>>>
VM::unsafeExecute(std::string_view Func, Span<const ComponentValVariant> Params,
                  Span<const ComponentValType> ParamTypes) {
  if (unlikely(!ActiveCompInst)) {
    spdlog::error(ErrCode::Value::WrongInstanceAddress);
    spdlog::error(ErrInfo::InfoExecuting("When invoking"sv, Func));
    return Unexpect(ErrCode::Value::WrongInstanceAddress);
  }
  return unsafeExecute(ActiveCompInst.get(), Func, Params, ParamTypes);
}

Expect<std::vector<std::pair<ComponentValVariant, ComponentValType>>>
VM::unsafeExecute(std::string_view CompName, std::string_view Func,
                  Span<const ComponentValVariant> Params,
                  Span<const ComponentValType> ParamTypes) {
  // Find the component instance by name.
  const auto *FindCompInst = StoreRef.find(CompName);
  if (unlikely(!FindCompInst)) {
    spdlog::error(ErrCode::Value::WrongInstanceAddress);
    spdlog::error(ErrInfo::InfoExecuting(CompName, Func));
    return Unexpect(ErrCode::Value::WrongInstanceAddress);
  }
  return unsafeExecute(FindCompInst, Func, Params, ParamTypes);
}

Expect<std::vector<std::pair<ComponentValVariant, ComponentValType>>>
VM::unsafeExecute(const Runtime::Instance::ComponentInstance *CompInst,
                  std::string_view Func, Span<const ComponentValVariant> Params,
                  Span<const ComponentValType> ParamTypes) {
  // Find exported function by name.
  Runtime::Instance::Component::FunctionInstance *FuncInst =
      CompInst->findFunction(Func);

  // A caller that passes values without types, such as the spec-test
  // harness, takes the parameter types from the own type of the function.
  std::vector<ComponentValType> DerivedTypes;
  if (FuncInst != nullptr && ParamTypes.empty() && !Params.empty()) {
    for (const auto &P : FuncInst->getFuncType().getParamList()) {
      DerivedTypes.push_back(P.getValType());
    }
    ParamTypes = DerivedTypes;
  }

  // Execute function.
  return ExecEngine.invoke(FuncInst, Params, ParamTypes)
      .map_error([&CompInst, &Func](auto E) {
        if (E != ErrCode::Value::Terminated) {
          spdlog::error(
              ErrInfo::InfoExecuting(CompInst->getComponentName(), Func));
        }
        return E;
      });
}

Async<Expect<std::vector<std::pair<ComponentValVariant, ComponentValType>>>>
VM::asyncExecute(std::string_view Func, Span<const ComponentValVariant> Params,
                 Span<const ComponentValType> ParamTypes) {
  Expect<std::vector<std::pair<ComponentValVariant, ComponentValType>>> (
      VM::*FPtr)(std::string_view, Span<const ComponentValVariant>,
                 Span<const ComponentValType>) = &VM::execute;
  return {FPtr, *this, std::string(Func),
          std::vector(Params.begin(), Params.end()),
          std::vector(ParamTypes.begin(), ParamTypes.end())};
}

Async<Expect<std::vector<std::pair<ComponentValVariant, ComponentValType>>>>
VM::asyncExecute(std::string_view CompName, std::string_view Func,
                 Span<const ComponentValVariant> Params,
                 Span<const ComponentValType> ParamTypes) {
  Expect<std::vector<std::pair<ComponentValVariant, ComponentValType>>> (
      VM::*FPtr)(std::string_view, std::string_view,
                 Span<const ComponentValVariant>,
                 Span<const ComponentValType>) = &VM::execute;
  return {FPtr,
          *this,
          std::string(CompName),
          std::string(Func),
          std::vector(Params.begin(), Params.end()),
          std::vector(ParamTypes.begin(), ParamTypes.end())};
}

void VM::unsafeCleanup() {
  Comp.reset();
  ActiveCompInst.reset();
  StoreRef.reset();
  RegCompInsts.clear();
  RegCompASTs.clear();
  PlugInCompInsts.clear();
  Stat.clear();
  unsafeInitVM();
  Stage = VMStage::Inited;
}

std::vector<std::pair<std::string, const AST::Component::FuncType &>>
VM::unsafeGetFunctionList() const {
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

} // namespace Component
} // namespace VM
} // namespace WasmEdge
