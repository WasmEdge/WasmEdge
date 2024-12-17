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
#include "common/types.h"
#include "runtime/component/hostfunc.h"
#include "runtime/instance/component/function.h"
#include "runtime/instance/module.h"
#include "runtime/instance/resource.h"

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

class ComponentInstance {
public:
  ComponentInstance(std::string_view Name) : CompName(Name) {}

  std::string_view getComponentName() const noexcept;

  void addModule(const AST::Module &M) noexcept;
  const AST::Module &getModule(uint32_t Index) const noexcept;

  void addComponent(const AST::Component::Component &C) noexcept;
  const AST::Component::Component &getComponent(uint32_t Index) const noexcept;

  void addModuleInstance(ModuleInstance *Inst) noexcept;
  void addModuleInstance(std::unique_ptr<ModuleInstance> Inst) noexcept;
  Expect<const ModuleInstance *>
  getModuleInstance(uint32_t Index) const noexcept;

  void addComponentInstance(const ComponentInstance *Inst) noexcept;
  void addComponentInstance(std::unique_ptr<ComponentInstance> Inst) noexcept;
  const ComponentInstance *getComponentInstance(uint32_t Index) const noexcept;

  void addHostFunc(
      std::string_view Name,
      std::unique_ptr<WasmEdge::Runtime::Component::HostFunctionBase> &&Func);
  void addHostFunc(std::string_view Name,
                   std::unique_ptr<Component::FunctionInstance> &&Func);

  void
  addCoreFunctionInstance(std::unique_ptr<FunctionInstance> &&Inst) noexcept;
  void addCoreFunctionInstance(FunctionInstance *Inst) noexcept;
  Expect<FunctionInstance *>
  getCoreFunctionInstance(uint32_t Index) const noexcept;

  void addFunctionInstance(
      std::unique_ptr<Component::FunctionInstance> Inst) noexcept;
  void addFunctionInstance(Component::FunctionInstance *Inst) noexcept;
  Component::FunctionInstance *
  getFunctionInstance(uint32_t Index) const noexcept;

  // values stored in component instance
  ValInterface getValue(uint32_t Index) const noexcept;
  void setValue(uint32_t Index, ValInterface V) noexcept;

  void addExport(std::string_view Name, const ModuleInstance *Inst) noexcept;
  const ModuleInstance *findModuleExports(std::string_view Name) const noexcept;
  void addExport(std::string_view Name,
                 Component::FunctionInstance *Inst) noexcept;
  Component::FunctionInstance *
  findFuncExports(std::string_view Name) const noexcept;
  std::vector<std::pair<std::string, const AST::FunctionType &>>
  getFuncExports() const noexcept;

  void addCoreTableInstance(TableInstance *Inst) noexcept;
  Expect<TableInstance *> getCoreTableInstance(uint32_t Index) const noexcept;

  void addCoreMemoryInstance(MemoryInstance *Inst) noexcept;
  Expect<MemoryInstance *> getCoreMemoryInstance(uint32_t Index) const noexcept;

  void addCoreGlobalInstance(GlobalInstance *Inst) noexcept;
  Expect<GlobalInstance *> getCoreGlobalInstance(uint32_t Index) const noexcept;

  void addCoreType(const CoreDefType &Ty) noexcept;
  Expect<const CoreDefType> getCoreType(uint32_t Idx) const noexcept;

  void addHostType(std::string_view Name, PrimValType &&Type) noexcept;
  void addHostType(std::string_view Name, RecordTy &&Type) noexcept;
  void addHostType(std::string_view Name, VariantTy &&Type) noexcept;
  void addHostType(std::string_view Name, ListTy &&Type) noexcept;
  void addHostType(std::string_view Name, TupleTy &&Type) noexcept;
  void addHostType(std::string_view Name, Flags &&Type) noexcept;
  void addHostType(std::string_view Name, EnumTy &&Type) noexcept;
  void addHostType(std::string_view Name, OptionTy &&Type) noexcept;
  void addHostType(std::string_view Name, ResultTy &&Type) noexcept;
  void addHostType(std::string_view Name, Own &&Type) noexcept;
  void addHostType(std::string_view Name, Borrow &&Type) noexcept;
  void addHostType(std::string_view Name, FuncType &&Type) noexcept;
  void addHostType(std::string_view Name, ResourceType &&Type) noexcept;
  void addHostType(std::string_view Name, ComponentType &&Type) noexcept;
  void addHostType(std::string_view Name, InstanceType &&Type) noexcept;

  const AST::Component::DefType getType(std::string_view Name) const noexcept;
  void addCoreFuncType(const AST::FunctionType &Ty) noexcept;
  void addType(DefType Ty) noexcept;
  Expect<const DefType> getType(uint32_t Idx) const noexcept;
  TypeIndex typeToIndex(DefType Ty) noexcept;
  TypeIndex getLastTypeIndex() noexcept;

  std::shared_ptr<ResourceHandle> removeResource(uint32_t ResourceTypeIndex,
                                                 uint32_t HandleIndex) noexcept;

private:
  void unsafeAddHostFunc(
      std::string_view Name,
      std::unique_ptr<Component::FunctionInstance> Inst) noexcept {
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

  std::map<std::string, AST::Component::DefType, std::less<>> ExportTypesMap;
  std::vector<AST::Component::DefType> Types;

  std::map<uint32_t, std::map<uint32_t, std::shared_ptr<ResourceHandle>>>
      Resources;

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
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
