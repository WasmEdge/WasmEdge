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

#include "ast/component/component.h"
#include "ast/module.h"
#include "ast/type.h"
#include "common/errcode.h"
#include "common/types.h"
#include "runtime/instance/component/function.h"
#include "runtime/instance/component/hostfunc.h"
#include "runtime/instance/module.h"

#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

namespace WasmEdge {
namespace Runtime {
namespace Instance {

namespace {
void typeConvert(AST::Component::ValueType &VT, const ValType &Ty) noexcept {
  switch (Ty.getCode()) {
  case TypeCode::I8:
    VT.setCode(AST::Component::PrimValType::S8);
    break;
  case TypeCode::I16:
    VT.setCode(AST::Component::PrimValType::S16);
    break;
  case TypeCode::I32:
    VT.setCode(AST::Component::PrimValType::S32);
    break;
  case TypeCode::I64:
    VT.setCode(AST::Component::PrimValType::S64);
    break;
  case TypeCode::F32:
    VT.setCode(AST::Component::PrimValType::F32);
    break;
  case TypeCode::F64:
    VT.setCode(AST::Component::PrimValType::F64);
    break;
  default:
    break;
  }
}

void typeConvert(AST::Component::FuncType &FT,
                 const AST::FunctionType &Ty) noexcept {
  std::vector<AST::Component::LabelValType> PList;
  for (const auto &PT : Ty.getParamTypes()) {
    AST::Component::ValueType VT;
    typeConvert(VT, PT);
    PList.emplace_back("", VT);
  }
  FT.setParamList(std::move(PList));

  if (Ty.getReturnTypes().size() == 1) {
    AST::Component::ValueType VT;
    typeConvert(VT, Ty.getReturnTypes()[0]);
    FT.setResultType(VT);
  } else {
    std::vector<AST::Component::LabelValType> RList;
    for (const auto &RT : Ty.getReturnTypes()) {
      AST::Component::ValueType VT;
      typeConvert(VT, RT);
      PList.emplace_back("", VT);
    }
    FT.setResultList(std::move(RList));
  }
}
} // namespace

class ComponentInstance {
public:
  ComponentInstance(std::string_view Name) : CompName(Name) {}

  std::string_view getComponentName() const noexcept { return CompName; }

  void addModule(const AST::Module &M) noexcept { ModList.emplace_back(&M); }
  const AST::Module &getModule(uint32_t Index) const noexcept {
    return *ModList[Index];
  }

  void addComponent(const AST::Component::Component &C) noexcept {
    CompList.emplace_back(&C);
  }
  const AST::Component::Component &getComponent(uint32_t Index) const noexcept {
    return *CompList[Index];
  }

  void addModuleInstance(ModuleInstance *Inst) noexcept {
    ModInstList.push_back(std::move(Inst));
  }
  void addModuleInstance(std::unique_ptr<ModuleInstance> &&Inst) noexcept {
    ModInstList.push_back(Inst.get());
    OwnedModInstList.push_back(std::move(Inst));
  }
  const ModuleInstance *getModuleInstance(uint32_t Index) const noexcept {
    return ModInstList[Index];
  }

  void addComponentInstance(const ComponentInstance *Inst) noexcept {
    CompInstList.push_back(Inst);
  }
  void
  addComponentInstance(std::unique_ptr<ComponentInstance> &&Inst) noexcept {
    CompInstList.push_back(Inst.get());
    OwnedCompInstList.push_back(std::move(Inst));
  }
  const ComponentInstance *getComponentInstance(uint32_t Index) const noexcept {
    return CompInstList[Index];
  }

  void addHostFunc(std::string_view Name,
                   std::unique_ptr<Component::HostFunctionBase> &&Func) {
    addType(Func->getFuncType());
    auto FuncInst =
        std::make_unique<Component::FunctionInstance>(std::move(Func));
    unsafeAddHostFunc(Name, std::move(FuncInst));
  }
  void addHostFunc(std::string_view Name,
                   std::unique_ptr<Component::FunctionInstance> &&Func) {
    addType(Func->getFuncType());
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

  void addFunctionInstance(
      std::unique_ptr<Component::FunctionInstance> &&Inst) noexcept {
    addFunctionInstance(Inst.get());
    OwnedFuncInstList.emplace_back(std::move(Inst));
  }
  void addFunctionInstance(Component::FunctionInstance *Inst) noexcept {
    FuncInstList.push_back(Inst);
  }
  Component::FunctionInstance *
  getFunctionInstance(uint32_t Index) const noexcept {
    return FuncInstList[Index];
  }

  // values stored in component instance
  ValInterface getValue(uint32_t Index) const noexcept {
    return ValueList[Index];
  }
  void setValue(uint32_t Index, ValInterface V) noexcept {
    ValueList[Index] = V;
  }

  void addExport(std::string_view Name, const ModuleInstance *Inst) {
    ExportModuleMap.emplace(Name, Inst);
  }
  const ModuleInstance *
  findModuleExports(std::string_view Name) const noexcept {
    return ExportModuleMap.at(std::string(Name));
  }
  void addExport(std::string_view Name, Component::FunctionInstance *Inst) {
    ExportFuncMap.insert_or_assign(std::string(Name), Inst);
  }
  Component::FunctionInstance *
  findFuncExports(std::string_view Name) const noexcept {
    return ExportFuncMap.at(std::string(Name));
  }
  std::vector<std::pair<std::string, const AST::FunctionType &>>
  getFuncExports() {
    std::vector<std::pair<std::string, const AST::FunctionType &>> R;
    R.reserve(ExportFuncMap.size());
    for (auto &&[Name, Func] : ExportFuncMap) {
      const auto &FuncType = Func->getFuncType();
      R.emplace_back(Name, FuncType);
    }
    return R;
  }

  void addCoreType(const AST::Component::CoreDefType &Ty) noexcept {
    CoreTypes.emplace_back(&Ty);
  }
  const AST::Component::CoreDefType &getCoreType(uint32_t Idx) const noexcept {
    return *CoreTypes[Idx];
  }

  void addType(const AST::FunctionType &Ty) noexcept {
    AST::Component::FuncType FT;
    typeConvert(FT, Ty);
    OwnedTypes.push_back(std::make_unique<AST::Component::DefType>());
    OwnedTypes.back()->setFuncType(std::move(FT));
    Types.push_back(OwnedTypes.back().get());
  }
  void addType(const AST::Component::DefType &Ty) noexcept {
    Types.emplace_back(&Ty);
  }
  const AST::Component::DefType &getType(uint32_t Idx) const noexcept {
    return *Types[Idx];
  }

private:
  void unsafeAddHostFunc(
      std::string_view Name,
      std::unique_ptr<Component::FunctionInstance> &&Inst) noexcept {
    addFunctionInstance(Inst.get());
    ExportFuncMap.insert_or_assign(std::string(Name), FuncInstList.back());
    OwnedFuncInstList.push_back(std::move(Inst));
  }

private:
  std::string CompName;

  std::vector<const AST::Module *> ModList;
  std::vector<const AST::Component::Component *> CompList;

  std::vector<std::unique_ptr<ModuleInstance>> OwnedModInstList;
  std::vector<const ModuleInstance *> ModInstList;
  std::vector<std::unique_ptr<ComponentInstance>> OwnedCompInstList;
  std::vector<const ComponentInstance *> CompInstList;

  // value
  std::vector<ValInterface> ValueList;

  // core function
  //
  // The owned core functions are created by lowering process
  std::vector<std::unique_ptr<FunctionInstance>> OwnedCoreFuncInstList;
  std::vector<FunctionInstance *> CoreFuncInstList;

  // component function
  std::vector<std::unique_ptr<Component::FunctionInstance>> OwnedFuncInstList;
  std::vector<Component::FunctionInstance *> FuncInstList;
  std::map<std::string, Component::FunctionInstance *, std::less<>>
      ExportFuncMap;

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

  std::vector<const AST::Component::CoreDefType *> CoreTypes;
  std::vector<const AST::Component::DefType *> Types;
  std::vector<std::unique_ptr<AST::Component::DefType>> OwnedTypes;
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
