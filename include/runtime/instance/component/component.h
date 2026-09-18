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
#include "runtime/component/thread.h"
#include "runtime/instance/component/function.h"
#include "runtime/instance/component/resource.h"
#include "runtime/instance/component/stream.h"
#include "runtime/instance/module.h"

#include <cstdint>
#include <deque>
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
} // namespace Executor

namespace Runtime {
namespace Component {
class Task;
} // namespace Component

namespace Instance {

/// The runtime state of one component instantiation: the index spaces, the
/// owned runtime objects, and the exports. A root instance owns the copy of
/// the component AST its index spaces point into; a nested one points into
/// the copy of the instance it belongs to.
class ComponentInstance {
public:
  /// A component definition plus the lexical environment it closed over.
  struct ComponentDefinition {
    const AST::Component::Component *Comp = nullptr;
    ComponentInstance *Env = nullptr;
  };

  /// A resource entry of the `handles` table. The table keeps it at a stable
  /// address, so a lending subtask may hold a pointer to it.
  struct ResourceHandle {
    const Component::ResourceTypeInstance *ResType = nullptr;
    uint64_t Rep = 0;
    bool Own = true;
    uint32_t NumLends = 0;
    /// The task that received this borrow; null for own and host handles.
    Runtime::Component::Task *BorrowScope = nullptr;
  };

  /// A stream or future end entry of the `handles` table.
  using StreamEnd =
      std::pair<std::shared_ptr<Component::Stream>, Component::Stream::End>;

  /// One slot of the `handles` table; the monostate marks a free slot.
  using HandleSlot =
      std::variant<std::monostate, ResourceHandle,
                   std::shared_ptr<Runtime::Component::Task>,
                   std::vector<uint32_t>, StreamEnd, std::string>;

  /// A type definition, the runtime identity of a resource type, and the
  /// instance whose index space the nested type indices belong to.
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
  /// The threads of this instance end with it: a parked one unwinds now,
  /// before any member it may still read goes away.
  virtual ~ComponentInstance() noexcept {
    for (auto *Entry : Threads) {
      if (Entry == nullptr) {
        continue;
      }
      if (!Entry->isEnded() && Entry->getInnermost() == Entry &&
          !Entry->isRunning()) {
        Entry->onResume(Runtime::Component::Thread::Reason::Abort);
      }
      Entry->setIndex(0);
    }
    Threads.clear();
  }
  ComponentInstance(const ComponentInstance &) = delete;
  ComponentInstance &operator=(const ComponentInstance &) = delete;

  /// Getter for the component name.
  std::string_view getComponentName() const noexcept { return CompName; }

  /// Getter for the lexical parent.
  ComponentInstance *getParent() noexcept { return Parent; }
  const ComponentInstance *getParent() const noexcept { return Parent; }

  /// Take the copy of the component this instance is instantiated from. A
  /// root instance walks this copy, so its index spaces never point into an
  /// AST its embedder may free.
  const AST::Component::Component &
  ownComponent(std::unique_ptr<AST::Component::Component> &&CompAST) noexcept {
    OwnedComp = std::move(CompAST);
    return *OwnedComp;
  }

  /// Root of the lexical instantiation tree (poisoning + host-entry checks).
  const ComponentInstance *getRoot() const noexcept { return Root; }

  /// True when callee and caller are the same instance or lexical relatives,
  /// for which an adapter call always traps.
  bool isLinealRelative(const ComponentInstance *Other) const noexcept {
    if (Other == nullptr) {
      return false;
    }
    for (const ComponentInstance *Inst = this; Inst != nullptr;
         Inst = Inst->Parent) {
      if (Inst == Other) {
        return true;
      }
    }
    for (const ComponentInstance *Inst = Other; Inst != nullptr;
         Inst = Inst->Parent) {
      if (Inst == this) {
        return true;
      }
    }
    return false;
  }

  /// \name The canonical `handles` table, one index space for every kind.
  /// @{
  /// The live slot at an index, whatever kind it holds. Error logging needs
  /// to be handled by the caller.
  Expect<const HandleSlot *> getHandleSlot(uint32_t Idx) const noexcept {
    if (!isSlotLive(Idx)) {
      return Unexpect(ErrCode::Value::ComponentHandleUnknown);
    }
    return &Handles[Idx];
  }

  /// Resource entries.
  ResourceHandle *findHandle(uint32_t Idx) const noexcept {
    if (!isSlotLive(Idx)) {
      return nullptr;
    }
    return std::get_if<ResourceHandle>(&Handles[Idx]);
  }

  /// Subtask entries: the callee task itself.
  Runtime::Component::Task *findSubtask(uint32_t Idx) const noexcept {
    if (!isSlotLive(Idx)) {
      return nullptr;
    }
    if (auto *TaskPtr = std::get_if<std::shared_ptr<Runtime::Component::Task>>(
            &Handles[Idx])) {
      return TaskPtr->get();
    }
    return nullptr;
  }

  /// Stream and future end entries.
  const StreamEnd *findStreamEnd(uint32_t Idx) const noexcept {
    if (!isSlotLive(Idx)) {
      return nullptr;
    }
    return std::get_if<StreamEnd>(&Handles[Idx]);
  }

  /// Waitable-set entries: the waitables that joined, in join order.
  std::vector<uint32_t> *findWaitableSet(uint32_t Idx) const noexcept {
    if (!isSlotLive(Idx)) {
      return nullptr;
    }
    return std::get_if<std::vector<uint32_t>>(&Handles[Idx]);
  }

  /// Error-context entries.
  std::string *findErrorContext(uint32_t Idx) const noexcept {
    if (!isSlotLive(Idx)) {
      return nullptr;
    }
    return std::get_if<std::string>(&Handles[Idx]);
  }
  /// @}

  /// \name The concurrency state of the instance.
  /// @{
  /// True while the instance is executing or still instantiating.
  bool isEntered() const noexcept { return Entered; }

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

  /// \name Exports: value and component function.
  /// @{
  const ComponentValVariant *findValue(std::string_view Name) const noexcept {
    auto Iter = ExpValues.find(Name);
    return Iter != ExpValues.end() ? &Iter->second : nullptr;
  }
  /// Register a host component function under an export name; it declares
  /// its type into the type index space of this instance.
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

  /// \name Index space and exports: component instance.
  /// @{
  /// A nested instance this instance owns.
  const ComponentInstance *
  addComponentInstance(std::unique_ptr<ComponentInstance> &&Inst) noexcept {
    OwnedCompInsts.push_back(std::move(Inst));
    CompInsts.push_back(OwnedCompInsts.back().get());
    return OwnedCompInsts.back().get();
  }
  Expect<const ComponentInstance *>
  getComponentInstance(uint32_t Idx) const noexcept {
    if (unlikely(Idx >= CompInsts.size())) {
      return Unexpect(ErrCode::Value::WrongInstanceIndex);
    }
    return CompInsts[Idx];
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

  /// \name Index space and exports: component.
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

  /// \name Index space and exports: core module.
  /// @{
  /// A core module definition; a host instance exports the ones it offers.
  void addModule(const AST::Module &Mod) noexcept { CoreMods.push_back(&Mod); }
  Expect<const AST::Module *> getModule(uint32_t Idx) const noexcept {
    if (unlikely(Idx >= CoreMods.size())) {
      return Unexpect(ErrCode::Value::WrongInstanceIndex);
    }
    return CoreMods[Idx];
  }
  void exportCoreModule(std::string_view Name, uint32_t Idx) noexcept {
    assuming(Idx < CoreMods.size());
    ExpCoreMods.insert_or_assign(std::string(Name), CoreMods[Idx]);
  }
  const AST::Module *findCoreModule(std::string_view Name) const noexcept {
    return findExport(ExpCoreMods, Name);
  }
  /// @}

  /// \name Index space and exports: core module instance.
  /// @{
  Expect<const ModuleInstance *>
  getCoreModuleInstance(uint32_t Idx) const noexcept {
    if (unlikely(Idx >= CoreModInsts.size())) {
      return Unexpect(ErrCode::Value::WrongInstanceIndex);
    }
    return CoreModInsts[Idx];
  }
  /// @}

  /// \name Index spaces: core function, table, memory, global, and tag.
  /// @{
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

  /// \name Index spaces: component function and value.
  /// @{
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
  /// @}

protected:
  friend class Executor::ComponentExecutor;
  friend class Runtime::Component::Task;

  /// \name Mutators for the `handles` table.
  /// @{
  uint32_t
  addHandle(const Component::ResourceTypeInstance *ResType, uint64_t Rep,
            bool Own,
            Runtime::Component::Task *BorrowScope = nullptr) const noexcept {
    return addSlot(
        HandleSlot{ResourceHandle{ResType, Rep, Own, 0, BorrowScope}});
  }
  std::optional<ResourceHandle> removeHandle(uint32_t Idx) const noexcept {
    auto *Slot = findHandle(Idx);
    if (Slot == nullptr) {
      return std::nullopt;
    }
    ResourceHandle Out = *Slot;
    removeSlot(Idx);
    return Out;
  }

  uint32_t
  addSubtask(std::shared_ptr<Runtime::Component::Task> T) const noexcept {
    return addSlot(HandleSlot{std::move(T)});
  }
  std::shared_ptr<Runtime::Component::Task>
  removeSubtask(uint32_t Idx) const noexcept {
    if (auto *TaskPtr = std::get_if<std::shared_ptr<Runtime::Component::Task>>(
            isSlotLive(Idx) ? &Handles[Idx] : nullptr)) {
      auto Out = std::move(*TaskPtr);
      removeSlot(Idx);
      return Out;
    }
    return nullptr;
  }

  uint32_t addStreamEnd(std::shared_ptr<Component::Stream> Shared,
                        Component::Stream::End End) const noexcept {
    return addSlot(HandleSlot{StreamEnd{std::move(Shared), End}});
  }
  std::shared_ptr<Component::Stream>
  removeStreamEnd(uint32_t Idx) const noexcept {
    if (auto *EndPtr =
            std::get_if<StreamEnd>(isSlotLive(Idx) ? &Handles[Idx] : nullptr)) {
      auto Out = std::move(EndPtr->first);
      removeSlot(Idx);
      return Out;
    }
    return nullptr;
  }

  uint32_t addWaitableSet() const noexcept {
    return addSlot(HandleSlot{std::vector<uint32_t>{}});
  }
  bool removeWaitableSet(uint32_t Idx) const noexcept {
    if (findWaitableSet(Idx) == nullptr) {
      return false;
    }
    removeSlot(Idx);
    return true;
  }

  uint32_t addErrorContext(std::string Msg) const noexcept {
    return addSlot(HandleSlot{std::move(Msg)});
  }
  bool removeErrorContext(uint32_t Idx) const noexcept {
    if (findErrorContext(Idx) == nullptr) {
      return false;
    }
    removeSlot(Idx);
    return true;
  }
  /// @}

  /// \name Mutators for the concurrency state.
  /// @{
  void setEntered(bool Flag) const noexcept { Entered = Flag; }
  void setMayLeave(bool Flag) const noexcept { MayLeave = Flag; }

  /// Raise the backpressure counter; false on overflow.
  bool incBackpressure() const noexcept {
    Backpressure += 1;
    return Backpressure != MaxBackpressure;
  }
  /// Lower the backpressure counter; false on underflow.
  bool decBackpressure() const noexcept {
    Backpressure -= 1;
    return Backpressure >= 0;
  }

  void incWaitingToEnter() const noexcept { NumWaitingToEnter += 1; }
  void decWaitingToEnter() const noexcept { NumWaitingToEnter -= 1; }

  void setExclusiveThread(Runtime::Component::Thread *Thread) const noexcept {
    ExclusiveThread = Thread;
  }
  void setPoisoned() const noexcept { Poisoned = true; }

  /// Register a thread activation; index 0 stays unused.
  uint32_t addThread(Runtime::Component::Thread *Thread) const noexcept {
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
  void removeThread(uint32_t Idx) const noexcept {
    if (Idx != 0 && Idx < Threads.size()) {
      Threads[Idx] = nullptr;
      FreeThreads.push_back(Idx);
    }
  }
  /// @}

  /// \name Mutators for instantiation.
  /// @{
  void exportValue(std::string_view Name, ComponentValVariant Val) noexcept {
    ExpValues.insert_or_assign(std::string(Name), std::move(Val));
  }
  void addValue(ComponentValVariant Val) noexcept {
    Values.push_back(std::move(Val));
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
  void addTypeDefinition(const TypeDefinition &Def) noexcept {
    Types.push_back(
        {Def.Def, Def.Resource, Def.Owner != nullptr ? Def.Owner : this});
  }
  void exportType(std::string_view Name, const TypeDefinition &Def) noexcept {
    ExpTypes.insert_or_assign(std::string(Name), Def);
  }

  /// A component definition closing over this instance as its environment.
  void addComponent(const AST::Component::Component &Comp) noexcept {
    Comps.push_back({&Comp, this});
  }
  void addComponentDefinition(const ComponentDefinition &Def) noexcept {
    Comps.push_back(Def);
  }
  void exportComponent(std::string_view Name, uint32_t Idx) noexcept {
    assuming(Idx < Comps.size());
    ExpComps.insert_or_assign(std::string(Name), Comps[Idx]);
  }
  void exportComponent(std::string_view Name,
                       const ComponentDefinition &Def) noexcept {
    ExpComps.insert_or_assign(std::string(Name), Def);
  }

  /// An imported or aliased instance.
  void addComponentInstance(const ComponentInstance *Inst) noexcept {
    CompInsts.push_back(Inst);
  }

  /// A synthesized host function in the core function index space; the core
  /// module importing it registers its type.
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
  /// @}

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

  /// The backpressure counter overflows at this value.
  static inline constexpr const int64_t MaxBackpressure = INT64_C(1) << 16;

  /// Index 0 stays unused, and a removed slot returns through the LIFO list.
  uint32_t addSlot(HandleSlot &&Slot) const noexcept {
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
  void removeSlot(uint32_t Idx) const noexcept {
    Handles[Idx] = std::monostate{};
    FreeSlots.push_back(Idx);
  }

  /// \name Data of component instance.
  /// @{
  const std::string CompName;
  ComponentInstance *const Parent;
  /// The root of the lexical instantiation tree.
  ComponentInstance *const Root;

  /// The call-time state, mutable behind the const instance of the canon
  /// options. A deque keeps the entries at stable addresses.
  mutable std::deque<HandleSlot> Handles;
  mutable std::vector<uint32_t> FreeSlots;
  mutable bool MayLeave = true;
  mutable int64_t Backpressure = 0;
  mutable uint32_t NumWaitingToEnter = 0;
  mutable Runtime::Component::Thread *ExclusiveThread = nullptr;
  mutable bool Poisoned = false;
  mutable bool Entered = false;
  mutable std::vector<Runtime::Component::Thread *> Threads;
  mutable std::vector<uint32_t> FreeThreads;

  /// The index spaces.
  std::vector<TypeDefinition> Types;
  /// Host resource types by the C++ tag they were registered under.
  std::map<std::type_index, uint32_t> HostResourceTags;
  std::vector<ComponentDefinition> Comps;
  std::vector<const ComponentInstance *> CompInsts;
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

  /// Owned definitions and runtime objects.
  std::unique_ptr<AST::Component::Component> OwnedComp;
  std::vector<std::unique_ptr<AST::Component::DefType>> OwnedTypes;
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
  std::map<std::string, const AST::Module *, std::less<>> ExpCoreMods;
  /// @}
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
