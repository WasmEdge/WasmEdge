// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

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
#include "runtime/instance/component/async.h"
#include "runtime/instance/component/function.h"
#include "runtime/instance/module.h"
#include "runtime/instance/tag.h"

#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

namespace WasmEdge {
namespace Runtime {
namespace Instance {

class ComponentInstance;

class ComponentImportManager {
  // The import manager is used to supply imports for locally instantiated child
  // components and core modules.
public:
  // Export a named component func to this import manager.
  void exportFunction(std::string_view Name,
                      Component::FunctionInstance *Inst) noexcept {
    NamedFunc.emplace(Name, Inst);
  }

  // Export a named component instance to this import manager.
  void exportComponentInstance(std::string_view Name,
                               const ComponentInstance *Inst) noexcept {
    NamedCompInst.emplace(Name, Inst);
  }

  // Export a named core function instance to this import manager.
  void exportCoreFunctionInstance(std::string_view Name,
                                  FunctionInstance *Inst) noexcept {
    NamedCoreFunc.emplace(Name, Inst);
  }

  // Export a named core table instance to this import manager.
  void exportCoreTableInstance(std::string_view Name,
                               TableInstance *Inst) noexcept {
    NamedCoreTable.emplace(Name, Inst);
  }

  // Export a named core memory instance to this import manager.
  void exportCoreMemoryInstance(std::string_view Name,
                                MemoryInstance *Inst) noexcept {
    NamedCoreMemory.emplace(Name, Inst);
  }

  // Export a named core global instance to this import manager.
  void exportCoreGlobalInstance(std::string_view Name,
                                GlobalInstance *Inst) noexcept {
    NamedCoreGlobal.emplace(Name, Inst);
  }

  // Export a named core module instance to this import manager.
  void exportCoreModuleInstance(std::string_view Name,
                                const ModuleInstance *Inst) noexcept {
    NamedCoreModInst.emplace(Name, Inst);
  }

  // Export a named value to this import manager.
  void exportValue(std::string_view Name, ComponentValVariant V) noexcept {
    NamedValue.insert_or_assign(std::string(Name), std::move(V));
  }
  const ComponentValVariant *findValue(std::string_view Name) const noexcept {
    auto It = NamedValue.find(Name);
    return It != NamedValue.end() ? &It->second : nullptr;
  }

  // Export a named type to this import manager.
  void exportType(std::string_view Name, const AST::Component::DefType *Ty,
                  const void *ResourceRT = nullptr) noexcept {
    NamedType.emplace(std::string(Name), std::make_pair(Ty, ResourceRT));
  }

  // Export a named component definition to this import manager.
  void exportComponent(std::string_view Name,
                       const AST::Component::Component *C,
                       const ComponentInstance *Env) noexcept {
    NamedComp.emplace(std::string(Name), std::make_pair(C, Env));
  }

  // Export a named core module (definition) to this import manager.
  void exportCoreModule(std::string_view Name, const AST::Module *M) noexcept {
    NamedCoreMod.emplace(Name, M);
  }

  // Find component func by name.
  Component::FunctionInstance *
  findFunction(std::string_view Name) const noexcept {
    return findExport(NamedFunc, Name);
  }

  // Find a named type (definition and optional resource identity).
  const AST::Component::DefType *
  findType(std::string_view Name) const noexcept {
    auto It = NamedType.find(std::string(Name));
    return It != NamedType.end() ? It->second.first : nullptr;
  }
  const void *findTypeResource(std::string_view Name) const noexcept {
    auto It = NamedType.find(std::string(Name));
    return It != NamedType.end() ? It->second.second : nullptr;
  }

  // Find a named component definition (AST) or its environment.
  const AST::Component::Component *
  findComponent(std::string_view Name) const noexcept {
    auto It = NamedComp.find(std::string(Name));
    return It != NamedComp.end() ? It->second.first : nullptr;
  }
  const ComponentInstance *
  findComponentEnv(std::string_view Name) const noexcept {
    auto It = NamedComp.find(std::string(Name));
    return It != NamedComp.end() ? It->second.second : nullptr;
  }
  bool hasComponent(std::string_view Name) const noexcept {
    return NamedComp.count(std::string(Name)) != 0;
  }

  // Find a named core module definition.
  const AST::Module *findCoreModule(std::string_view Name) const noexcept {
    return findExport(NamedCoreMod, Name);
  }

  // Find component instance by name.
  const ComponentInstance *
  findComponentInstance(std::string_view Name) const noexcept {
    return findExport(NamedCompInst, Name);
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
    NamedValue.clear();
    NamedType.clear();
    NamedComp.clear();
    NamedCoreMod.clear();
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
  std::map<std::string, ComponentValVariant, std::less<>> NamedValue;
  std::map<std::string,
           std::pair<const AST::Component::DefType *, const void *>>
      NamedType;
  std::map<std::string, const ComponentInstance *, std::less<>> NamedCompInst;
  std::map<std::string, std::pair<const AST::Component::Component *,
                                  const ComponentInstance *>>
      NamedComp;
  std::map<std::string, const AST::Module *, std::less<>> NamedCoreMod;
  std::map<std::string, FunctionInstance *, std::less<>> NamedCoreFunc;
  std::map<std::string, TableInstance *, std::less<>> NamedCoreTable;
  std::map<std::string, MemoryInstance *, std::less<>> NamedCoreMemory;
  std::map<std::string, GlobalInstance *, std::less<>> NamedCoreGlobal;
  // TODO: NamedCoreType
  std::map<std::string, const ModuleInstance *, std::less<>> NamedCoreModInst;
  // TODO: NamedCoreMod
};

class ComponentInstance {
  // The component instance class is used not only for the runtime data
  // structure but also for the instantiation context according to the linking
  // isolation and the module and component type declarations.
public:
  ComponentInstance(std::string_view Name) : CompName(Name) {}

  // Getter for the component name.
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
  void addValue(ComponentValVariant V) noexcept {
    ValueList.push_back(std::move(V));
  }
  uint32_t getValueCount() const noexcept {
    return static_cast<uint32_t>(ValueList.size());
  }
  void exportValue(std::string_view Name, ComponentValVariant V) noexcept {
    ExpValues.insert_or_assign(std::string(Name), std::move(V));
  }
  const ComponentValVariant *
  findValueExport(std::string_view Name) const noexcept {
    auto It = ExpValues.find(Name);
    return It != ExpValues.end() ? &It->second : nullptr;
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
    return Index < FuncInsts.size() ? FuncInsts[Index] : nullptr;
  }
  void exportFunction(std::string_view Name, uint32_t Idx) noexcept {
    if (Idx < FuncInsts.size()) {
      ExpFuncInsts.insert_or_assign(std::string(Name), FuncInsts[Idx]);
    }
  }
  Component::FunctionInstance *
  findFunction(std::string_view Name) const noexcept {
    return findExport(ExpFuncInsts, Name);
  }
  template <typename CallbackT>
  auto getFuncExports(CallbackT &&CallBack) const noexcept {
    return std::forward<CallbackT>(CallBack)(ExpFuncInsts);
  }

  // Runtime identity of a resource type: the defining instance, the
  // destructor (guest core function or host callback), and the rep width.
  struct ResourceTypeRT {
    const ComponentInstance *Impl = nullptr;
    FunctionInstance *Dtor = nullptr;
    std::function<void(uint32_t)> HostDtor;
  };

  // A resource entry of the canonical `handles` table.
  struct HandleSlot {
    const ResourceTypeRT *RT = nullptr;
    uint32_t Rep = 0;
    bool Own = true;
    uint32_t Lends = 0;
    // Borrow scope: the task that received this borrow (num_borrows
    // accounting); null for own handles and host-boundary borrows.
    Executor::ComponentTask *BorrowScope = nullptr;
  };

  // One slot of the unified per-instance `handles` table: resources,
  // waitables (subtasks and stream/future ends), waitable sets, and error
  // contexts share a single index space (CanonicalABI.md
  // `ComponentInstance.handles`). Waitables are shared: the tasks and
  // rendezvous objects that queue their events keep them alive.
  using TableSlot = std::variant<
      std::monostate, HandleSlot, std::shared_ptr<Component::WaitableObj>,
      std::unique_ptr<Component::WaitableSetObj>, Component::ErrorContextObj>;

  // Index space: type.
  // TODO: deep copy the type
  void addType(const AST::Component::DefType &Ty) noexcept {
    Types.emplace_back(&Ty);
    TypeResources.emplace_back(nullptr);
  }
  // Placeholder entry keeping the type index space aligned when no
  // definition is available (abstract resource imports).
  void addDummyType() noexcept {
    Types.emplace_back(nullptr);
    TypeResources.emplace_back(nullptr);
  }
  // Host-built type entry: the instance owns the definition.
  uint32_t addOwnedType(AST::Component::DefType &&Ty) noexcept {
    OwnedDefTypes.push_back(
        std::make_unique<AST::Component::DefType>(std::move(Ty)));
    addType(*OwnedDefTypes.back());
    return static_cast<uint32_t>(Types.size() - 1);
  }
  // Host-defined resource type with a host destructor.
  uint32_t addHostResourceType(std::function<void(uint32_t)> Dtor) noexcept {
    OwnedResourceTypes.push_back(std::make_unique<ResourceTypeRT>());
    OwnedResourceTypes.back()->Impl = this;
    OwnedResourceTypes.back()->HostDtor = std::move(Dtor);
    Types.emplace_back(nullptr);
    TypeResources.emplace_back(OwnedResourceTypes.back().get());
    return static_cast<uint32_t>(Types.size() - 1);
  }
  uint32_t getTypeCount() const noexcept {
    return static_cast<uint32_t>(Types.size());
  }
  // Host component function registered under an export name.
  void
  addHostFunc(std::string_view Name,
              std::unique_ptr<Component::FunctionInstance> &&Inst) noexcept {
    addFunction(std::move(Inst));
    exportFunction(Name, static_cast<uint32_t>(FuncInsts.size() - 1));
  }

  // A locally-defined resource type: mints the runtime identity.
  const ResourceTypeRT *addResourceType(const AST::Component::DefType &Ty,
                                        FunctionInstance *Dtor) noexcept {
    OwnedResourceTypes.push_back(std::make_unique<ResourceTypeRT>());
    OwnedResourceTypes.back()->Impl = this;
    OwnedResourceTypes.back()->Dtor = Dtor;
    Types.emplace_back(&Ty);
    TypeResources.emplace_back(OwnedResourceTypes.back().get());
    return OwnedResourceTypes.back().get();
  }
  // An imported / aliased resource type entry sharing an existing identity.
  void addTypeWithResource(const AST::Component::DefType *Ty,
                           const ResourceTypeRT *RT) noexcept {
    Types.emplace_back(Ty);
    TypeResources.emplace_back(RT);
  }
  const ResourceTypeRT *getTypeResource(uint32_t Index) const noexcept {
    return Index < TypeResources.size() ? TypeResources[Index] : nullptr;
  }

  // Unified canonical `handles` table (index 0 is never used; LIFO slot
  // reuse). The table mutates through const instance pointers held by
  // canonical contexts.
  uint32_t slotAdd(TableSlot &&Slot) const noexcept {
    if (Handles.empty()) {
      Handles.emplace_back(); // slot 0 stays dead
    }
    if (!FreeSlots.empty()) {
      const uint32_t Idx = FreeSlots.back();
      FreeSlots.pop_back();
      Handles[Idx] = std::move(Slot);
      return Idx;
    }
    Handles.push_back(std::move(Slot));
    return static_cast<uint32_t>(Handles.size() - 1);
  }
  bool slotLive(uint32_t Idx) const noexcept {
    return Idx != 0 && Idx < Handles.size() &&
           !std::holds_alternative<std::monostate>(Handles[Idx]);
  }
  void slotFree(uint32_t Idx) const noexcept {
    Handles[Idx] = std::monostate{};
    FreeSlots.push_back(Idx);
  }

  // Resource entries.
  uint32_t
  handleAdd(const ResourceTypeRT *RT, uint32_t Rep, bool Own,
            Executor::ComponentTask *BorrowScope = nullptr) const noexcept {
    return slotAdd(TableSlot{HandleSlot{RT, Rep, Own, 0, BorrowScope}});
  }
  HandleSlot *handleGet(uint32_t Idx) const noexcept {
    if (!slotLive(Idx)) {
      return nullptr;
    }
    return std::get_if<HandleSlot>(&Handles[Idx]);
  }
  std::optional<HandleSlot> handleRemove(uint32_t Idx) const noexcept {
    auto *Slot = handleGet(Idx);
    if (Slot == nullptr) {
      return std::nullopt;
    }
    HandleSlot Out = *Slot;
    slotFree(Idx);
    return Out;
  }

  // Waitable entries (subtasks and stream/future copy ends).
  uint32_t
  waitableAdd(std::shared_ptr<Component::WaitableObj> W) const noexcept {
    return slotAdd(TableSlot{std::move(W)});
  }
  Component::WaitableObj *waitableGet(uint32_t Idx) const noexcept {
    if (!slotLive(Idx)) {
      return nullptr;
    }
    if (auto *P = std::get_if<std::shared_ptr<Component::WaitableObj>>(
            &Handles[Idx])) {
      return P->get();
    }
    return nullptr;
  }
  std::shared_ptr<Component::WaitableObj>
  waitableRemove(uint32_t Idx) const noexcept {
    if (auto *W = std::get_if<std::shared_ptr<Component::WaitableObj>>(
            slotLive(Idx) ? &Handles[Idx] : nullptr)) {
      auto Out = std::move(*W);
      slotFree(Idx);
      return Out;
    }
    return nullptr;
  }

  // Waitable-set entries.
  uint32_t waitableSetAdd() const noexcept {
    return slotAdd(TableSlot{std::make_unique<Component::WaitableSetObj>()});
  }
  Component::WaitableSetObj *waitableSetGet(uint32_t Idx) const noexcept {
    if (!slotLive(Idx)) {
      return nullptr;
    }
    if (auto *P = std::get_if<std::unique_ptr<Component::WaitableSetObj>>(
            &Handles[Idx])) {
      return P->get();
    }
    return nullptr;
  }
  std::unique_ptr<Component::WaitableSetObj>
  waitableSetRemove(uint32_t Idx) const noexcept {
    if (auto *W = std::get_if<std::unique_ptr<Component::WaitableSetObj>>(
            slotLive(Idx) ? &Handles[Idx] : nullptr)) {
      auto Out = std::move(*W);
      slotFree(Idx);
      return Out;
    }
    return nullptr;
  }

  // Error-context entries.
  uint32_t errorContextAdd(std::string Msg) const noexcept {
    return slotAdd(TableSlot{Component::ErrorContextObj{std::move(Msg)}});
  }
  Component::ErrorContextObj *errorContextGet(uint32_t Idx) const noexcept {
    if (!slotLive(Idx)) {
      return nullptr;
    }
    return std::get_if<Component::ErrorContextObj>(&Handles[Idx]);
  }
  bool errorContextRemove(uint32_t Idx) const noexcept {
    if (errorContextGet(Idx) == nullptr) {
      return false;
    }
    slotFree(Idx);
    return true;
  }

  // The per-instance thread table (spec `ComponentInstance.threads`):
  // thread activations (implicit and spawned) register here; thread.index
  // reads it.
  uint32_t threadAdd(Executor::ComponentThreadCtx *T) const noexcept {
    auto &St = Concurrency;
    if (St.Threads.empty()) {
      St.Threads.push_back(nullptr); // slot 0 stays dead
    }
    if (!St.ThreadFree.empty()) {
      const uint32_t Idx = St.ThreadFree.back();
      St.ThreadFree.pop_back();
      St.Threads[Idx] = T;
      return Idx;
    }
    St.Threads.push_back(T);
    return static_cast<uint32_t>(St.Threads.size() - 1);
  }
  void threadRemove(uint32_t Idx) const noexcept {
    if (Idx != 0 && Idx < Concurrency.Threads.size()) {
      Concurrency.Threads[Idx] = nullptr;
      Concurrency.ThreadFree.push_back(Idx);
    }
  }
  Executor::ComponentThreadCtx *threadGet(uint32_t Idx) const noexcept {
    return Idx != 0 && Idx < Concurrency.Threads.size()
               ? Concurrency.Threads[Idx]
               : nullptr;
  }
  const AST::Component::DefType *getType(uint32_t Index) const noexcept {
    return Index < Types.size() ? Types[Index] : nullptr;
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
  const ResourceTypeRT *findTypeResource(std::string_view Name) const noexcept {
    auto It = ExpTypes.find(Name);
    return It != ExpTypes.end() ? It->second.second : nullptr;
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

  // Index space: component. (declaration for instantiation phase)
  // Lexical parent for outer-alias resolution during instantiation.
  void setParent(const ComponentInstance *P) noexcept { Parent = P; }
  const ComponentInstance *getParent() const noexcept { return Parent; }

  // Reentrance guard: a component instance cannot be entered while it is
  // already executing or still instantiating.
  void setEntered(bool E) const noexcept { Entered = E; }
  bool isEntered() const noexcept { return Entered; }

  // Concurrency state of this instance (spec `ComponentInstance` fields).
  // MayLeave=false models the regions where built-ins and lowered imports
  // must trap "cannot leave component instance" (argument lowering and the
  // post-return call). Poisoned is meaningful on the root instance: after
  // any trap inside the tree, further entries trap "cannot enter".
  struct ConcurrencyState {
    bool MayLeave = true;
    int64_t Backpressure = 0;
    uint32_t NumWaitingToEnter = 0;
    Executor::ComponentTask *ExclusiveTask = nullptr;
    bool Poisoned = false;
    std::vector<Executor::ComponentThreadCtx *> Threads;
    std::vector<uint32_t> ThreadFree;
  };
  ConcurrencyState &getConcurrency() const noexcept { return Concurrency; }

  // Root of the lexical instantiation tree (poisoning + host-entry checks).
  const ComponentInstance *getRoot() const noexcept {
    const ComponentInstance *R = this;
    while (R->Parent != nullptr) {
      R = R->Parent;
    }
    return R;
  }
  // True when the callee instance and the caller instance are the same or
  // one is a lexical ancestor of the other: such adapter calls trap
  // unconditionally (wasmtime fact/trampoline.rs reentrance rule).
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

  // A component index entry: the definition plus the lexical environment
  // the value closed over.
  struct CompEntry {
    const AST::Component::Component *Ast;
    const ComponentInstance *Env;
  };

  void addComponent(const AST::Component::Component &C) noexcept {
    // A component value closes over its lexical environment: outer aliases
    // inside it resolve against the defining instance, wherever it is
    // eventually instantiated.
    Comps.push_back({&C, this});
  }
  void addComponentEntry(const AST::Component::Component *C,
                         const ComponentInstance *Env) noexcept {
    Comps.push_back({C, Env});
  }
  const ComponentInstance *getComponentEnv(uint32_t Index) const noexcept {
    return Index < Comps.size() ? Comps[Index].Env : nullptr;
  }
  void exportComponent(std::string_view Name, uint32_t Idx) noexcept {
    if (Idx < Comps.size()) {
      ExpComps.insert_or_assign(std::string(Name), Comps[Idx]);
    }
  }
  const CompEntry *findComponentEntry(std::string_view Name) const noexcept {
    auto It = ExpComps.find(Name);
    return It != ExpComps.end() ? &It->second : nullptr;
  }
  const AST::Component::Component *getComponent(uint32_t Index) const noexcept {
    return Index < Comps.size() ? Comps[Index].Ast : nullptr;
  }

  // Index space: core function.
  void addCoreFunction(std::unique_ptr<FunctionInstance> &&Inst) noexcept {
    OwnedCoreFuncInsts.push_back(std::move(Inst));
    CoreFuncInsts.push_back(OwnedCoreFuncInsts.back().get());
  }
  void addCoreFunction(FunctionInstance *Inst) noexcept {
    CoreFuncInsts.push_back(Inst);
  }
  /// Add a host function to the core function index space. The function is
  /// owned by an auxiliary ModuleInstance (created here) whose only purpose
  /// is to register the host function's defined type so that downstream
  /// import matching (which walks ModInst::getTypeList()) succeeds. This is
  /// used by canon lower to expose its thunk to core wasm.
  void addCoreHostFunction(std::unique_ptr<Runtime::HostFunctionBase> &&Host,
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
  void exportCoreFunction(std::string_view Name, uint32_t Idx) noexcept {
    if (Idx < CoreFuncInsts.size()) {
      ExpCoreFuncInsts.insert_or_assign(std::string(Name), CoreFuncInsts[Idx]);
    }
  }
  FunctionInstance *findCoreFunction(std::string_view Name) const noexcept {
    return findExport(ExpCoreFuncInsts, Name);
  }

  // Index space: core table.
  void addCoreTable(TableInstance *Inst) noexcept {
    CoreTabInsts.push_back(Inst);
  }
  TableInstance *getCoreTable(uint32_t Index) const noexcept {
    return Index < CoreTabInsts.size() ? CoreTabInsts[Index] : nullptr;
  }
  void exportCoreTable(std::string_view Name, uint32_t Idx) noexcept {
    if (Idx < CoreTabInsts.size()) {
      ExpCoreTabInsts.insert_or_assign(std::string(Name), CoreTabInsts[Idx]);
    }
  }
  TableInstance *findCoreTable(std::string_view Name) const noexcept {
    return findExport(ExpCoreTabInsts, Name);
  }

  // Index space: core memory.
  void addCoreMemory(MemoryInstance *Inst) noexcept {
    CoreMemInsts.push_back(Inst);
  }
  MemoryInstance *getCoreMemory(uint32_t Index) const noexcept {
    return Index < CoreMemInsts.size() ? CoreMemInsts[Index] : nullptr;
  }
  void exportCoreMemory(std::string_view Name, uint32_t Idx) noexcept {
    if (Idx < CoreMemInsts.size()) {
      ExpCoreMemInsts.insert_or_assign(std::string(Name), CoreMemInsts[Idx]);
    }
  }
  MemoryInstance *findCoreMemory(std::string_view Name) const noexcept {
    return findExport(ExpCoreMemInsts, Name);
  }

  // Index space: core glocal.
  void addCoreGlobal(GlobalInstance *Inst) noexcept {
    CoreGlobInsts.push_back(Inst);
  }
  GlobalInstance *getCoreGlobal(uint32_t Index) const noexcept {
    return Index < CoreGlobInsts.size() ? CoreGlobInsts[Index] : nullptr;
  }
  void exportCoreGlobal(std::string_view Name, uint32_t Idx) noexcept {
    if (Idx < CoreGlobInsts.size()) {
      ExpCoreGlobInsts.insert_or_assign(std::string(Name), CoreGlobInsts[Idx]);
    }
  }
  GlobalInstance *findCoreGlobal(std::string_view Name) const noexcept {
    return findExport(ExpCoreGlobInsts, Name);
  }

  // Index space: core tag.
  void addCoreTag(TagInstance *Inst) noexcept { CoreTagInsts.push_back(Inst); }
  TagInstance *getCoreTag(uint32_t Index) const noexcept {
    return Index < CoreTagInsts.size() ? CoreTagInsts[Index] : nullptr;
  }
  void exportCoreTag(std::string_view Name, uint32_t Idx) noexcept {
    if (Idx < CoreTagInsts.size()) {
      ExpCoreTagInsts.insert_or_assign(std::string(Name), CoreTagInsts[Idx]);
    }
  }
  TagInstance *findCoreTag(std::string_view Name) const noexcept {
    return findExport(ExpCoreTagInsts, Name);
  }

  // Index space: core type.
  // TODO: deep copy the type
  void addCoreType(const AST::Component::CoreDefType &Ty) noexcept {
    CoreTypes.emplace_back(&Ty);
  }
  const AST::Component::CoreDefType *
  getCoreType(uint32_t Index) const noexcept {
    return Index < CoreTypes.size() ? CoreTypes[Index] : nullptr;
  }

  // Index space: core module instance.
  void addCoreModuleInstance(const ModuleInstance *Inst) noexcept {
    CoreModInsts.push_back(Inst);
  }
  void addCoreModuleInstance(std::unique_ptr<ModuleInstance> &&Inst) noexcept {
    OwnedCoreModInsts.push_back(std::move(Inst));
    CoreModInsts.push_back(OwnedCoreModInsts.back().get());
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

  // Index space: module. (declaration for instantiation phase)
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
  std::string CompName;
  const ComponentInstance *Parent = nullptr;
  mutable bool Entered = false;
  mutable ConcurrencyState Concurrency;
  std::map<std::string, const AST::Module *, std::less<>> ExpCoreMods;
  std::map<std::string, CompEntry, std::less<>> ExpComps;
  std::map<std::string, ComponentValVariant, std::less<>> ExpValues;
  std::vector<const ResourceTypeRT *> TypeResources;
  std::vector<std::unique_ptr<ResourceTypeRT>> OwnedResourceTypes;
  std::vector<std::unique_ptr<AST::Component::DefType>> OwnedDefTypes;
  mutable std::vector<TableSlot> Handles;
  mutable std::vector<uint32_t> FreeSlots;

  // value
  std::vector<ComponentValVariant> ValueList;

  // Index spaces.
  // The index spaces of AST should be cleaned after instantiation.
  std::vector<Component::FunctionInstance *> FuncInsts;
  // TODO: values
  std::vector<const AST::Component::DefType *> Types;
  std::vector<const ComponentInstance *> CompInsts;
  std::vector<CompEntry> Comps;
  std::vector<FunctionInstance *> CoreFuncInsts;
  std::vector<TableInstance *> CoreTabInsts;
  std::vector<MemoryInstance *> CoreMemInsts;
  std::vector<GlobalInstance *> CoreGlobInsts;
  std::vector<const AST::Component::CoreDefType *> CoreTypes;
  std::vector<TagInstance *> CoreTagInsts;
  std::vector<const ModuleInstance *> CoreModInsts;
  std::vector<const AST::Module *> CoreMods;

  // Storage of index spaces.
  std::vector<std::unique_ptr<Component::FunctionInstance>> OwnedFuncInsts;
  // std::vector<std::unique_ptr<AST::Component::DefType>> OwnedTypes;
  std::vector<std::unique_ptr<ComponentInstance>> OwnedCompInsts;
  std::vector<std::unique_ptr<FunctionInstance>> OwnedCoreFuncInsts;
  // std::vector<std::unique_ptr<AST::Component::CoreDefType>> OwnedCoreTypes;
  std::vector<std::unique_ptr<ModuleInstance>> OwnedCoreModInsts;
  // Holder modules for synthesized host functions (e.g., canon lower thunks).
  // Not visible in the core-module index space; only owns the host function
  // and its registered SubType for matchType lookups.
  std::vector<std::unique_ptr<ModuleInstance>> OwnedAuxModInsts;

  // Export alias.
  std::map<std::string, Component::FunctionInstance *, std::less<>>
      ExpFuncInsts;
  // TODO: ExpValue
  std::map<std::string,
           std::pair<const AST::Component::DefType *, const ResourceTypeRT *>,
           std::less<>>
      ExpTypes;
  std::map<std::string, const ComponentInstance *, std::less<>> ExpCompInsts;
  // TODO: ExpComps
  std::map<std::string, FunctionInstance *, std::less<>> ExpCoreFuncInsts;
  std::map<std::string, TableInstance *, std::less<>> ExpCoreTabInsts;
  std::map<std::string, MemoryInstance *, std::less<>> ExpCoreMemInsts;
  std::map<std::string, GlobalInstance *, std::less<>> ExpCoreGlobInsts;
  std::map<std::string, TagInstance *, std::less<>> ExpCoreTagInsts;
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
