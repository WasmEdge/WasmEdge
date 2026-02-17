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

class ComponentInstance;

class ComponentImportManager {
  // The import manager is used for supplying the imports to local instantiate
  // child components and core modules.
public:
  // Export component func with name into this import manager.
  void exportFunction(const AST::Component::ComponentName &Name,
                      Component::FunctionInstance *Inst) noexcept {
    NamedFunc.emplace(Name.getFullName(), Inst);
  }
  void exportFunction(std::string_view Name,
                      Component::FunctionInstance *Inst) noexcept {
    NamedFunc.emplace(Name, Inst);
  }

  // Export component instance with name into this import manager.
  void exportComponentInstance(const AST::Component::ComponentName &Name,
                               const ComponentInstance *Inst) noexcept {
    NamedCompInst.emplace(Name.getFullName(), Inst);
  }

  // Export core function instance with name into this import manager.
  void exportCoreFunctionInstance(const AST::Component::ComponentName &Name,
                                  FunctionInstance *Inst) noexcept {
    NamedCoreFunc.emplace(Name.getFullName(), Inst);
  }
  void exportCoreFunctionInstance(std::string_view Name,
                                  FunctionInstance *Inst) noexcept {
    NamedCoreFunc.emplace(Name, Inst);
  }

  // Export core table instance with name into this import manager.
  void exportCoreTableInstance(const AST::Component::ComponentName &Name,
                               TableInstance *Inst) noexcept {
    NamedCoreTable.emplace(Name.getFullName(), Inst);
  }
  void exportCoreTableInstance(std::string_view Name,
                               TableInstance *Inst) noexcept {
    NamedCoreTable.emplace(Name, Inst);
  }

  // Export core memory instance with name into this import manager.
  void exportCoreMemoryInstance(const AST::Component::ComponentName &Name,
                                MemoryInstance *Inst) noexcept {
    NamedCoreMemory.emplace(Name.getFullName(), Inst);
  }
  void exportCoreMemoryInstance(std::string_view Name,
                                MemoryInstance *Inst) noexcept {
    NamedCoreMemory.emplace(Name, Inst);
  }

  // Export core global instance with name into this import manager.
  void exportCoreGlobalInstance(const AST::Component::ComponentName &Name,
                                GlobalInstance *Inst) noexcept {
    NamedCoreGlobal.emplace(Name.getFullName(), Inst);
  }
  void exportCoreGlobalInstance(std::string_view Name,
                                GlobalInstance *Inst) noexcept {
    NamedCoreGlobal.emplace(Name, Inst);
  }

  // Export core module instance with name into this import manager.
  void exportCoreModuleInstance(const AST::Component::ComponentName &Name,
                                const ModuleInstance *Inst) noexcept {
    NamedCoreModInst.emplace(Name.getFullName(), Inst);
  }
  void exportCoreModuleInstance(std::string_view Name,
                                const ModuleInstance *Inst) noexcept {
    NamedCoreModInst.emplace(Name, Inst);
  }

  // Find component func by name.
  Component::FunctionInstance *
  findFunction(const AST::Component::ComponentName &Name) const noexcept {
    return findExport(NamedFunc, Name.getFullName());
  }

  // Find component instance by name.
  const ComponentInstance *findComponentInstance(
      const AST::Component::ComponentName &Name) const noexcept {
    return findExport(NamedCompInst, Name.getFullName());
  }

  // Find core function instance by name.
  FunctionInstance *
  findCoreFunctionInstance(std::string_view Name) const noexcept {
    return findExport(NamedCoreFunc, Name);
  }

  // Find core table instance by name.
  TableInstance *findCoreTableInstance(std::string_view Name) const noexcept {
    return findExport(NamedCoreTable, Name);
  }

  // Find core memory instance by name.
  MemoryInstance *findCoreMemoryInstance(std::string_view Name) const noexcept {
    return findExport(NamedCoreMemory, Name);
  }

  // Find core global instance by name.
  GlobalInstance *findCoreGlobalInstance(std::string_view Name) const noexcept {
    return findExport(NamedCoreGlobal, Name);
  }

  // Find core module instance by name.
  const ModuleInstance *
  findCoreModuleInstance(std::string_view Name) const noexcept {
    return findExport(NamedCoreModInst, Name);
  }

  // Reset the import manager.
  void reset() noexcept {
    NamedFunc.clear();
    NamedCompInst.clear();
    NamedCoreFunc.clear();
    NamedCoreTable.clear();
    NamedCoreMemory.clear();
    NamedCoreGlobal.clear();
    NamedCoreModInst.clear();
  }

private:
  // Find export template.
  template <typename T>
  T *findExport(const std::map<std::string, T *, std::less<>> &Map,
                std::string_view ExtName) const noexcept {
    auto Iter = Map.find(ExtName);
    if (likely(Iter != Map.cend())) {
      return Iter->second;
    }
    return nullptr;
  }

  // Export with name for the index spaces.
  std::map<std::string, Component::FunctionInstance *, std::less<>> NamedFunc;
  // TODO: NamedValue
  // TODO: NamedType
  std::map<std::string, const ComponentInstance *, std::less<>> NamedCompInst;
  // TODO: NamedComp
  std::map<std::string, FunctionInstance *, std::less<>> NamedCoreFunc;
  std::map<std::string, TableInstance *, std::less<>> NamedCoreTable;
  std::map<std::string, MemoryInstance *, std::less<>> NamedCoreMemory;
  std::map<std::string, GlobalInstance *, std::less<>> NamedCoreGlobal;
  // TODO: NamedCoreType
  std::map<std::string, const ModuleInstance *, std::less<>> NamedCoreModInst;
  // TODO: NamedCoreMod
};

class ComponentInstance {
  // The component instance class is not only for the runtime data structure,
  // but also for the instantiation context according to the linking isolation
  // and the module and component type declarations.
public:
  ComponentInstance(std::string_view Name) : CompName(Name) {}

  // Getter of the component name.
  std::string_view getComponentName() const noexcept { return CompName; }

  // Instantiation finalizer. Should clean up all instantiation time data.
  void finishInstantiation() noexcept {
    Comps.clear();
    CoreMods.clear();
  }

  // values stored in component instance
  ComponentValVariant getValue(uint32_t Index) const noexcept {
    if (ValueList.size() > Index) {
      return ValueList[Index];
    }
    return 0;
  }
  void setValue(uint32_t Index, ComponentValVariant V) noexcept {
    if (ValueList.size() <= Index) {
      ValueList.resize(Index + 1, 0U);
    }
    ValueList[Index] = V;
  }

  // Index space: component function instance.
  void
  addFunction(std::unique_ptr<Component::FunctionInstance> &&Inst) noexcept {
    OwnedFuncInsts.push_back(std::move(Inst));
    FuncInsts.push_back(OwnedFuncInsts.back().get());
  }
  void addFunction(Component::FunctionInstance *Inst) noexcept {
    FuncInsts.push_back(Inst);
  }
  Component::FunctionInstance *getFunction(uint32_t Index) const noexcept {
    return FuncInsts[Index];
  }
  void exportFunction(const AST::Component::ComponentName &Name,
                      uint32_t Idx) noexcept {
    ExpFuncInsts.insert_or_assign(Name.getFullName(), FuncInsts[Idx]);
  }
  Component::FunctionInstance *
  findFunction(const AST::Component::ComponentName &Name) const noexcept {
    return findExport(ExpFuncInsts, Name.getFullName());
  }
  template <typename CallbackT>
  auto getFuncExports(CallbackT &&CallBack) const noexcept {
    return std::forward<CallbackT>(CallBack)(ExpFuncInsts);
  }

  // Index space: type.
  // TODO: deep copy the type
  void addType(const AST::Component::DefType &Ty) noexcept {
    Types.emplace_back(&Ty);
  }
  const AST::Component::DefType *getType(uint32_t Index) const noexcept {
    return Types[Index];
  }
  void exportType(const AST::Component::ComponentName &Name,
                  uint32_t Idx) noexcept {
    ExpTypes.insert_or_assign(Name.getFullName(), Types[Idx]);
  }
  const AST::Component::DefType *
  findType(const AST::Component::ComponentName &Name) const noexcept {
    return findExport(ExpTypes, Name.getFullName());
  }

  // Index space: component instance.
  void
  addComponentInstance(std::unique_ptr<ComponentInstance> &&Inst) noexcept {
    OwnedCompInsts.push_back(std::move(Inst));
    CompInsts.push_back(OwnedCompInsts.back().get());
  }
  void addComponentInstance(const ComponentInstance *Inst) noexcept {
    CompInsts.push_back(Inst);
  }
  const ComponentInstance *getComponentInstance(uint32_t Index) const noexcept {
    return CompInsts[Index];
  }
  void exportComponentInstance(const AST::Component::ComponentName &Name,
                               uint32_t Idx) noexcept {
    ExpCompInsts.insert_or_assign(Name.getFullName(), CompInsts[Idx]);
  }
  const ComponentInstance *findComponentInstance(
      const AST::Component::ComponentName &Name) const noexcept {
    return findExport(ExpCompInsts, Name.getFullName());
  }

  // Index space: component. (declaration for instantiation phase)
  void addComponent(const AST::Component::Component &C) noexcept {
    Comps.emplace_back(&C);
  }
  const AST::Component::Component &getComponent(uint32_t Index) const noexcept {
    return *Comps[Index];
  }

  // Index space: core function.
  void addCoreFunction(std::unique_ptr<FunctionInstance> &&Inst) noexcept {
    OwnedCoreFuncInsts.push_back(std::move(Inst));
    CoreFuncInsts.push_back(OwnedCoreFuncInsts.back().get());
  }
  void addCoreFunction(FunctionInstance *Inst) noexcept {
    CoreFuncInsts.push_back(Inst);
  }
  FunctionInstance *getCoreFunction(uint32_t Index) const noexcept {
    return CoreFuncInsts[Index];
  }
  void exportCoreFunction(const AST::Component::ComponentName &Name,
                          uint32_t Idx) noexcept {
    ExpCoreFuncInsts.insert_or_assign(Name.getFullName(), CoreFuncInsts[Idx]);
  }
  FunctionInstance *
  findCoreFunction(const AST::Component::ComponentName &Name) const noexcept {
    return findExport(ExpCoreFuncInsts, Name.getFullName());
  }

  // Index space: core table.
  void addCoreTable(TableInstance *Inst) noexcept {
    CoreTabInsts.push_back(Inst);
  }
  TableInstance *getCoreTable(uint32_t Index) const noexcept {
    return CoreTabInsts[Index];
  }
  void exportCoreTable(const AST::Component::ComponentName &Name,
                       uint32_t Idx) noexcept {
    ExpCoreTabInsts.insert_or_assign(Name.getFullName(), CoreTabInsts[Idx]);
  }
  TableInstance *
  findCoreTable(const AST::Component::ComponentName &Name) const noexcept {
    return findExport(ExpCoreTabInsts, Name.getFullName());
  }

  // Index space: core memory.
  void addCoreMemory(MemoryInstance *Inst) noexcept {
    CoreMemInsts.push_back(Inst);
  }
  MemoryInstance *getCoreMemory(uint32_t Index) const noexcept {
    return CoreMemInsts[Index];
  }
  void exportCoreMemory(const AST::Component::ComponentName &Name,
                        uint32_t Idx) noexcept {
    ExpCoreMemInsts.insert_or_assign(Name.getFullName(), CoreMemInsts[Idx]);
  }
  MemoryInstance *
  findCoreMemory(const AST::Component::ComponentName &Name) const noexcept {
    return findExport(ExpCoreMemInsts, Name.getFullName());
  }

  // Index space: core glocal.
  void addCoreGlobal(GlobalInstance *Inst) noexcept {
    CoreGlobInsts.push_back(Inst);
  }
  GlobalInstance *getCoreGlobal(uint32_t Index) const noexcept {
    return CoreGlobInsts[Index];
  }
  void exportCoreGlobal(const AST::Component::ComponentName &Name,
                        uint32_t Idx) noexcept {
    ExpCoreGlobInsts.insert_or_assign(Name.getFullName(), CoreGlobInsts[Idx]);
  }
  GlobalInstance *
  findCoreGlobal(const AST::Component::ComponentName &Name) const noexcept {
    return findExport(ExpCoreGlobInsts, Name.getFullName());
  }

  // Index space: core type.
  // TODO: deep copy the type
  void addCoreType(const AST::Component::CoreDefType &Ty) noexcept {
    CoreTypes.emplace_back(&Ty);
  }
  const AST::Component::CoreDefType &
  getCoreType(uint32_t Index) const noexcept {
    return *CoreTypes[Index];
  }

  // Index space: core module instance.
  void addCoreModuleInstance(std::unique_ptr<ModuleInstance> &&Inst) noexcept {
    OwnedCoreModInsts.push_back(std::move(Inst));
    CoreModInsts.push_back(OwnedCoreModInsts.back().get());
  }
  const ModuleInstance *getCoreModuleInstance(uint32_t Index) const noexcept {
    return CoreModInsts[Index];
  }
  void exportCoreModuleInstance(const AST::Component::ComponentName &Name,
                                uint32_t Idx) noexcept {
    ExpCoreModInsts.insert_or_assign(Name.getFullName(), CoreModInsts[Idx]);
  }
  const ModuleInstance *findCoreModuleInstance(
      const AST::Component::ComponentName &Name) const noexcept {
    return findExport(ExpCoreModInsts, Name.getFullName());
  }

  // Index space: module. (declaration for instantiation phase)
  void addModule(const AST::Module &M) noexcept { CoreMods.emplace_back(&M); }
  const AST::Module &getModule(uint32_t Index) const noexcept {
    return *CoreMods[Index];
  }

private:
  std::string CompName;

  // value
  std::vector<ComponentValVariant> ValueList;

  // Index spaces.
  // The index spaces of AST should be cleaned after instantiation.
  std::vector<Component::FunctionInstance *> FuncInsts;
  // TODO: values
  std::vector<const AST::Component::DefType *> Types;
  std::vector<const ComponentInstance *> CompInsts;
  std::vector<const AST::Component::Component *> Comps;
  std::vector<FunctionInstance *> CoreFuncInsts;
  std::vector<TableInstance *> CoreTabInsts;
  std::vector<MemoryInstance *> CoreMemInsts;
  std::vector<GlobalInstance *> CoreGlobInsts;
  std::vector<const AST::Component::CoreDefType *> CoreTypes;
  std::vector<const ModuleInstance *> CoreModInsts;
  std::vector<const AST::Module *> CoreMods;

  // Storage of index spaces.
  std::vector<std::unique_ptr<Component::FunctionInstance>> OwnedFuncInsts;
  // std::vector<std::unique_ptr<AST::Component::DefType>> OwnedTypes;
  std::vector<std::unique_ptr<ComponentInstance>> OwnedCompInsts;
  std::vector<std::unique_ptr<FunctionInstance>> OwnedCoreFuncInsts;
  // std::vector<std::unique_ptr<AST::Component::CoreDefType>> OwnedCoreTypes;
  std::vector<std::unique_ptr<ModuleInstance>> OwnedCoreModInsts;

  // Export alias.
  std::map<std::string, Component::FunctionInstance *, std::less<>>
      ExpFuncInsts;
  // TODO: ExpValue
  std::map<std::string, const AST::Component::DefType *, std::less<>> ExpTypes;
  std::map<std::string, const ComponentInstance *, std::less<>> ExpCompInsts;
  // TODO: ExpComps
  std::map<std::string, FunctionInstance *, std::less<>> ExpCoreFuncInsts;
  std::map<std::string, TableInstance *, std::less<>> ExpCoreTabInsts;
  std::map<std::string, MemoryInstance *, std::less<>> ExpCoreMemInsts;
  std::map<std::string, GlobalInstance *, std::less<>> ExpCoreGlobInsts;
  // TODO: ExpCoreTypes
  std::map<std::string, const ModuleInstance *, std::less<>> ExpCoreModInsts;
  // TODO: ExpCoreMods

  // Find export template.
  template <typename T>
  T *findExport(const std::map<std::string, T *, std::less<>> &Map,
                std::string_view ExtName) const noexcept {
    auto Iter = Map.find(ExtName);
    if (likely(Iter != Map.cend())) {
      return Iter->second;
    }
    return nullptr;
  }
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
