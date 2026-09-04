// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/runtime/instance/component/component.h - Component Instance -//
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
#include "common/types.h"
#include "runtime/instance/component/concurrencymgr.h"
#include "runtime/instance/component/function.h"
#include "runtime/instance/component/handlemgr.h"
#include "runtime/instance/component/resource.h"
#include "runtime/instance/module.h"
#include "runtime/instance/tag.h"

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Runtime {
namespace Instance {

/// The component instance: both the runtime data structure and the
/// instantiation context, following the linking isolation of the module and
/// component type declarations.
class ComponentInstance {
public:
  /// A component index entry: the definition plus the lexical environment the
  /// value closed over.
  struct ComponentEntry {
    const AST::Component::Component *Ast;
    const ComponentInstance *Env;
  };

  /// Constructor. The lexical parent P of a nested instance is what its outer
  /// aliases resolve through during instantiation.
  ComponentInstance(std::string_view Name, const ComponentInstance *P = nullptr)
      : CompName(Name), Parent(P) {}
  /// The threads of this instance end here, before any member it holds goes
  /// away. The task manager installs the hook when it spawns the first one.
  ~ComponentInstance() noexcept { Concurrency->runDestroyHook(); }

  /// Getter for the component name.
  std::string_view getComponentName() const noexcept { return CompName; }

  /// Getter for the lexical parent.
  const ComponentInstance *getParent() const noexcept { return Parent; }

  /// Root of the lexical instantiation tree (poisoning + host-entry checks).
  const ComponentInstance *getRoot() const noexcept {
    const ComponentInstance *R = this;
    while (R->Parent != nullptr) {
      R = R->Parent;
    }
    return R;
  }

  /// True when callee and caller are the same instance or lexical relatives,
  /// for which an adapter call always traps.
  bool isLinealRelativeOf(const ComponentInstance *Other) const noexcept {
    if (Other == nullptr) {
      return false;
    }
    for (const ComponentInstance *P = this; P != nullptr; P = P->Parent) {
      if (P == Other) {
        return true;
      }
    }
    for (const ComponentInstance *P = Other; P != nullptr; P = P->Parent) {
      if (P == this) {
        return true;
      }
    }
    return false;
  }

  /// The mutable runtime state. Both live behind an owning pointer, so a const
  /// instance still hands out a mutable reference and no member needs mutable.
  Component::HandleManager &handles() const noexcept { return *Handles; }
  Component::ConcurrencyManager &concurrency() const noexcept {
    return *Concurrency;
  }

  /// Index space: value.
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
  void addValue(ComponentValVariant V) noexcept {
    ValueList.push_back(std::move(V));
  }
  void exportValue(std::string_view Name, ComponentValVariant V) noexcept {
    ExpValues.insert_or_assign(std::string(Name), std::move(V));
  }
  const ComponentValVariant *findValue(std::string_view Name) const noexcept {
    auto It = ExpValues.find(Name);
    return It != ExpValues.end() ? &It->second : nullptr;
  }

  /// Index space: component function instance.
  void addFunction(std::unique_ptr<ComponentFunctionInstance> &&Inst) noexcept {
    OwnedFuncInsts.push_back(std::move(Inst));
    FuncInsts.push_back(OwnedFuncInsts.back().get());
  }
  void addFunction(ComponentFunctionInstance *Inst) noexcept {
    FuncInsts.push_back(Inst);
  }
  /// Host component function registered under an export name.
  void addHostFunc(std::string_view Name,
                   std::unique_ptr<ComponentFunctionInstance> &&Inst) noexcept {
    addFunction(std::move(Inst));
    exportFunction(Name, static_cast<uint32_t>(FuncInsts.size() - 1));
  }
  ComponentFunctionInstance *getFunction(uint32_t Index) const noexcept {
    return Index < FuncInsts.size() ? FuncInsts[Index] : nullptr;
  }
  void exportFunction(std::string_view Name, uint32_t Idx) noexcept {
    if (Idx < FuncInsts.size()) {
      ExpFuncInsts.insert_or_assign(std::string(Name), FuncInsts[Idx]);
    }
  }
  ComponentFunctionInstance *
  findFunction(std::string_view Name) const noexcept {
    return findExport(ExpFuncInsts, Name);
  }
  /// Find an exported function by the embedder-facing path: either a top-level
  /// export name, or `interface#func` reaching into an exported instance.
  ComponentFunctionInstance *
  findExportedFunction(std::string_view Name) const noexcept {
    if (auto *Func = findFunction(Name); Func != nullptr) {
      return Func;
    }
    const auto Pos = Name.find('#');
    if (Pos == std::string_view::npos) {
      return nullptr;
    }
    const auto *Inst = findComponentInstance(Name.substr(0, Pos));
    return Inst != nullptr ? Inst->findFunction(Name.substr(Pos + 1)) : nullptr;
  }
  template <typename CallbackT>
  auto getFuncExports(CallbackT &&CallBack) const noexcept {
    return std::forward<CallbackT>(CallBack)(ExpFuncInsts);
  }

  /// Index space: type.
  void addType(const AST::Component::DefType &Ty) noexcept {
    Types.emplace_back(&Ty);
    TypeResources.emplace_back(nullptr);
  }
  /// Host-built type entry: the instance owns the definition.
  uint32_t addOwnedType(AST::Component::DefType &&Ty) noexcept {
    OwnedDefTypes.push_back(
        std::make_unique<AST::Component::DefType>(std::move(Ty)));
    addType(*OwnedDefTypes.back());
    return static_cast<uint32_t>(Types.size() - 1);
  }
  /// A locally-defined resource type: mints the runtime identity.
  const Component::ResourceTypeInstance *
  addResourceType(const AST::Component::DefType &Ty,
                  FunctionInstance *Dtor) noexcept {
    OwnedResourceTypes.push_back(
        std::make_unique<Component::ResourceTypeInstance>());
    OwnedResourceTypes.back()->Impl = this;
    OwnedResourceTypes.back()->Dtor = Dtor;
    OwnedResourceTypes.back()->RepI64 =
        Ty.isResourceType() && Ty.getResourceType().isAddrI64();
    Types.emplace_back(&Ty);
    TypeResources.emplace_back(OwnedResourceTypes.back().get());
    return OwnedResourceTypes.back().get();
  }
  /// Host-defined resource type with a host destructor.
  uint32_t addHostResourceType(std::function<void(uint64_t)> Dtor) noexcept {
    OwnedResourceTypes.push_back(
        std::make_unique<Component::ResourceTypeInstance>());
    OwnedResourceTypes.back()->Impl = this;
    OwnedResourceTypes.back()->HostDtor = std::move(Dtor);
    Types.emplace_back(nullptr);
    TypeResources.emplace_back(OwnedResourceTypes.back().get());
    return static_cast<uint32_t>(Types.size() - 1);
  }
  /// An imported or aliased resource type entry sharing an existing identity.
  void addTypeWithResource(const AST::Component::DefType *Ty,
                           const Component::ResourceTypeInstance *RT) noexcept {
    Types.emplace_back(Ty);
    TypeResources.emplace_back(RT);
  }
  const AST::Component::DefType *getType(uint32_t Index) const noexcept {
    return Index < Types.size() ? Types[Index] : nullptr;
  }
  const Component::ResourceTypeInstance *
  getTypeResource(uint32_t Index) const noexcept {
    return Index < TypeResources.size() ? TypeResources[Index] : nullptr;
  }
  void exportType(std::string_view Name, uint32_t Idx) noexcept {
    if (Idx < Types.size()) {
      ExpTypes.insert_or_assign(
          std::string(Name), std::make_pair(Types[Idx], getTypeResource(Idx)));
    }
  }
  const AST::Component::DefType *
  findType(std::string_view Name) const noexcept {
    auto It = ExpTypes.find(Name);
    return It != ExpTypes.end() ? It->second.first : nullptr;
  }
  const Component::ResourceTypeInstance *
  findTypeResource(std::string_view Name) const noexcept {
    auto It = ExpTypes.find(Name);
    return It != ExpTypes.end() ? It->second.second : nullptr;
  }

  /// Index space: component instance.
  void
  addComponentInstance(std::unique_ptr<ComponentInstance> &&Inst) noexcept {
    OwnedCompInsts.push_back(std::move(Inst));
    CompInsts.push_back(OwnedCompInsts.back().get());
  }
  void addComponentInstance(const ComponentInstance *Inst) noexcept {
    CompInsts.push_back(Inst);
  }
  const ComponentInstance *getComponentInstance(uint32_t Index) const noexcept {
    return Index < CompInsts.size() ? CompInsts[Index] : nullptr;
  }
  uint32_t getComponentInstanceCount() const noexcept {
    return static_cast<uint32_t>(CompInsts.size());
  }
  void exportComponentInstance(std::string_view Name, uint32_t Idx) noexcept {
    if (Idx < CompInsts.size()) {
      ExpCompInsts.insert_or_assign(std::string(Name), CompInsts[Idx]);
    }
  }
  const ComponentInstance *
  findComponentInstance(std::string_view Name) const noexcept {
    return findExport(ExpCompInsts, Name);
  }
  template <typename CallbackT>
  auto getComponentInstanceExports(CallbackT &&CallBack) const noexcept {
    return std::forward<CallbackT>(CallBack)(ExpCompInsts);
  }

  /// Index space: component. A component value closes over its lexical
  /// environment, so its outer aliases resolve against the defining instance.
  void addComponent(const AST::Component::Component &C) noexcept {
    Comps.push_back({&C, this});
  }
  void addComponentEntry(const AST::Component::Component *C,
                         const ComponentInstance *Env) noexcept {
    Comps.push_back({C, Env});
  }
  const AST::Component::Component *getComponent(uint32_t Index) const noexcept {
    return Index < Comps.size() ? Comps[Index].Ast : nullptr;
  }
  const ComponentInstance *getComponentEnv(uint32_t Index) const noexcept {
    return Index < Comps.size() ? Comps[Index].Env : nullptr;
  }
  void exportComponent(std::string_view Name, uint32_t Idx) noexcept {
    if (Idx < Comps.size()) {
      ExpComps.insert_or_assign(std::string(Name), Comps[Idx]);
    }
  }
  const ComponentEntry *
  findComponentEntry(std::string_view Name) const noexcept {
    auto It = ExpComps.find(Name);
    return It != ExpComps.end() ? &It->second : nullptr;
  }

  /// Index space: core function.
  void addCoreFunction(std::unique_ptr<FunctionInstance> &&Inst) noexcept {
    OwnedCoreFuncInsts.push_back(std::move(Inst));
    CoreFuncInsts.push_back(OwnedCoreFuncInsts.back().get());
  }
  void addCoreFunction(FunctionInstance *Inst) noexcept {
    CoreFuncInsts.push_back(Inst);
  }
  /// Add a host function to the core function index space, owned by an
  /// auxiliary ModuleInstance that registers its type for import matching.
  void addCoreHostFunction(std::unique_ptr<HostFunctionBase> &&Host,
                           std::string_view Name = "$canon-lower") {
    auto Mod = std::make_unique<ModuleInstance>("");
    Mod->addHostFunc(std::string(Name), std::move(Host));
    auto *FuncPtr = Mod->findFuncExports(std::string(Name));
    CoreFuncInsts.push_back(FuncPtr);
    OwnedAuxModInsts.push_back(std::move(Mod));
  }
  FunctionInstance *getCoreFunction(uint32_t Index) const noexcept {
    return Index < CoreFuncInsts.size() ? CoreFuncInsts[Index] : nullptr;
  }

  /// Index space: core table.
  void addCoreTable(TableInstance *Inst) noexcept {
    CoreTabInsts.push_back(Inst);
  }
  TableInstance *getCoreTable(uint32_t Index) const noexcept {
    return Index < CoreTabInsts.size() ? CoreTabInsts[Index] : nullptr;
  }

  /// Index space: core memory.
  void addCoreMemory(MemoryInstance *Inst) noexcept {
    CoreMemInsts.push_back(Inst);
  }
  MemoryInstance *getCoreMemory(uint32_t Index) const noexcept {
    return Index < CoreMemInsts.size() ? CoreMemInsts[Index] : nullptr;
  }

  /// Index space: core global.
  void addCoreGlobal(GlobalInstance *Inst) noexcept {
    CoreGlobInsts.push_back(Inst);
  }
  GlobalInstance *getCoreGlobal(uint32_t Index) const noexcept {
    return Index < CoreGlobInsts.size() ? CoreGlobInsts[Index] : nullptr;
  }

  /// Index space: core tag.
  void addCoreTag(TagInstance *Inst) noexcept { CoreTagInsts.push_back(Inst); }
  TagInstance *getCoreTag(uint32_t Index) const noexcept {
    return Index < CoreTagInsts.size() ? CoreTagInsts[Index] : nullptr;
  }

  /// Index space: core type.
  void addCoreType(const AST::Component::CoreDefType &Ty) noexcept {
    CoreTypes.emplace_back(&Ty);
  }
  const AST::Component::CoreDefType *
  getCoreType(uint32_t Index) const noexcept {
    return Index < CoreTypes.size() ? CoreTypes[Index] : nullptr;
  }

  /// Index space: core module instance.
  void addCoreModuleInstance(std::unique_ptr<ModuleInstance> &&Inst) noexcept {
    OwnedCoreModInsts.push_back(std::move(Inst));
    CoreModInsts.push_back(OwnedCoreModInsts.back().get());
  }
  void addCoreModuleInstance(const ModuleInstance *Inst) noexcept {
    CoreModInsts.push_back(Inst);
  }
  const ModuleInstance *getCoreModuleInstance(uint32_t Index) const noexcept {
    return Index < CoreModInsts.size() ? CoreModInsts[Index] : nullptr;
  }
  void exportCoreModuleInstance(std::string_view Name, uint32_t Idx) noexcept {
    if (Idx < CoreModInsts.size()) {
      ExpCoreModInsts.insert_or_assign(std::string(Name), CoreModInsts[Idx]);
    }
  }
  const ModuleInstance *
  findCoreModuleInstance(std::string_view Name) const noexcept {
    return findExport(ExpCoreModInsts, Name);
  }

  /// Index space: core module.
  void addModule(const AST::Module &M) noexcept { CoreMods.emplace_back(&M); }
  const AST::Module *getModule(uint32_t Index) const noexcept {
    return Index < CoreMods.size() ? CoreMods[Index] : nullptr;
  }
  void exportCoreModule(std::string_view Name, uint32_t Idx) noexcept {
    if (Idx < CoreMods.size()) {
      ExpCoreMods.insert_or_assign(std::string(Name), CoreMods[Idx]);
    }
  }
  const AST::Module *findCoreModule(std::string_view Name) const noexcept {
    return findExport(ExpCoreMods, Name);
  }

private:
  /// Find export template.
  template <typename T>
  T *findExport(const std::map<std::string, T *, std::less<>> &Map,
                std::string_view ExtName) const noexcept {
    auto Iter = Map.find(ExtName);
    if (likely(Iter != Map.cend())) {
      return Iter->second;
    }
    return nullptr;
  }

  /// \name Data of component instance.
  /// @{
  const std::string CompName;
  const ComponentInstance *const Parent;
  /// The mutable runtime state behind handles() and concurrency().
  const std::unique_ptr<Component::HandleManager> Handles =
      std::make_unique<Component::HandleManager>();
  const std::unique_ptr<Component::ConcurrencyManager> Concurrency =
      std::make_unique<Component::ConcurrencyManager>();

  /// Index spaces.
  std::vector<ComponentValVariant> ValueList;
  std::vector<ComponentFunctionInstance *> FuncInsts;
  std::vector<const AST::Component::DefType *> Types;
  std::vector<const Component::ResourceTypeInstance *> TypeResources;
  std::vector<const ComponentInstance *> CompInsts;
  std::vector<ComponentEntry> Comps;
  std::vector<FunctionInstance *> CoreFuncInsts;
  std::vector<TableInstance *> CoreTabInsts;
  std::vector<MemoryInstance *> CoreMemInsts;
  std::vector<GlobalInstance *> CoreGlobInsts;
  std::vector<TagInstance *> CoreTagInsts;
  std::vector<const AST::Component::CoreDefType *> CoreTypes;
  std::vector<const ModuleInstance *> CoreModInsts;
  std::vector<const AST::Module *> CoreMods;

  /// Owned entries of the index spaces.
  std::vector<std::unique_ptr<Component::ResourceTypeInstance>>
      OwnedResourceTypes;
  std::vector<std::unique_ptr<AST::Component::DefType>> OwnedDefTypes;
  std::vector<std::unique_ptr<ComponentFunctionInstance>> OwnedFuncInsts;
  std::vector<std::unique_ptr<ComponentInstance>> OwnedCompInsts;
  std::vector<std::unique_ptr<FunctionInstance>> OwnedCoreFuncInsts;
  std::vector<std::unique_ptr<ModuleInstance>> OwnedCoreModInsts;
  /// Holder modules for synthesized host functions, owning the function and
  /// its registered SubType for matchType lookups.
  std::vector<std::unique_ptr<ModuleInstance>> OwnedAuxModInsts;

  /// Exported name maps.
  std::map<std::string, ComponentValVariant, std::less<>> ExpValues;
  std::map<std::string, ComponentFunctionInstance *, std::less<>> ExpFuncInsts;
  std::map<std::string,
           std::pair<const AST::Component::DefType *,
                     const Component::ResourceTypeInstance *>,
           std::less<>>
      ExpTypes;
  std::map<std::string, const ComponentInstance *, std::less<>> ExpCompInsts;
  std::map<std::string, ComponentEntry, std::less<>> ExpComps;
  std::map<std::string, const ModuleInstance *, std::less<>> ExpCoreModInsts;
  std::map<std::string, const AST::Module *, std::less<>> ExpCoreMods;
  /// @}
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
