// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/runtime/instance/component.h - Component Instance definition //
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the component instance definition.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/component/type.h"
#include "ast/module.h"
#include "ast/type.h"
#include "common/errcode.h"
#include "runtime/instance/module.h"

#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <shared_mutex>
#include <string>
#include <type_traits>
#include <vector>

namespace WasmEdge {

namespace Runtime {

namespace Instance {

using namespace AST::Component;

namespace {
void typeConvert(ValueType &VT, const ValType &ValTy) noexcept {
  switch (ValTy.getCode()) {
  case TypeCode::I32:
    VT.emplace<PrimValType>(PrimValType::S32);
    break;

  default:
    break;
  }
}

void typeConvert(FuncType &FT, const AST::FunctionType &Ty) noexcept {
  auto &PL = FT.getParamList();
  for (auto const &PT : Ty.getParamTypes()) {
    LabelValType L{};
    typeConvert(L.getValType(), PT);
    PL.emplace_back(L);
  }
  auto &RL = FT.getResultList();
  if (Ty.getReturnTypes().size() == 1) {
    typeConvert(RL.emplace<ValueType>(), Ty.getReturnTypes()[0]);
  } else {
    auto &LL = RL.emplace<std::vector<LabelValType>>();
    for (auto const &RT : Ty.getReturnTypes()) {
      LabelValType L{};
      typeConvert(L.getValType(), RT);
      LL.emplace_back(L);
    }
  }
}
} // namespace

class ComponentInstance {
public:
  ComponentInstance(std::string_view Name) : CompName(Name) {}

  std::string_view getComponentName() const noexcept { return CompName; }

  void addModule(const AST::Module &M) noexcept { ModList.emplace_back(M); }
  const AST::Module &getModule(uint32_t Index) const noexcept {
    return ModList[Index];
  }

  void addComponent(const AST::Component::Component &C) noexcept {
    CompList.emplace_back(C);
  }
  const AST::Component::Component &getComponent(uint32_t Index) const noexcept {
    return CompList[Index];
  }

  void addModuleInstance(ModuleInstance *Inst) noexcept {
    ModInstList.push_back(std::move(Inst));
  }
  void addModuleInstance(std::unique_ptr<ModuleInstance> Inst) noexcept {
    ModInstList.push_back(Inst.get());
    OwnedModInstList.push_back(std::move(Inst));
  }
  const ModuleInstance *getModuleInstance(uint32_t Index) const noexcept {
    return ModInstList[Index];
  }

  void addComponentInstance(const ComponentInstance *Inst) noexcept {
    CompInstList.push_back(Inst);
  }
  void addComponentInstance(std::unique_ptr<ComponentInstance> Inst) noexcept {
    CompInstList.push_back(Inst.get());
    OwnedCompInstList.push_back(std::move(Inst));
  }
  const ComponentInstance *getComponentInstance(uint32_t Index) const noexcept {
    return CompInstList[Index];
  }

  void addHostFunc(std::string_view Name,
                   std::unique_ptr<HostFunctionBase> &&Func) {
    addType(Func->getFuncType());
    auto FuncInst = std::make_unique<FunctionInstance>(
        this, static_cast<uint32_t>(Types.size()) - 1, std::move(Func));
    unsafeAddHostFunc(Name, std::move(FuncInst));
  }
  void addHostFunc(std::string_view Name,
                   std::unique_ptr<FunctionInstance> &&Func) {
    assuming(Func->isHostFunction());
    addType(Func->getFuncType());
    Func->linkDefinedType(this, static_cast<uint32_t>(Types.size()) - 1);
    unsafeAddHostFunc(Name, std::move(Func));
  }

  void
  addCoreFunctionInstance(std::unique_ptr<FunctionInstance> &&Inst) noexcept {
    addCoreFunctionInstance(Inst.get());
    OwnedCoreFuncInstList.emplace_back(std::move(Inst));
  }
  void addCoreFunctionInstance(FunctionInstance *Inst) noexcept {
    CoreFuncInstList.push_back(Inst);
  }
  FunctionInstance *getCoreFunctionInstance(uint32_t Index) const noexcept {
    return CoreFuncInstList[Index];
  }

  void addFunctionInstance(std::unique_ptr<FunctionInstance> Inst) noexcept {
    addFunctionInstance(Inst.get());
    OwnedFuncInstList.emplace_back(std::move(Inst));
  }
  void addFunctionInstance(FunctionInstance *Inst) noexcept {
    FuncInstList.push_back(Inst);
  }
  FunctionInstance *getFunctionInstance(uint32_t Index) const noexcept {
    return FuncInstList[Index];
  }

  void addExport(std::string_view Name, const ModuleInstance *Inst) {
    ExportModuleMap.emplace(Name, Inst);
  }
  const ModuleInstance *
  findModuleExports(std::string_view Name) const noexcept {
    return ExportModuleMap.at(std::string(Name));
  }
  void addExport(std::string_view Name, FunctionInstance *Inst) {
    ExportFuncMap.emplace(Name, Inst);
  }
  FunctionInstance *findFuncExports(std::string_view Name) const noexcept {
    return ExportFuncMap.at(std::string(Name));
  }
  std::vector<std::pair<std::string, const AST::FunctionType &>>
  getFuncExports() {
    std::vector<std::pair<std::string, const AST::FunctionType &>> R;
    for (auto &&[Name, Func] : ExportFuncMap) {
      const auto &FuncType = Func->getFuncType();
      R.emplace_back(Name, FuncType);
    }
    return R;
  }

  void addCoreTableInstance(TableInstance *Inst) noexcept {
    CoreTabInstList.push_back(Inst);
  }
  TableInstance *getCoreTableInstance(uint32_t Index) const noexcept {
    return CoreTabInstList[Index];
  }
  void addCoreMemoryInstance(MemoryInstance *Inst) noexcept {
    CoreMemInstList.push_back(Inst);
  }
  MemoryInstance *getCoreMemoryInstance(uint32_t Index) const noexcept {
    return CoreMemInstList[Index];
  }
  void addCoreGlobalInstance(GlobalInstance *Inst) noexcept {
    CoreGlobInstList.push_back(Inst);
  }
  GlobalInstance *getCoreGlobalInstance(uint32_t Index) const noexcept {
    return CoreGlobInstList[Index];
  }

  void addCoreType(const CoreDefType &Ty) noexcept {
    CoreTypes.emplace_back(Ty);
  }
  const CoreDefType &getCoreType(uint32_t Idx) const noexcept {
    return CoreTypes[Idx];
  }

  void addType(const AST::FunctionType &Ty) noexcept {
    FuncType FT{};
    typeConvert(FT, Ty);
    addType(FT);
  }
  void addType(const DefType &Ty) noexcept { Types.emplace_back(Ty); }
  const DefType &getType(uint32_t Idx) const noexcept { return Types[Idx]; }

private:
  void unsafeAddHostFunc(std::string_view Name,
                         std::unique_ptr<FunctionInstance> Inst) noexcept {
    addFunctionInstance(Inst.get());
    ExportFuncMap.insert_or_assign(std::string(Name), FuncInstList.back());
    OwnedFuncInstList.push_back(std::move(Inst));
  }

private:
  std::string CompName;

  std::vector<AST::Module> ModList;
  std::vector<AST::Component::Component> CompList;

  std::vector<std::unique_ptr<ModuleInstance>> OwnedModInstList;
  std::vector<const ModuleInstance *> ModInstList;
  std::vector<std::unique_ptr<ComponentInstance>> OwnedCompInstList;
  std::vector<const ComponentInstance *> CompInstList;

  // core function
  //
  // The owned core functions are created by lowering process
  std::vector<std::unique_ptr<FunctionInstance>> OwnedCoreFuncInstList;
  std::vector<FunctionInstance *> CoreFuncInstList;

  // component function
  std::vector<std::unique_ptr<FunctionInstance>> OwnedFuncInstList;
  std::vector<FunctionInstance *> FuncInstList;
  std::map<std::string, FunctionInstance *, std::less<>> ExportFuncMap;

  std::map<std::string, const ModuleInstance *, std::less<>> ExportModuleMap;

  // core memory, this is prepared for canonical ABI
  //
  // when a function is lowering or lifting, it can have options
  // 1. memory of a module instance
  // 2. realloc of the same module instance
  // these data will help a component function encode its high-level data to
  // core module data, for example, `string` converted to `(i32, i32)`. Which is
  // a pair of pointer and length, where pointer stored in the memory
  // instance from option.
  std::vector<TableInstance *> CoreTabInstList;
  std::vector<MemoryInstance *> CoreMemInstList;
  std::vector<GlobalInstance *> CoreGlobInstList;

  std::vector<AST::Component::CoreDefType> CoreTypes;
  std::vector<AST::Component::DefType> Types;
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
