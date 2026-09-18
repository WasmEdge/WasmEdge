// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/runtime/instance/component/component.h - Instance --------===//
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
#include "common/span.h"
#include "runtime/component/hostfunc.h"
#include "runtime/component/task.h"
#include "runtime/component/thread.h"
#include "runtime/instance/component/function.h"
#include "runtime/instance/component/resource.h"
#include "runtime/instance/component/stream.h"
#include "runtime/instance/module.h"
#include "runtime/instance/reflifetime.h"

#include <algorithm>
#include <cstdint>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <typeindex>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

namespace WasmEdge {

namespace Executor {
class ComponentExecutor;
} // namespace Executor

namespace Runtime {
namespace Component {
class Task;
class StoreManager;
} // namespace Component

namespace Instance {

class ComponentInstance;

namespace Component {

/// A component definition plus the lexical environment it closed over.
struct ComponentDefinition {
  const AST::Component::Component *Comp = nullptr;
  ComponentInstance *Env = nullptr;
};

/// A type definition, its resource identity, and the instance owning its
/// nested type indices.
struct ComponentTypeDefinition {
  const AST::Component::DefType *Def = nullptr;
  const ResourceTypeInstance *Resource = nullptr;
  const ComponentInstance *Owner = nullptr;
};

/// A resource entry of the `handles` table, kept at a stable address.
struct ResourceHandle {
  const ResourceTypeInstance *ResType = nullptr;
  uint64_t Rep = 0;
  bool Own = true;
  uint32_t NumLends = 0;
  /// The task that received this borrow; null otherwise.
  Runtime::Component::Task *BorrowScope = nullptr;
};

/// A stream or future entry of the `handles` table.
struct StreamHandle {
  StreamInstance *Stream = nullptr;
  StreamInstance::Role Role = StreamInstance::Role::Reader;
};

} // namespace Component

/// The runtime state of one component instantiation.
class ComponentInstance {
public:
  /// One slot of the `handles` table; the monostate marks a free slot.
  using HandleSlot =
      std::variant<std::monostate, Component::ResourceHandle,
                   Runtime::Component::Task *, std::vector<uint32_t>,
                   Component::StreamHandle, std::string>;

  /// Constructor. A nested instance resolves outer aliases through its parent.
  ComponentInstance(std::string_view Name,
                    ComponentInstance *ParentInst = nullptr)
      : CompName(Name), Parent(ParentInst),
        Root(ParentInst != nullptr ? ParentInst->Root : this) {}
  /// A root leaves the linker and tears its store down before its members go;
  /// an instance that goes alone takes the tasks of its tree along.
  virtual ~ComponentInstance() noexcept {
    unlinkAllStores();
    assuming(!Life.hasDependents());
    if (Root == this) {
      Terminating = true;
      abortTasks(/*IsPoisoning=*/false);
      // A thread this store does not own unwinds as well, and every stack
      // unwinds before a stream end goes.
      for (auto *Thread : Threads) {
        if (Thread != nullptr && !Thread->isEnded() &&
            Thread->getInnermost() == Thread && !Thread->isRunning()) {
          Thread->onResume(Runtime::Component::Thread::WakeReason::Aborted);
        }
        if (Thread != nullptr) {
          Thread->setIndex(0);
        }
      }
      Threads.clear();
      releaseHandles();
      drainEndedTasks();
      releaseStreams();
    } else if (!Root->Terminating) {
      Root->abortTasks(/*IsPoisoning=*/true);
      releaseHandles();
      for (auto &S : Root->OwnedStreamInsts) {
        if (S->getElemTypeInst() == this) {
          S->onCreatorGone();
        }
      }
      if (Root->Entry == EntryKind::None) {
        Root->drainEndedTasks();
        Root->drainClosedStreams();
      }
    }
    releaseProviders();
  }
  ComponentInstance(const ComponentInstance &) = delete;
  ComponentInstance &operator=(const ComponentInstance &) = delete;

  /// Give up the ownership; the instance goes once no importer depends on it.
  void terminate() noexcept {
    unlinkAllStores();
    if (Life.releaseOwner()) {
      delete this;
    }
  }

  /// Keep Provider alive while this instance imports from it.
  void addDependency(ComponentInstance &Provider) {
    if (Providers.insert(&Provider).second) {
      Provider.Life.addDependent();
    }
    // A provider linked during an entry runs under that entry.
    if (auto *EntryInst = Root->EntryRoot; EntryInst != nullptr) {
      Provider.Root->EntryRoot = EntryInst;
      if (std::find(EntryInst->EntryStores.begin(),
                    EntryInst->EntryStores.end(),
                    Provider.Root) == EntryInst->EntryStores.end()) {
        EntryInst->EntryStores.push_back(Provider.Root);
      }
    }
  }

  /// Getter for the component name.
  std::string_view getComponentName() const noexcept { return CompName; }

  /// Join the instance tree, and so the store, of ParentInst. Only before the
  /// instance runs anything; the owner of the instance does not change.
  void setParent(ComponentInstance &ParentInst) noexcept {
    assuming(Root == this && OwnedTasks.empty());
    Parent = &ParentInst;
    Root = ParentInst.Root;
  }

  /// Getter for the lexical parent.
  ComponentInstance *getParent() noexcept { return Parent; }
  const ComponentInstance *getParent() const noexcept { return Parent; }

  /// Root of the lexical instantiation tree.
  ComponentInstance *getRoot() const noexcept { return Root; }

  /// \name Host registration.
  /// @{
  /// Register a host component function under an export name.
  void addHostFunc(
      std::string_view Name,
      std::unique_ptr<Runtime::Component::HostFunctionBase> &&Host) noexcept {
    Host->declare(getTypeMinter());
    exportFunction(Name,
                   addFunction(std::make_unique<ComponentFunctionInstance>(
                       std::move(Host), this)));
  }
  /// The minter of host-declared types into the type index space.
  Runtime::Component::TypeMinter getTypeMinter() noexcept {
    return Runtime::Component::TypeMinter(
        [this](AST::Component::DefType &&Ty) { return addType(std::move(Ty)); },
        [this](std::type_index Tag) {
          auto Iter = HostResourceTags.find(Tag);
          assuming(Iter != HostResourceTags.end());
          return Iter->second;
        });
  }
  /// A host-built type; the instance owns the definition.
  uint32_t addType(AST::Component::DefType &&Ty) noexcept {
    OwnedTypes.push_back(
        std::make_unique<AST::Component::DefType>(std::move(Ty)));
    Types.push_back({OwnedTypes.back().get(), nullptr, this});
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
  /// Host-defined resource type registered under the C++ tag R.
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
  void exportType(std::string_view Name, uint32_t Idx) noexcept {
    assuming(Idx < Types.size());
    ExpTypes.insert_or_assign(std::string(Name), Types[Idx]);
  }
  /// A nested instance this instance owns.
  const ComponentInstance *
  addComponentInstance(std::unique_ptr<ComponentInstance> &&Inst) noexcept {
    OwnedCompInsts.push_back(std::move(Inst));
    CompInsts.push_back(OwnedCompInsts.back().get());
    return OwnedCompInsts.back().get();
  }
  void exportComponentInstance(std::string_view Name,
                               const ComponentInstance *Inst) noexcept {
    ExpCompInsts.insert_or_assign(std::string(Name), Inst);
  }
  /// A core module definition.
  void addModule(const AST::Module &Mod) noexcept { CoreMods.push_back(&Mod); }
  void exportCoreModule(std::string_view Name, uint32_t Idx) noexcept {
    assuming(Idx < CoreMods.size());
    ExpCoreMods.insert_or_assign(std::string(Name), CoreMods[Idx]);
  }
  /// @}

  /// \name Exports.
  /// @{
  const ComponentValVariant *findValue(std::string_view Name) const noexcept {
    auto Iter = ExpValues.find(Name);
    return Iter != ExpValues.end() ? &Iter->second : nullptr;
  }
  ComponentFunctionInstance *
  findFunction(std::string_view Name) const noexcept {
    return findExport(ExpFuncInsts, Name);
  }
  /// Find an exported function by name or by `interface#func`.
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
  const Component::ComponentTypeDefinition *
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
  const ComponentInstance *
  findComponentInstance(std::string_view Name) const noexcept {
    return findExport(ExpCompInsts, Name);
  }
  template <typename CallbackT>
  auto getComponentInstanceExports(CallbackT &&CallBack) const noexcept {
    return std::forward<CallbackT>(CallBack)(ExpCompInsts);
  }
  const Component::ComponentDefinition *
  findComponentDefinition(std::string_view Name) const noexcept {
    auto Iter = ExpComps.find(Name);
    return Iter != ExpComps.end() ? &Iter->second : nullptr;
  }
  const AST::Module *findCoreModule(std::string_view Name) const noexcept {
    return findExport(ExpCoreMods, Name);
  }
  /// @}

  /// \name Index spaces.
  /// @{
  Expect<const Component::ComponentTypeDefinition *>
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
  /// The resource identity behind a type index; null for a value type.
  Expect<const Component::ResourceTypeInstance *>
  getTypeResource(uint32_t Idx) const noexcept {
    EXPECTED_TRY(const auto *TypeDef, getTypeDefinition(Idx));
    return TypeDef->Resource;
  }
  Expect<const ComponentInstance *>
  getComponentInstance(uint32_t Idx) const noexcept {
    if (unlikely(Idx >= CompInsts.size())) {
      return Unexpect(ErrCode::Value::WrongInstanceIndex);
    }
    return CompInsts[Idx];
  }
  Expect<const Component::ComponentDefinition *>
  getComponentDefinition(uint32_t Idx) const noexcept {
    if (unlikely(Idx >= Comps.size())) {
      return Unexpect(ErrCode::Value::WrongInstanceIndex);
    }
    return &Comps[Idx];
  }
  Expect<ComponentFunctionInstance *> getFunction(uint32_t Idx) const noexcept {
    if (unlikely(Idx >= FuncInsts.size())) {
      return Unexpect(ErrCode::Value::WrongInstanceIndex);
    }
    return FuncInsts[Idx];
  }
  Expect<const ComponentValVariant *> getValue(uint32_t Idx) const noexcept {
    if (unlikely(Idx >= Values.size())) {
      return Unexpect(ErrCode::Value::WrongInstanceIndex);
    }
    return &Values[Idx];
  }
  Expect<const AST::Component::CoreDefType *>
  getCoreType(uint32_t Idx) const noexcept {
    if (unlikely(Idx >= CoreTypes.size())) {
      return Unexpect(ErrCode::Value::WrongInstanceIndex);
    }
    return CoreTypes[Idx];
  }
  Expect<const AST::Module *> getModule(uint32_t Idx) const noexcept {
    if (unlikely(Idx >= CoreMods.size())) {
      return Unexpect(ErrCode::Value::WrongInstanceIndex);
    }
    return CoreMods[Idx];
  }
  Expect<const ModuleInstance *>
  getCoreModuleInstance(uint32_t Idx) const noexcept {
    if (unlikely(Idx >= CoreModInsts.size())) {
      return Unexpect(ErrCode::Value::WrongInstanceIndex);
    }
    return CoreModInsts[Idx];
  }
  Expect<FunctionInstance *> getCoreFunction(uint32_t Idx) const noexcept {
    if (unlikely(Idx >= CoreFuncInsts.size())) {
      return Unexpect(ErrCode::Value::WrongInstanceIndex);
    }
    return CoreFuncInsts[Idx];
  }
  Expect<TableInstance *> getCoreTable(uint32_t Idx) const noexcept {
    if (unlikely(Idx >= CoreTabInsts.size())) {
      return Unexpect(ErrCode::Value::WrongInstanceIndex);
    }
    return CoreTabInsts[Idx];
  }
  Expect<MemoryInstance *> getCoreMemory(uint32_t Idx) const noexcept {
    if (unlikely(Idx >= CoreMemInsts.size())) {
      return Unexpect(ErrCode::Value::WrongInstanceIndex);
    }
    return CoreMemInsts[Idx];
  }
  Expect<GlobalInstance *> getCoreGlobal(uint32_t Idx) const noexcept {
    if (unlikely(Idx >= CoreGlobInsts.size())) {
      return Unexpect(ErrCode::Value::WrongInstanceIndex);
    }
    return CoreGlobInsts[Idx];
  }
  Expect<TagInstance *> getCoreTag(uint32_t Idx) const noexcept {
    if (unlikely(Idx >= CoreTagInsts.size())) {
      return Unexpect(ErrCode::Value::WrongInstanceIndex);
    }
    return CoreTagInsts[Idx];
  }
  /// @}

  /// \name The canonical `handles` table.
  /// @{
  /// The live slot at an index. Error logging needs to be handled by the
  /// caller.
  Expect<const HandleSlot *> getHandleSlot(uint32_t Idx) const noexcept {
    if (!isSlotLive(Idx)) {
      return Unexpect(ErrCode::Value::ComponentHandleUnknown);
    }
    return &Handles[Idx];
  }
  /// Resource entries.
  Component::ResourceHandle *findResourceHandle(uint32_t Idx) noexcept {
    if (!isSlotLive(Idx)) {
      return nullptr;
    }
    return std::get_if<Component::ResourceHandle>(&Handles[Idx]);
  }
  /// Subtask entries: the callee task itself.
  Runtime::Component::Task *findSubtask(uint32_t Idx) const noexcept {
    if (!isSlotLive(Idx)) {
      return nullptr;
    }
    const auto *TaskPtr =
        std::get_if<Runtime::Component::Task *>(&Handles[Idx]);
    return TaskPtr != nullptr ? *TaskPtr : nullptr;
  }

  /// Stream and future entries.
  const Component::StreamHandle *findStreamHandle(uint32_t Idx) const noexcept {
    if (!isSlotLive(Idx)) {
      return nullptr;
    }
    return std::get_if<Component::StreamHandle>(&Handles[Idx]);
  }
  /// Waitable-set entries: the members in join order.
  std::vector<uint32_t> *findWaitableSet(uint32_t Idx) noexcept {
    if (!isSlotLive(Idx)) {
      return nullptr;
    }
    return std::get_if<std::vector<uint32_t>>(&Handles[Idx]);
  }
  const std::vector<uint32_t> *findWaitableSet(uint32_t Idx) const noexcept {
    if (!isSlotLive(Idx)) {
      return nullptr;
    }
    return std::get_if<std::vector<uint32_t>>(&Handles[Idx]);
  }
  /// Error-context entries.
  const std::string *findErrorContext(uint32_t Idx) const noexcept {
    if (!isSlotLive(Idx)) {
      return nullptr;
    }
    return std::get_if<std::string>(&Handles[Idx]);
  }
  /// @}

  /// \name The concurrency state of the instance.
  /// @{
  /// Getter for the no-leave region flag.
  bool mayLeave() const noexcept { return MayLeave; }
  /// Getter for the backpressure counter.
  int64_t getBackpressure() const noexcept { return Backpressure; }
  /// Getter for the number of tasks blocked at the entry gate.
  uint32_t getNumWaitingToEnter() const noexcept { return NumWaitingToEnter; }
  /// Getter for the thread holding the instance exclusively.
  Runtime::Component::Thread *getExclusiveThread() const noexcept {
    return ExclusiveThread;
  }
  /// A trapped instance tree rejects every further entry.
  bool isPoisoned() const noexcept { return Poisoned; }
  /// Getter for a thread of the per-instance thread table.
  Runtime::Component::Thread *findThread(uint32_t Idx) const noexcept {
    return Idx != 0 && Idx < Threads.size() ? Threads[Idx] : nullptr;
  }
  /// Every registered thread; the dead slot 0 included as null.
  Span<Runtime::Component::Thread *const> getThreads() const noexcept {
    return Threads;
  }
  /// @}

protected:
  friend class Executor::ComponentExecutor;
  friend class Runtime::Component::Task;

  /// \name Mutators for instantiation.
  /// @{
  /// Take the copy of the component this instance walks.
  const AST::Component::Component &
  ownComponent(std::unique_ptr<AST::Component::Component> &&CompAST) noexcept {
    OwnedComp = std::move(CompAST);
    return *OwnedComp;
  }

  /// A locally-defined type.
  void addType(const AST::Component::DefType &Ty) noexcept {
    Types.push_back({&Ty, nullptr, this});
  }
  /// A locally-defined resource type: mints the runtime identity.
  const Component::ResourceTypeInstance *
  addResourceType(const AST::Component::DefType &Ty,
                  FunctionInstance *Dtor) noexcept {
    OwnedResourceTypes.push_back(
        std::make_unique<Component::ResourceTypeInstance>(
            this, Dtor, nullptr,
            Ty.isResourceType() && Ty.getResourceType().isAddrI64()));
    Types.push_back({&Ty, OwnedResourceTypes.back().get(), this});
    return OwnedResourceTypes.back().get();
  }
  /// An imported or aliased type; an absent owner means this instance.
  void
  addTypeDefinition(const Component::ComponentTypeDefinition &Def) noexcept {
    Types.push_back(
        {Def.Def, Def.Resource, Def.Owner != nullptr ? Def.Owner : this});
  }
  void exportType(std::string_view Name,
                  const Component::ComponentTypeDefinition &Def) noexcept {
    ExpTypes.insert_or_assign(std::string(Name), Def);
  }

  /// An imported or aliased instance.
  void addComponentInstance(const ComponentInstance *Inst) noexcept {
    CompInsts.push_back(Inst);
  }

  /// A component definition closing over this instance as its environment.
  void addComponent(const AST::Component::Component &Comp) noexcept {
    Comps.push_back({&Comp, this});
  }
  void
  addComponentDefinition(const Component::ComponentDefinition &Def) noexcept {
    Comps.push_back(Def);
  }
  void exportComponent(std::string_view Name, uint32_t Idx) noexcept {
    assuming(Idx < Comps.size());
    ExpComps.insert_or_assign(std::string(Name), Comps[Idx]);
  }
  void exportComponent(std::string_view Name,
                       const Component::ComponentDefinition &Def) noexcept {
    ExpComps.insert_or_assign(std::string(Name), Def);
  }

  ComponentFunctionInstance *
  addFunction(std::unique_ptr<ComponentFunctionInstance> &&Inst) noexcept {
    OwnedFuncInsts.push_back(std::move(Inst));
    FuncInsts.push_back(OwnedFuncInsts.back().get());
    return OwnedFuncInsts.back().get();
  }
  void addFunction(ComponentFunctionInstance *Func) noexcept {
    FuncInsts.push_back(Func);
  }
  void exportFunction(std::string_view Name,
                      ComponentFunctionInstance *Func) noexcept {
    ExpFuncInsts.insert_or_assign(std::string(Name), Func);
  }

  void addValue(ComponentValVariant Val) noexcept {
    Values.push_back(std::move(Val));
  }
  void exportValue(std::string_view Name, ComponentValVariant Val) noexcept {
    ExpValues.insert_or_assign(std::string(Name), std::move(Val));
  }

  /// A locally-defined core type.
  void addCoreType(const AST::Component::CoreDefType &Ty) noexcept {
    CoreTypes.push_back(&Ty);
  }
  void exportCoreModule(std::string_view Name,
                        const AST::Module &Mod) noexcept {
    ExpCoreMods.insert_or_assign(std::string(Name), &Mod);
  }
  /// A core module instance this instance owns.
  const ModuleInstance *
  addCoreModuleInstance(std::unique_ptr<ModuleInstance> &&Inst) noexcept {
    OwnedCoreModInsts.push_back(std::move(Inst));
    CoreModInsts.push_back(OwnedCoreModInsts.back().get());
    return OwnedCoreModInsts.back().get();
  }
  /// An imported or aliased core module instance.
  void addCoreModuleInstance(const ModuleInstance *Inst) noexcept {
    CoreModInsts.push_back(Inst);
  }
  /// A synthesized host function in the core function index space.
  FunctionInstance *
  addCoreHostFunction(std::unique_ptr<HostFunctionBase> &&Host) noexcept {
    OwnedCoreFuncInsts.push_back(
        std::make_unique<FunctionInstance>(std::move(Host)));
    CoreFuncInsts.push_back(OwnedCoreFuncInsts.back().get());
    return OwnedCoreFuncInsts.back().get();
  }
  void addCoreFunction(FunctionInstance *Func) noexcept {
    CoreFuncInsts.push_back(Func);
  }
  void addCoreTable(TableInstance *Tab) noexcept {
    CoreTabInsts.push_back(Tab);
  }
  void addCoreMemory(MemoryInstance *Mem) noexcept {
    CoreMemInsts.push_back(Mem);
  }
  void addCoreGlobal(GlobalInstance *Glob) noexcept {
    CoreGlobInsts.push_back(Glob);
  }
  void addCoreTag(TagInstance *Tag) noexcept { CoreTagInsts.push_back(Tag); }
  /// @}

  /// \name Mutators for the `handles` table.
  /// @{
  uint32_t
  addResourceHandle(const Component::ResourceTypeInstance *ResType,
                    uint64_t Rep, bool Own,
                    Runtime::Component::Task *BorrowScope = nullptr) noexcept {
    return addSlot(HandleSlot{
        Component::ResourceHandle{ResType, Rep, Own, 0, BorrowScope}});
  }
  std::optional<Component::ResourceHandle>
  removeResourceHandle(uint32_t Idx) noexcept {
    auto *Slot = findResourceHandle(Idx);
    if (Slot == nullptr) {
      return std::nullopt;
    }
    Component::ResourceHandle Out = *Slot;
    removeSlot(Idx);
    return Out;
  }

  uint32_t addSubtask(Runtime::Component::Task &T) noexcept {
    return addSlot(HandleSlot{&T});
  }
  /// The task behind a removed subtask; null when the slot holds none.
  Runtime::Component::Task *removeSubtask(uint32_t Idx) noexcept {
    auto *T = findSubtask(Idx);
    if (T != nullptr) {
      removeSlot(Idx);
    }
    return T;
  }

  /// The end of the role Role of S enters this table.
  uint32_t addStreamHandle(Component::StreamInstance &S,
                           Component::StreamInstance::Role Role) noexcept {
    S.setHolder(Role, this);
    return addSlot(HandleSlot{Component::StreamHandle{&S, Role}});
  }
  /// The stream behind a removed handle; null when the slot holds no stream.
  Component::StreamInstance *removeStreamHandle(uint32_t Idx) noexcept {
    const auto *Slot = findStreamHandle(Idx);
    if (Slot == nullptr) {
      return nullptr;
    }
    Component::StreamInstance *S = Slot->Stream;
    S->setHolder(Slot->Role, nullptr);
    removeSlot(Idx);
    return S;
  }

  uint32_t addWaitableSet() noexcept {
    return addSlot(HandleSlot{std::vector<uint32_t>{}});
  }
  bool removeWaitableSet(uint32_t Idx) noexcept {
    if (findWaitableSet(Idx) == nullptr) {
      return false;
    }
    removeSlot(Idx);
    return true;
  }

  uint32_t addErrorContext(std::string Msg) noexcept {
    return addSlot(HandleSlot{std::move(Msg)});
  }
  bool removeErrorContext(uint32_t Idx) noexcept {
    if (findErrorContext(Idx) == nullptr) {
      return false;
    }
    removeSlot(Idx);
    return true;
  }
  /// @}

  /// \name Mutators for the concurrency state.
  /// @{
  void setMayLeave(bool Flag) noexcept { MayLeave = Flag; }
  /// Raise the backpressure counter; false on overflow.
  bool incBackpressure() noexcept {
    Backpressure += 1;
    return Backpressure != MaxBackpressure;
  }
  /// Lower the backpressure counter; false on underflow.
  bool decBackpressure() noexcept {
    Backpressure -= 1;
    return Backpressure >= 0;
  }
  void incWaitingToEnter() noexcept { NumWaitingToEnter += 1; }
  void decWaitingToEnter() noexcept { NumWaitingToEnter -= 1; }
  void setExclusiveThread(Runtime::Component::Thread *Thread) noexcept {
    ExclusiveThread = Thread;
  }
  void setPoisoned() noexcept { Poisoned = true; }
  /// Register a thread activation; index 0 stays unused.
  uint32_t addThread(Runtime::Component::Thread *Thread) noexcept {
    if (Threads.empty()) {
      Threads.push_back(nullptr);
    }
    if (!FreeThreads.empty()) {
      const uint32_t Idx = FreeThreads.back();
      FreeThreads.pop_back();
      Threads[Idx] = Thread;
      return Idx;
    }
    Threads.push_back(Thread);
    return static_cast<uint32_t>(Threads.size() - 1);
  }
  void removeThread(uint32_t Idx) noexcept {
    if (Idx != 0 && Idx < Threads.size()) {
      Threads[Idx] = nullptr;
      FreeThreads.push_back(Idx);
    }
  }
  /// @}

  /// \name The store of a root instance: its tasks, threads, streams and
  /// scheduler.
  /// @{
  /// The kind of the embedder entry in progress; None outside one.
  enum class EntryKind : uint8_t {
    None,
    Invoke,
    Instantiate,
  };

  template <typename... Args>
  Runtime::Component::Task *newTask(Args &&...CtorArgs) {
    OwnedTasks.push_back(std::make_unique<Runtime::Component::Task>(
        std::forward<Args>(CtorArgs)...));
    return OwnedTasks.back().get();
  }
  template <typename... Args>
  Runtime::Component::Thread *newThread(Args &&...CtorArgs) {
    OwnedThreads.push_back(std::make_unique<Runtime::Component::Thread>(
        std::forward<Args>(CtorArgs)...));
    return OwnedThreads.back().get();
  }
  template <typename... Args>
  Component::StreamInstance *newStream(Args &&...CtorArgs) {
    OwnedStreamInsts.push_back(std::make_unique<Component::StreamInstance>(
        std::forward<Args>(CtorArgs)...));
    return OwnedStreamInsts.back().get();
  }

  /// This root and the roots it imports from, transitively, in address order.
  std::vector<ComponentInstance *> getStoreRoots() noexcept {
    std::vector<ComponentInstance *> Roots{this};
    for (size_t I = 0; I < Roots.size(); ++I) {
      for (auto *Provider : Roots[I]->Providers) {
        if (std::find(Roots.begin(), Roots.end(), Provider->Root) ==
            Roots.end()) {
          Roots.push_back(Provider->Root);
        }
      }
    }
    std::sort(Roots.begin(), Roots.end());
    return Roots;
  }

  /// The root whose entry runs this store; itself outside an entry.
  ComponentInstance *getEntryRoot() const noexcept {
    return Root->EntryRoot != nullptr ? Root->EntryRoot : Root;
  }

  /// Add a waiting thread once.
  void pushWaiting(Runtime::Component::Thread &Parked) noexcept {
    if (std::find(Waiting.begin(), Waiting.end(), &Parked) == Waiting.end()) {
      Waiting.push_back(&Parked);
    }
  }

  /// Abort the tasks of this tree, those sharing a stack with them and their
  /// callers; IsPoisoning poisons the guest trees that lose a task.
  void abortTasks(bool IsPoisoning) noexcept {
    std::unordered_set<Runtime::Component::Task *> Aborted;
    for (auto &T : OwnedTasks) {
      if (!T->isAborted()) {
        Aborted.insert(T.get());
      }
    }
    auto GetThreads = [](const Runtime::Component::Task &T) {
      const auto Registered = T.getThreads();
      std::vector<Runtime::Component::Thread *> All(Registered.begin(),
                                                    Registered.end());
      // A thread waiting at the entry gate is not registered yet.
      if (T.getImplicitThread() != nullptr &&
          std::find(All.begin(), All.end(), T.getImplicitThread()) ==
              All.end()) {
        All.push_back(T.getImplicitThread());
      }
      return All;
    };
    // The unwinding of one stack can end the threads of other tasks, so the
    // scan restarts until nothing changes.
    bool Progress = true;
    while (Progress) {
      Progress = false;
      const std::vector<Runtime::Component::Task *> Found(Aborted.begin(),
                                                          Aborted.end());
      for (auto *T : Found) {
        if (T->getCaller() != nullptr && !T->getCaller()->isAborted() &&
            Aborted.insert(T->getCaller()).second) {
          Progress = true;
        }
        for (auto *Thread : GetThreads(*T)) {
          for (auto *Sharing : Thread->getStackThreads()) {
            if (!Sharing->getOwner().isAborted() &&
                Aborted.insert(&Sharing->getOwner()).second) {
              Progress = true;
            }
          }
        }
      }
      for (auto *T : Found) {
        for (auto *Thread : GetThreads(*T)) {
          Runtime::Component::Thread *Top = Thread->getInnermost();
          if (Thread->getRoot().isEnded() || Top == nullptr || Top->isEnded() ||
              Top->isRunning()) {
            continue;
          }
          auto *TopInst = Top->getOwner().getInstance();
          Runtime::Component::Thread *Prev = nullptr;
          if (TopInst != nullptr) {
            Prev = std::exchange(TopInst->getEntryRoot()->Current, Top);
          }
          Top->onResume(Runtime::Component::Thread::WakeReason::Aborted);
          if (TopInst != nullptr) {
            TopInst->getEntryRoot()->Current = Prev;
          }
          Progress = true;
        }
      }
    }
    // The stores that lost a thread forget it.
    std::unordered_set<ComponentInstance *> Stores;
    for (auto *T : Aborted) {
      auto *Inst = T->getInstance();
      if (Inst == nullptr) {
        continue;
      }
      Stores.insert(Inst->Root);
      for (auto *Thread : GetThreads(*T)) {
        if (Thread->getIndex() != 0) {
          Inst->removeThread(Thread->getIndex());
          Thread->setIndex(0);
        }
      }
    }
    for (auto *Store : Stores) {
      Store->Waiting.erase(
          std::remove_if(Store->Waiting.begin(), Store->Waiting.end(),
                         [](const Runtime::Component::Thread *Parked) {
                           return Parked->isEnded();
                         }),
          Store->Waiting.end());
      if (Store->Current != nullptr && Store->Current->isEnded()) {
        Store->Current = nullptr;
      }
    }
    // A callee of an aborted task keeps callbacks naming what is going; one
    // still waiting to start has no arguments left and is cancelled instead.
    for (auto *Store : Stores) {
      for (auto *Provider : Store->getStoreRoots()) {
        for (auto &T : Provider->OwnedTasks) {
          if (T->getCaller() == nullptr || Aborted.count(T.get()) != 0 ||
              Aborted.count(T->getCaller()) == 0) {
            continue;
          }
          T->onCallerAbort();
          if (T->getSubtaskCode() == 0) {
            if (auto *Gated = T->onCancelRequest()) {
              Gated->onReady();
            }
          }
        }
      }
    }
    for (auto *T : Aborted) {
      // A guest tree that lost a task in the middle of a call is unusable.
      if (auto *Inst = T->getInstance(); Inst != nullptr) {
        if (IsPoisoning && !T->isHost()) {
          Inst->Root->Poisoned = true;
        }
      }
      T->onAbort();
    }
    // A stack of this root that is going carried threads of other stores.
    if (Terminating) {
      for (auto *Store : Stores) {
        if (Store != this && Store->Entry == EntryKind::None) {
          Store->drainEndedTasks();
        }
      }
    }
  }

  /// Drop the ended threads and the tasks nothing holds any more.
  void drainEndedTasks() noexcept {
    Waiting.erase(std::remove_if(Waiting.begin(), Waiting.end(),
                                 [](const Runtime::Component::Thread *Parked) {
                                   return Parked->isEnded();
                                 }),
                  Waiting.end());
    OwnedThreads.erase(
        std::remove_if(
            OwnedThreads.begin(), OwnedThreads.end(),
            [](const std::unique_ptr<Runtime::Component::Thread> &Parked) {
              return Parked->isEnded();
            }),
        OwnedThreads.end());
    OwnedTasks.erase(
        std::remove_if(OwnedTasks.begin(), OwnedTasks.end(),
                       [](const std::unique_ptr<Runtime::Component::Task> &T) {
                         return T->isReclaimable();
                       }),
        OwnedTasks.end());
  }

  /// Drop the streams both ends of which are dropped.
  void drainClosedStreams() noexcept {
    OwnedStreamInsts.erase(
        std::remove_if(OwnedStreamInsts.begin(), OwnedStreamInsts.end(),
                       [](const std::unique_ptr<Component::StreamInstance> &S) {
                         return S->isClosed();
                       }),
        OwnedStreamInsts.end());
  }

  /// Unpin what the handles of this instance and of the instances nested in
  /// it hold, and drop the stream ends among them.
  void releaseHandles() noexcept {
    for (auto &Slot : Handles) {
      if (auto *Subtask = std::get_if<Runtime::Component::Task *>(&Slot)) {
        (*Subtask)->releaseDependent();
      } else if (auto *Res = std::get_if<Component::ResourceHandle>(&Slot);
                 Res != nullptr && !Res->Own && Res->BorrowScope != nullptr) {
        Res->BorrowScope->dropBorrow();
      } else if (auto *End = std::get_if<Component::StreamHandle>(&Slot)) {
        End->Stream->onDrop(End->Role);
      }
    }
    Handles.clear();
    FreeSlots.clear();
    for (auto &Nested : OwnedCompInsts) {
      Nested->releaseHandles();
    }
  }

  /// A stream of this store an end of which sits in the table of another
  /// store moves to that store, with its creator gone; the rest go here.
  void releaseStreams() noexcept {
    for (auto &S : OwnedStreamInsts) {
      ComponentInstance *Holder =
          S->getHolder(Component::StreamInstance::Role::Reader);
      if (Holder == nullptr) {
        Holder = S->getHolder(Component::StreamInstance::Role::Writer);
      }
      if (Holder == nullptr || Holder->Root == this) {
        continue;
      }
      S->onCreatorGone();
      Holder->Root->OwnedStreamInsts.push_back(std::move(S));
    }
    OwnedStreamInsts.clear();
  }
  /// @}

  /// \name The linkers naming this instance, and the providers it imports from.
  /// @{
  friend class Runtime::Component::StoreManager;
  using LinkedStoreKey =
      std::pair<Runtime::Component::StoreManager *, std::string>;
  using BeforeStoreUnlinkCallback = void(const LinkedStoreKey &Key,
                                         const ComponentInstance *Inst);
  void linkStore(Runtime::Component::StoreManager *Store, std::string_view Name,
                 BeforeStoreUnlinkCallback *Callback) {
    LinkedStores.insert_or_assign(LinkedStoreKey{Store, std::string(Name)},
                                  Callback);
  }
  void unlinkStore(Runtime::Component::StoreManager *Store,
                   std::string_view Name) {
    LinkedStores.erase(LinkedStoreKey{Store, std::string(Name)});
  }
  void unlinkAllStores() noexcept {
    std::map<LinkedStoreKey, BeforeStoreUnlinkCallback *> Stores;
    Stores.swap(LinkedStores);
    for (auto &&[Key, Callback] : Stores) {
      Callback(Key, this);
    }
  }
  /// Release the providers; one whose owner already let go is deleted.
  void releaseProviders() noexcept {
    std::vector<ComponentInstance *> ToDelete;
    for (auto *Provider : Providers) {
      if (Provider->Life.releaseDependent()) {
        ToDelete.push_back(Provider);
      }
    }
    Providers.clear();
    for (auto *Provider : ToDelete) {
      delete Provider;
    }
  }
  /// @}

private:
  /// The backpressure counter overflows at this value.
  static inline constexpr const int64_t MaxBackpressure = INT64_C(1) << 16;

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

  /// Index 0 stays unused; freed slots are reused LIFO.
  uint32_t addSlot(HandleSlot &&Slot) noexcept {
    if (Handles.empty()) {
      Handles.emplace_back();
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
  void removeSlot(uint32_t Idx) noexcept {
    Handles[Idx] = std::monostate{};
    FreeSlots.push_back(Idx);
  }

  /// \name Data of component instance.
  /// @{
  const std::string CompName;
  ComponentInstance *Parent;
  /// The root of the lexical instantiation tree.
  ComponentInstance *Root;

  /// The index spaces.
  std::vector<Component::ComponentTypeDefinition> Types;
  std::vector<const ComponentInstance *> CompInsts;
  std::vector<Component::ComponentDefinition> Comps;
  std::vector<ComponentFunctionInstance *> FuncInsts;
  std::vector<ComponentValVariant> Values;
  std::vector<const AST::Component::CoreDefType *> CoreTypes;
  std::vector<const AST::Module *> CoreMods;
  std::vector<const ModuleInstance *> CoreModInsts;
  std::vector<FunctionInstance *> CoreFuncInsts;
  std::vector<TableInstance *> CoreTabInsts;
  std::vector<MemoryInstance *> CoreMemInsts;
  std::vector<GlobalInstance *> CoreGlobInsts;
  std::vector<TagInstance *> CoreTagInsts;
  /// Host resource types by the C++ tag they were registered under.
  std::map<std::type_index, uint32_t> HostResourceTags;

  /// Exported name maps.
  std::map<std::string, Component::ComponentTypeDefinition, std::less<>>
      ExpTypes;
  std::map<std::string, const ComponentInstance *, std::less<>> ExpCompInsts;
  std::map<std::string, Component::ComponentDefinition, std::less<>> ExpComps;
  std::map<std::string, ComponentFunctionInstance *, std::less<>> ExpFuncInsts;
  std::map<std::string, ComponentValVariant, std::less<>> ExpValues;
  std::map<std::string, const AST::Module *, std::less<>> ExpCoreMods;

  /// Owned definitions and runtime objects.
  std::unique_ptr<AST::Component::Component> OwnedComp;
  std::vector<std::unique_ptr<AST::Component::DefType>> OwnedTypes;
  std::vector<std::unique_ptr<Component::ResourceTypeInstance>>
      OwnedResourceTypes;
  std::vector<std::unique_ptr<ComponentInstance>> OwnedCompInsts;
  std::vector<std::unique_ptr<ComponentFunctionInstance>> OwnedFuncInsts;
  std::vector<std::unique_ptr<ModuleInstance>> OwnedCoreModInsts;
  /// The synthesized host functions of the canon section.
  std::vector<std::unique_ptr<FunctionInstance>> OwnedCoreFuncInsts;

  /// The `handles` table; a deque keeps entries at stable addresses.
  std::deque<HandleSlot> Handles;
  std::vector<uint32_t> FreeSlots;

  /// The concurrency state.
  bool MayLeave = true;
  int64_t Backpressure = 0;
  uint32_t NumWaitingToEnter = 0;
  Runtime::Component::Thread *ExclusiveThread = nullptr;
  bool Poisoned = false;
  std::vector<Runtime::Component::Thread *> Threads;
  std::vector<uint32_t> FreeThreads;

  /// The lifetime: who points here, whom this imports from, the owner flag.
  std::map<LinkedStoreKey, BeforeStoreUnlinkCallback *> LinkedStores;
  std::unordered_set<ComponentInstance *> Providers;
  RefLifetime Life;

  /// The store of a root instance: the waiting threads in park order, the
  /// thread holding the stack, the entry state and the latched trap.
  std::vector<Runtime::Component::Thread *> Waiting;
  /// The root whose entry runs this store, and on that root the thread
  /// holding the stack: one thread runs at a time in an entry.
  ComponentInstance *EntryRoot = nullptr;
  Runtime::Component::Thread *Current = nullptr;
  /// On the entry root, the stores its entry runs; empty outside an entry.
  std::vector<ComponentInstance *> EntryStores;
  EntryKind Entry = EntryKind::None;
  /// Held by an entry over every store it runs. An instance is never
  /// destroyed while it or a store importing from it is entered.
  std::mutex EntryMutex;
  std::optional<ErrCode> Trap;
  bool Terminating = false;
  /// Declared last: the threads join before the tasks they run for go, the
  /// tasks before the streams, and all before everything they name.
  std::vector<std::unique_ptr<Component::StreamInstance>> OwnedStreamInsts;
  std::vector<std::unique_ptr<Runtime::Component::Task>> OwnedTasks;
  std::vector<std::unique_ptr<Runtime::Component::Thread>> OwnedThreads;
  /// @}
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge

#include "runtime/component/task.ipp"
