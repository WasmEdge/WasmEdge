// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/runtime/instance/component/component.h -------------------===//
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
#include "common/component_variant.h"
#include "common/errcode.h"
#include "runtime/component/hostfunc.h"
#include "runtime/instance/component/function.h"
#include "runtime/instance/component/resource.h"
#include "runtime/instance/component/waitable.h"
#include "runtime/instance/module.h"
#include "runtime/instance/tag.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <typeindex>
#include <utility>
#include <variant>
#include <vector>

namespace WasmEdge {

namespace Executor {
class ComponentExecutor;
namespace Component {
class Instantiator;
} // namespace Component
} // namespace Executor

namespace Runtime {

namespace Component {
class Task;
struct ThreadContext;
} // namespace Component

namespace Instance {

/// The component instance: the runtime state of one instantiation, following
/// the linking isolation of the module and component type declarations. The
/// executor's Instantiator reads the AST and holds the index spaces only
/// instantiation needs; afterwards the tree owns every definition it can
/// still reach, one copy each at the root.
class ComponentInstance {
public:
  /// A component definition as the index space and the exports hold it: the
  /// definition plus the lexical environment the value closed over.
  struct ComponentDefinition {
    const AST::Component::Component *Ast;
    ComponentInstance *Env;
  };

  /// A type definition as the index space and the exports hold it: the
  /// definition, the runtime identity of a resource type, and the instance
  /// whose index space the nested type indices of the definition belong to.
  struct TypeDefinition {
    const AST::Component::DefType *Def = nullptr;
    const Component::ResourceTypeInstance *Resource = nullptr;
    const ComponentInstance *Owner = nullptr;
  };

  /// Constructor. The lexical parent of a nested instance is what its outer
  /// aliases resolve through during instantiation.
  ComponentInstance(std::string_view Name,
                    ComponentInstance *ParentInst = nullptr)
      : CompName(Name), Parent(ParentInst),
        Root(ParentInst != nullptr ? ParentInst->Root : this) {}
  /// The threads of this instance end here, before any member it holds goes
  /// away. The task manager installs the hook when it spawns the first one.
  ~ComponentInstance() noexcept {
    if (OnDestroy) {
      OnDestroy();
    }
  }

  /// Getter for the component name.
  std::string_view getComponentName() const noexcept { return CompName; }

  /// Getter for the lexical parent.
  const ComponentInstance *getParent() const noexcept { return Parent; }

  /// Root of the lexical instantiation tree (poisoning + host-entry checks).
  const ComponentInstance *getRoot() const noexcept { return Root; }

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

  /// \name The canonical `handles` table. Every handle kind shares one index
  /// space, which is what the specification defines. The table is call-time
  /// state, so it mutates through the const instance of a canonical context.
  /// @{
  /// A resource entry of the table.
  struct ResourceHandle {
    const Component::ResourceTypeInstance *RT = nullptr;
    uint64_t Rep = 0;
    bool Own = true;
    uint32_t Lends = 0;
    /// The task that received this borrow, for its borrow accounting. It is
    /// null for own and host-boundary handles.
    Runtime::Component::Task *BorrowScope = nullptr;
  };

  /// Resource entries.
  uint32_t
  addHandle(const Component::ResourceTypeInstance *RT, uint64_t Rep, bool Own,
            Runtime::Component::Task *BorrowScope = nullptr) const noexcept {
    return addSlot(HandleSlot{ResourceHandle{RT, Rep, Own, 0, BorrowScope}});
  }
  ResourceHandle *getHandle(uint32_t Idx) const noexcept {
    if (!isSlotLive(Idx)) {
      return nullptr;
    }
    return std::get_if<ResourceHandle>(&Handles[Idx]);
  }
  std::optional<ResourceHandle> removeHandle(uint32_t Idx) const noexcept {
    auto *Slot = getHandle(Idx);
    if (Slot == nullptr) {
      return std::nullopt;
    }
    ResourceHandle Out = *Slot;
    freeSlot(Idx);
    return Out;
  }

  /// Waitable entries (subtasks and stream or future transmit ends).
  uint32_t
  addWaitable(std::shared_ptr<Component::WaitableBase> W) const noexcept {
    return addSlot(HandleSlot{std::move(W)});
  }
  Component::WaitableBase *getWaitable(uint32_t Idx) const noexcept {
    if (!isSlotLive(Idx)) {
      return nullptr;
    }
    if (auto *P = std::get_if<std::shared_ptr<Component::WaitableBase>>(
            &Handles[Idx])) {
      return P->get();
    }
    return nullptr;
  }
  std::shared_ptr<Component::WaitableBase>
  removeWaitable(uint32_t Idx) const noexcept {
    if (auto *W = std::get_if<std::shared_ptr<Component::WaitableBase>>(
            isSlotLive(Idx) ? &Handles[Idx] : nullptr)) {
      auto Out = std::move(*W);
      freeSlot(Idx);
      return Out;
    }
    return nullptr;
  }

  /// Waitable-set entries.
  uint32_t addWaitableSet() const noexcept {
    return addSlot(HandleSlot{std::make_unique<Component::WaitableSet>()});
  }
  Component::WaitableSet *getWaitableSet(uint32_t Idx) const noexcept {
    if (!isSlotLive(Idx)) {
      return nullptr;
    }
    if (auto *P = std::get_if<std::unique_ptr<Component::WaitableSet>>(
            &Handles[Idx])) {
      return P->get();
    }
    return nullptr;
  }
  std::unique_ptr<Component::WaitableSet>
  removeWaitableSet(uint32_t Idx) const noexcept {
    if (auto *W = std::get_if<std::unique_ptr<Component::WaitableSet>>(
            isSlotLive(Idx) ? &Handles[Idx] : nullptr)) {
      auto Out = std::move(*W);
      freeSlot(Idx);
      return Out;
    }
    return nullptr;
  }

  /// Error-context entries.
  uint32_t addErrorContext(std::string Msg) const noexcept {
    return addSlot(HandleSlot{std::move(Msg)});
  }
  std::string *getErrorContext(uint32_t Idx) const noexcept {
    if (!isSlotLive(Idx)) {
      return nullptr;
    }
    return std::get_if<std::string>(&Handles[Idx]);
  }
  bool removeErrorContext(uint32_t Idx) const noexcept {
    if (getErrorContext(Idx) == nullptr) {
      return false;
    }
    freeSlot(Idx);
    return true;
  }
  /// @}

  /// \name The concurrency state of the instance, as the specification keeps
  /// it on the instance: the entry gate, the no-leave regions, the exclusive
  /// task, the thread table, and the poison latch of a trapped tree.
  /// @{
  /// True while the instance is executing or still instantiating.
  bool isEntered() const noexcept { return Entered; }

  /// Sets the entered flag for the scope and restores the previous value on
  /// exit, so no error path can leak it. Instantiation enters the instance
  /// under construction, and its start function leaves it again.
  class EnteredGuard {
  public:
    EnteredGuard(const ComponentInstance &Inst, bool V) noexcept
        : Status(Inst), Saved(Inst.Entered) {
      Status.Entered = V;
    }
    ~EnteredGuard() noexcept { Status.Entered = Saved; }
    EnteredGuard(const EnteredGuard &) = delete;
    EnteredGuard &operator=(const EnteredGuard &) = delete;

  private:
    const ComponentInstance &Status;
    bool Saved;
  };

  /// Getter and setter for the no-leave region flag.
  bool isLeaveAllowed() const noexcept { return MayLeave; }
  void setMayLeave(bool V) const noexcept { MayLeave = V; }

  /// Getter for the backpressure counter.
  int64_t getBackpressure() const noexcept { return Backpressure; }

  /// Raise the backpressure counter; false on overflow.
  [[nodiscard]] bool incBackpressure() const noexcept {
    Backpressure += 1;
    return Backpressure != (INT64_C(1) << 16);
  }

  /// Lower the backpressure counter; false on underflow.
  [[nodiscard]] bool decBackpressure() const noexcept {
    Backpressure -= 1;
    return Backpressure >= 0;
  }

  /// Getter and counters for the tasks blocked at the entry gate.
  uint32_t getNumWaitingToEnter() const noexcept { return NumWaitingToEnter; }
  void incWaitingToEnter() const noexcept { NumWaitingToEnter += 1; }
  void decWaitingToEnter() const noexcept { NumWaitingToEnter -= 1; }

  /// Getter and setter for the task holding the instance exclusively.
  Runtime::Component::Task *getExclusiveTask() const noexcept {
    return ExclusiveTask;
  }
  void setExclusiveTask(Runtime::Component::Task *T) const noexcept {
    ExclusiveTask = T;
  }

  /// A trapped instance tree rejects every further entry.
  bool isPoisoned() const noexcept { return Poisoned; }
  void setPoisoned() const noexcept { Poisoned = true; }

  /// The destroy hook, installed by the task manager. A thread belongs to the
  /// instance rather than to the call that spawned it, so the instance aborts
  /// and joins its threads when it goes away.
  bool hasDestroyHook() const noexcept { return static_cast<bool>(OnDestroy); }
  void setDestroyHook(std::function<void()> Hook) const noexcept {
    OnDestroy = std::move(Hook);
  }
  void clearDestroyHook() const noexcept { OnDestroy = nullptr; }

  /// The per-instance thread table: every thread activation registers here,
  /// and thread.index reads it.
  uint32_t addThread(Runtime::Component::ThreadContext *T) const noexcept {
    if (Threads.empty()) {
      Threads.push_back(nullptr); // slot 0 stays dead
    }
    if (!ThreadFree.empty()) {
      const uint32_t Idx = ThreadFree.back();
      ThreadFree.pop_back();
      Threads[Idx] = T;
      return Idx;
    }
    Threads.push_back(T);
    return static_cast<uint32_t>(Threads.size() - 1);
  }
  void removeThread(uint32_t Idx) const noexcept {
    if (Idx != 0 && Idx < Threads.size()) {
      Threads[Idx] = nullptr;
      ThreadFree.push_back(Idx);
    }
  }
  Runtime::Component::ThreadContext *getThread(uint32_t Idx) const noexcept {
    return Idx != 0 && Idx < Threads.size() ? Threads[Idx] : nullptr;
  }
  /// @}

  /// \name Exports: value and component function.
  /// @{
  const ComponentValVariant *findValue(std::string_view Name) const noexcept {
    auto Iter = ExpValues.find(Name);
    return Iter != ExpValues.end() ? &Iter->second : nullptr;
  }
  /// Host component function registered under an export name. The function
  /// declares its type into the type index space of this instance.
  void addHostFunc(
      std::string_view Name,
      std::unique_ptr<Runtime::Component::HostFunctionBase> &&Host) noexcept {
    Host->declare(getTypeMinter());
    exportFunction(Name,
                   addFunction(std::make_unique<ComponentFunctionInstance>(
                       std::move(Host), this)));
  }
  /// The minter of host-declared types into the type index space of this
  /// instance.
  Runtime::Component::TypeMinter getTypeMinter() noexcept {
    return Runtime::Component::TypeMinter(
        [this](AST::Component::DefType &&Ty) { return addType(std::move(Ty)); },
        [this](std::type_index Tag) { return getResourceTypeIndex(Tag); });
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
  /// @}

  /// \name Index space: type.
  /// @{
  /// A host-built type; the tree owns the definition.
  uint32_t addType(AST::Component::DefType &&Ty) noexcept {
    Types.push_back(
        {Root->OwnedDefTypes.add(
             std::make_unique<AST::Component::DefType>(std::move(Ty))),
         nullptr, this});
    return static_cast<uint32_t>(Types.size() - 1);
  }
  /// Host-defined resource type with a host destructor.
  uint32_t addHostResourceType(std::function<void(uint64_t)> Dtor) noexcept {
    OwnedResourceTypes.push_back(
        std::make_unique<Component::ResourceTypeInstance>(
            this, nullptr, std::move(Dtor), false));
    Types.push_back({nullptr, OwnedResourceTypes.back().get(), this});
    return static_cast<uint32_t>(Types.size() - 1);
  }
  /// Host-defined resource type registered under the C++ tag R, which
  /// `Own<R>` and `Borrow<R>` in host function types resolve to.
  template <typename R>
  uint32_t addHostResourceType(std::function<void(uint64_t)> Dtor) noexcept {
    const uint32_t Idx = addHostResourceType(std::move(Dtor));
    HostResourceTags.insert_or_assign(std::type_index(typeid(R)), Idx);
    return Idx;
  }
  /// A resource type another host instance defined, shared under the tag R.
  template <typename R>
  uint32_t addSharedResourceType(
      const Component::ResourceTypeInstance *Resource) noexcept {
    addTypeDefinition({nullptr, Resource, nullptr});
    const uint32_t Idx = static_cast<uint32_t>(Types.size() - 1);
    HostResourceTags.insert_or_assign(std::type_index(typeid(R)), Idx);
    return Idx;
  }
  /// The type index registered under a host resource tag.
  uint32_t getResourceTypeIndex(std::type_index Tag) const noexcept {
    auto Iter = HostResourceTags.find(Tag);
    assuming(Iter != HostResourceTags.end());
    return Iter->second;
  }
  Expect<const TypeDefinition *>
  getTypeDefinition(uint32_t Idx) const noexcept {
    if (unlikely(Idx >= Types.size())) {
      return Unexpect(ErrCode::Value::WrongInstanceIndex);
    }
    return &Types[Idx];
  }
  /// The definition behind a type index; null for a host resource type.
  Expect<const AST::Component::DefType *> getType(uint32_t Idx) const noexcept {
    EXPECTED_TRY(const auto *TypeDef, getTypeDefinition(Idx));
    return TypeDef->Def;
  }
  /// The instance whose index space the definition at Idx refers to.
  Expect<const ComponentInstance *> getTypeOwner(uint32_t Idx) const noexcept {
    EXPECTED_TRY(const auto *TypeDef, getTypeDefinition(Idx));
    return TypeDef->Owner;
  }
  /// The resource identity behind a type index; null for a value type.
  Expect<const Component::ResourceTypeInstance *>
  getTypeResource(uint32_t Idx) const noexcept {
    EXPECTED_TRY(const auto *TypeDef, getTypeDefinition(Idx));
    return TypeDef->Resource;
  }
  void exportType(std::string_view Name, uint32_t Idx) noexcept {
    assuming(Idx < Types.size());
    ExpTypes.insert_or_assign(std::string(Name), Types[Idx]);
  }
  const TypeDefinition *
  findTypeDefinition(std::string_view Name) const noexcept {
    auto Iter = ExpTypes.find(Name);
    return Iter != ExpTypes.end() ? &Iter->second : nullptr;
  }
  const AST::Component::DefType *
  findType(std::string_view Name) const noexcept {
    const auto *TypeDef = findTypeDefinition(Name);
    return TypeDef != nullptr ? TypeDef->Def : nullptr;
  }
  const Component::ResourceTypeInstance *
  findTypeResource(std::string_view Name) const noexcept {
    const auto *TypeDef = findTypeDefinition(Name);
    return TypeDef != nullptr ? TypeDef->Resource : nullptr;
  }
  /// @}

  /// \name Exports: component instance.
  /// @{
  /// A nested instance this instance owns.
  const ComponentInstance *
  addComponentInstance(std::unique_ptr<ComponentInstance> &&Inst) noexcept {
    OwnedCompInsts.push_back(std::move(Inst));
    return OwnedCompInsts.back().get();
  }
  void exportComponentInstance(std::string_view Name,
                               const ComponentInstance *Inst) noexcept {
    ExpCompInsts.insert_or_assign(std::string(Name), Inst);
  }
  const ComponentInstance *
  findComponentInstance(std::string_view Name) const noexcept {
    return findExport(ExpCompInsts, Name);
  }
  template <typename CallbackT>
  auto getComponentInstanceExports(CallbackT &&CallBack) const noexcept {
    return std::forward<CallbackT>(CallBack)(ExpCompInsts);
  }
  /// @}

  /// \name Index space: component.
  /// @{
  Expect<const ComponentDefinition *>
  getComponentDefinition(uint32_t Idx) const noexcept {
    if (unlikely(Idx >= Comps.size())) {
      return Unexpect(ErrCode::Value::WrongInstanceIndex);
    }
    return &Comps[Idx];
  }
  const ComponentDefinition *
  findComponentDefinition(std::string_view Name) const noexcept {
    auto Iter = ExpComps.find(Name);
    return Iter != ExpComps.end() ? &Iter->second : nullptr;
  }
  /// @}

  /// \name Index space: core type.
  /// @{
  Expect<const AST::Component::CoreDefType *>
  getCoreType(uint32_t Idx) const noexcept {
    if (unlikely(Idx >= CoreTypes.size())) {
      return Unexpect(ErrCode::Value::WrongInstanceIndex);
    }
    return CoreTypes[Idx];
  }
  /// @}

  /// \name Exports: core module instance; index space: core module.
  /// @{
  void exportCoreModuleInstance(std::string_view Name,
                                const ModuleInstance *Inst) noexcept {
    ExpCoreModInsts.insert_or_assign(std::string(Name), Inst);
  }
  const ModuleInstance *
  findCoreModuleInstance(std::string_view Name) const noexcept {
    return findExport(ExpCoreModInsts, Name);
  }
  /// A core module definition; a host instance exports the ones it offers.
  void addModule(const AST::Module &Mod) noexcept { CoreMods.push_back(&Mod); }
  Expect<const AST::Module *> getModule(uint32_t Idx) const noexcept {
    if (unlikely(Idx >= CoreMods.size())) {
      return Unexpect(ErrCode::Value::WrongInstanceIndex);
    }
    return CoreMods[Idx];
  }
  /// An exported definition can be instantiated after this instantiation
  /// ends, so the tree owns it first.
  void exportCoreModule(std::string_view Name, uint32_t Idx) noexcept {
    assuming(Idx < CoreMods.size());
    ownCoreModule(Idx);
    ExpCoreMods.insert_or_assign(std::string(Name), CoreMods[Idx]);
  }
  const AST::Module *findCoreModule(std::string_view Name) const noexcept {
    return findExport(ExpCoreMods, Name);
  }
  /// @}

protected:
  friend class Executor::ComponentExecutor;
  friend class Executor::Component::Instantiator;

  /// \name Mutators for instantiation. An add* takes ownership or a
  /// definition; an export* names what a later instantiation may reach.
  /// @{
  void exportValue(std::string_view Name, ComponentValVariant V) noexcept {
    ExpValues.insert_or_assign(std::string(Name), std::move(V));
  }

  ComponentFunctionInstance *
  addFunction(std::unique_ptr<ComponentFunctionInstance> &&Inst) noexcept {
    OwnedFuncInsts.push_back(std::move(Inst));
    return OwnedFuncInsts.back().get();
  }
  void exportFunction(std::string_view Name,
                      ComponentFunctionInstance *Func) noexcept {
    ExpFuncInsts.insert_or_assign(std::string(Name), Func);
  }

  /// A locally-defined type: the tree owns one copy of the definition.
  void addType(const AST::Component::DefType &Ty) noexcept {
    Types.push_back({Root->OwnedDefTypes.own(&Ty), nullptr, this});
  }
  /// A locally-defined resource type: mints the runtime identity.
  const Component::ResourceTypeInstance *
  addResourceType(const AST::Component::DefType &Ty,
                  FunctionInstance *Dtor) noexcept {
    OwnedResourceTypes.push_back(
        std::make_unique<Component::ResourceTypeInstance>(
            this, Dtor, nullptr,
            Ty.isResourceType() && Ty.getResourceType().isAddrI64()));
    Types.push_back(
        {Root->OwnedDefTypes.own(&Ty), OwnedResourceTypes.back().get(), this});
    return OwnedResourceTypes.back().get();
  }
  /// An imported or aliased type sharing an existing definition and
  /// identity; an absent owner means this instance.
  void addTypeDefinition(const TypeDefinition &Def) noexcept {
    Types.push_back(
        {Def.Def, Def.Resource, Def.Owner != nullptr ? Def.Owner : this});
  }

  /// A component value closes over its lexical environment, so its outer
  /// aliases resolve against the defining instance.
  void addComponent(const AST::Component::Component &Comp) noexcept {
    Comps.push_back({&Comp, this});
  }
  void addComponentDefinition(const ComponentDefinition &Def) noexcept {
    Comps.push_back(Def);
  }
  /// An exported definition can be instantiated after this instantiation
  /// ends, so the tree owns it and what its outer aliases reach first.
  void exportComponent(std::string_view Name, uint32_t Idx) noexcept {
    assuming(Idx < Comps.size());
    std::vector<const AST::Component::Component *> Sealed;
    ownComponentDefinition(Comps[Idx], Sealed);
    ExpComps.insert_or_assign(std::string(Name), Comps[Idx]);
  }

  /// A synthesized host function in the core function index space. It has
  /// no module of its own: the core module importing it registers its type.
  FunctionInstance *
  addCoreHostFunction(std::unique_ptr<HostFunctionBase> &&Host) {
    OwnedCoreFuncInsts.push_back(
        std::make_unique<FunctionInstance>(std::move(Host)));
    return OwnedCoreFuncInsts.back().get();
  }
  void addCoreType(const AST::Component::CoreDefType &Ty) noexcept {
    CoreTypes.push_back(Root->OwnedCoreDefTypes.own(&Ty));
  }
  const ModuleInstance *
  addCoreModuleInstance(std::unique_ptr<ModuleInstance> &&Inst) noexcept {
    OwnedCoreModInsts.push_back(std::move(Inst));
    return OwnedCoreModInsts.back().get();
  }

  /// The root instantiation is over and Source may go away: drop every entry
  /// of the tree still pointing into it, and the source-keyed copy indexes.
  void releaseSource(const AST::Component::Component &Source) noexcept {
    std::vector<const AST::Component::Component *> SourceComps;
    std::vector<const AST::Module *> SourceMods;
    collectSourceDefinitions(Source, SourceComps, SourceMods);
    std::sort(SourceComps.begin(), SourceComps.end());
    std::sort(SourceMods.begin(), SourceMods.end());
    dropSourceEntries(SourceComps, SourceMods);
    OwnedDefTypes.releaseSources();
    OwnedCoreDefTypes.releaseSources();
    OwnedComponents.releaseSources();
    OwnedCoreModules.releaseSources();
  }
  /// @}

private:
  /// The tree's copies of one kind of definition, kept at the root.
  template <typename T> class OwnedDefinitions {
  public:
    /// The tree's copy of Src: one of the copies comes back as it is, a
    /// source node is copied once while the source is alive.
    const T *own(const T *Src) noexcept {
      if (std::any_of(Copies.begin(), Copies.end(), [Src](const auto &Owned) {
            return Owned.get() == Src;
          })) {
        return Src;
      }
      auto &Copy = Sources[Src];
      if (Copy == nullptr) {
        Copies.push_back(std::make_unique<T>(*Src));
        Copy = Copies.back().get();
      }
      return Copy;
    }
    /// Adopt a definition the host built.
    const T *add(std::unique_ptr<T> &&Def) noexcept {
      Copies.push_back(std::move(Def));
      return Copies.back().get();
    }
    /// The source is gone: its nodes identify nothing any more.
    void releaseSources() noexcept { Sources.clear(); }

  private:
    std::vector<std::unique_ptr<T>> Copies;
    std::map<const T *, const T *> Sources;
  };

  /// Own the definition of Def and, in the instances it closes over, the
  /// definitions its outer aliases reach; nested components recurse.
  void ownComponentDefinition(
      ComponentDefinition &Def,
      std::vector<const AST::Component::Component *> &Sealed) noexcept {
    Def.Ast = Root->OwnedComponents.own(Def.Ast);
    if (std::find(Sealed.begin(), Sealed.end(), Def.Ast) != Sealed.end()) {
      return;
    }
    Sealed.push_back(Def.Ast);
    ownOuterDefinitions(*Def.Ast, 0, Def.Env, Sealed);
  }
  void ownOuterDefinitions(
      const AST::Component::Component &Comp, uint32_t Depth,
      ComponentInstance *Env,
      std::vector<const AST::Component::Component *> &Sealed) noexcept {
    for (const auto &Sec : Comp.getSections()) {
      if (const auto *Nested =
              std::get_if<AST::Component::ComponentSection>(&Sec)) {
        ownOuterDefinitions(Nested->getContent(), Depth + 1, Env, Sealed);
        continue;
      }
      const auto *Aliases = std::get_if<AST::Component::AliasSection>(&Sec);
      if (Aliases == nullptr) {
        continue;
      }
      for (const auto &Alias : Aliases->getContent()) {
        // Every instance owns its types: only a module or component that a
        // later instantiation reads through the environment needs owning.
        if (Alias.getTargetType() != AST::Component::Alias::TargetType::Outer ||
            Alias.getOuter().first <= Depth) {
          continue;
        }
        ComponentInstance *Target = Env;
        for (uint32_t I = Depth + 1;
             I < Alias.getOuter().first && Target != nullptr; ++I) {
          Target = Target->Parent;
        }
        if (Target == nullptr) {
          continue;
        }
        const auto &Sort = Alias.getSort();
        const uint32_t Idx = Alias.getOuter().second;
        if (Sort.isCore()) {
          if (Sort.getCoreSortType() ==
              AST::Component::Sort::CoreSortType::Module) {
            Target->ownCoreModule(Idx);
          }
        } else if (Sort.getSortType() ==
                   AST::Component::Sort::SortType::Component) {
          assuming(Idx < Target->Comps.size());
          Target->ownComponentDefinition(Target->Comps[Idx], Sealed);
        }
      }
    }
  }
  void ownCoreModule(uint32_t Idx) noexcept {
    assuming(Idx < CoreMods.size());
    CoreMods[Idx] = Root->OwnedCoreModules.own(CoreMods[Idx]);
  }
  /// The component and core module nodes of a source tree, at every depth.
  void collectSourceDefinitions(
      const AST::Component::Component &Comp,
      std::vector<const AST::Component::Component *> &SourceComps,
      std::vector<const AST::Module *> &SourceMods) const noexcept {
    for (const auto &Sec : Comp.getSections()) {
      if (const auto *Nested =
              std::get_if<AST::Component::ComponentSection>(&Sec)) {
        SourceComps.push_back(&Nested->getContent());
        collectSourceDefinitions(Nested->getContent(), SourceComps, SourceMods);
      } else if (const auto *Mod =
                     std::get_if<AST::Component::CoreModuleSection>(&Sec)) {
        SourceMods.push_back(&Mod->getContent());
      }
    }
  }
  /// Null the entries of this instance and its nested ones that point into
  /// the sorted source node lists.
  void dropSourceEntries(
      const std::vector<const AST::Component::Component *> &SourceComps,
      const std::vector<const AST::Module *> &SourceMods) noexcept {
    for (auto &Def : Comps) {
      if (std::binary_search(SourceComps.begin(), SourceComps.end(), Def.Ast)) {
        Def.Ast = nullptr;
      }
    }
    for (auto &Mod : CoreMods) {
      if (std::binary_search(SourceMods.begin(), SourceMods.end(), Mod)) {
        Mod = nullptr;
      }
    }
    for (auto &Nested : OwnedCompInsts) {
      Nested->dropSourceEntries(SourceComps, SourceMods);
    }
  }

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

  /// One slot of the unified `handles` table, where every handle kind shares
  /// an index space.
  using HandleSlot =
      std::variant<std::monostate, ResourceHandle,
                   std::shared_ptr<Component::WaitableBase>,
                   std::unique_ptr<Component::WaitableSet>, std::string>;

  /// Index 0 stays dead, and a freed slot returns through the LIFO list.
  uint32_t addSlot(HandleSlot &&Slot) const noexcept {
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
  bool isSlotLive(uint32_t Idx) const noexcept {
    return Idx != 0 && Idx < Handles.size() &&
           !std::holds_alternative<std::monostate>(Handles[Idx]);
  }
  void freeSlot(uint32_t Idx) const noexcept {
    Handles[Idx] = std::monostate{};
    FreeSlots.push_back(Idx);
  }

  /// \name Data of component instance.
  /// @{
  const std::string CompName;
  ComponentInstance *const Parent;
  /// The root of the lexical instantiation tree, which owns the definitions
  /// of the whole tree.
  ComponentInstance *const Root;

  /// The call-time state, mutable behind the const instance pointers of the
  /// canonical contexts: the `handles` table and the concurrency state.
  mutable std::vector<HandleSlot> Handles;
  mutable std::vector<uint32_t> FreeSlots;
  mutable std::function<void()> OnDestroy;
  mutable bool MayLeave = true;
  mutable int64_t Backpressure = 0;
  mutable uint32_t NumWaitingToEnter = 0;
  mutable Runtime::Component::Task *ExclusiveTask = nullptr;
  mutable bool Poisoned = false;
  mutable bool Entered = false;
  mutable std::vector<Runtime::Component::ThreadContext *> Threads;
  mutable std::vector<uint32_t> ThreadFree;

  /// Owned definitions, filled at the root: every type, and the components
  /// and core modules an export or an exported component's outer alias can
  /// still reach.
  OwnedDefinitions<AST::Component::DefType> OwnedDefTypes;
  OwnedDefinitions<AST::Component::CoreDefType> OwnedCoreDefTypes;
  OwnedDefinitions<AST::Component::Component> OwnedComponents;
  OwnedDefinitions<AST::Module> OwnedCoreModules;

  /// The index spaces a later call or instantiation can still reach.
  std::vector<TypeDefinition> Types;
  /// Host resource types by the C++ tag they were registered under.
  std::map<std::type_index, uint32_t> HostResourceTags;
  std::vector<ComponentDefinition> Comps;
  std::vector<const AST::Component::CoreDefType *> CoreTypes;
  std::vector<const AST::Module *> CoreMods;

  /// Owned runtime objects.
  std::vector<std::unique_ptr<Component::ResourceTypeInstance>>
      OwnedResourceTypes;
  std::vector<std::unique_ptr<ComponentFunctionInstance>> OwnedFuncInsts;
  std::vector<std::unique_ptr<ComponentInstance>> OwnedCompInsts;
  std::vector<std::unique_ptr<ModuleInstance>> OwnedCoreModInsts;
  /// The synthesized host functions of the canon section.
  std::vector<std::unique_ptr<FunctionInstance>> OwnedCoreFuncInsts;

  /// Exported name maps.
  std::map<std::string, ComponentValVariant, std::less<>> ExpValues;
  std::map<std::string, ComponentFunctionInstance *, std::less<>> ExpFuncInsts;
  std::map<std::string, TypeDefinition, std::less<>> ExpTypes;
  std::map<std::string, const ComponentInstance *, std::less<>> ExpCompInsts;
  std::map<std::string, ComponentDefinition, std::less<>> ExpComps;
  std::map<std::string, const ModuleInstance *, std::less<>> ExpCoreModInsts;
  std::map<std::string, const AST::Module *, std::less<>> ExpCoreMods;
  /// @}
};

inline bool Component::TransmitBuffer::isByteElem() const noexcept {
  if (!Elem.has_value()) {
    return false;
  }
  if (Elem->isPrimValType()) {
    return Elem->getPrimValType() == PrimValType::U8;
  }
  if (ElemInst == nullptr) {
    return false;
  }
  const auto Def = ElemInst->getType(Elem->getTypeIndex());
  return Def && *Def != nullptr && (*Def)->isDefValType() &&
         (*Def)->getDefValType().isPrimValType() &&
         (*Def)->getDefValType().getPrimValType() == PrimValType::U8;
}

inline void Component::Subtask::deliverResolve() noexcept {
  if (Delivered) {
    return;
  }
  for (const auto &[Inst, Idx] : Lenders) {
    if (auto *Slot = Inst->getHandle(Idx); Slot != nullptr && Slot->Lends > 0) {
      Slot->Lends -= 1;
    }
  }
  Lenders.clear();
  Delivered = true;
}

inline void Component::Subtask::noteProgress() noexcept {
  if (!TableIdx.has_value()) {
    return;
  }
  const uint32_t Idx = *TableIdx;
  setPendingEvent([this, Idx]() -> Component::AsyncEvent {
    if (isResolved()) {
      deliverResolve();
    }
    return {Component::AsyncEventCode::Subtask, Idx,
            static_cast<uint32_t>(Status)};
  });
}

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
