// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/runtime/instance/module.h - Module Instance definition ---===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the module instance definition in store manager.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/component/component.h"
#include "ast/module.h"
#include "common/errcode.h"
#include "runtime/hostfunc.h"
#include "runtime/instance/data.h"
#include "runtime/instance/elem.h"
#include "runtime/instance/exception.h"
#include "runtime/instance/function.h"
#include "runtime/instance/global.h"
#include "runtime/instance/memory.h"
#include "runtime/instance/reflifetime.h"
#include "runtime/instance/table.h"
#include "runtime/instance/tag.h"

#include <atomic>
#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <shared_mutex>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <vector>

namespace WasmEdge {

namespace Executor {
class Executor;
}

namespace Runtime {

class StoreManager;
class CallingFrame;

namespace Instance {

namespace {
/// Return true if T is an entity that can be exported or imported.
template <typename T>
inline constexpr const bool IsEntityV =
    std::is_same_v<T, Instance::FunctionInstance> ||
    std::is_same_v<T, Instance::TableInstance> ||
    std::is_same_v<T, Instance::MemoryInstance> ||
    std::is_same_v<T, Instance::GlobalInstance> ||
    std::is_same_v<T, Instance::TagInstance>;

/// Return true if T is an instance.
template <typename T>
inline constexpr const bool IsInstanceV =
    IsEntityV<T> || std::is_same_v<T, Instance::ElementInstance> ||
    std::is_same_v<T, Instance::DataInstance>;
} // namespace

class ComponentInstance;

/// Module instances use a dependency-pinned lifetime: each importer pins the
/// instances it imports from (see RefLifetime) and releases them when
/// destroyed, so a heap instance torn down via terminate() is deleted only
/// after its owner and all importers release.
///
/// A stack- or member-allocated provider cannot be deferred by a pin, so it
/// must outlive its importers: declare a host module before the VM or
/// StoreManager that imports it. The destructor enforces this with
/// assuming(!hasDependents()).
class ModuleInstance {
public:
  ModuleInstance(std::string_view Name, void *Data = nullptr,
                 std::function<void(void *)> Finalizer = nullptr)
      : ModName(Name), HostData(Data), HostDataFinalizer(Finalizer) {}
  virtual ~ModuleInstance() noexcept {
    // Fallback for instances torn down outside terminate() (e.g.
    // stack-allocated).
    unlinkAllStores();
    assuming(!Life.hasDependents());
    if (HostDataFinalizer.operator bool()) {
      HostDataFinalizer(HostData);
    }
    releaseProviders();
  }

  void terminate() noexcept {
    unlinkAllStores();
    if (Life.releaseOwner()) {
      delete this;
    }
  }

  /// Pin this instance alive for one out-of-band user whose access may outlive
  /// the pinning call -- e.g. a detached async invocation whose worker
  /// dereferences a FunctionInstance from this module after the launch call
  /// returns. Mirrors the importer dependency pin (addDependency): a heap
  /// instance torn down via terminate() is not deleted while any such pin is
  /// live. Balance each pin() with exactly one unpin(). const because a
  /// lifetime pin does not alter logical module state; Life is a mutable
  /// intrusive counter.
  void pin() const noexcept { Life.addDependent(); }

  /// Release one pin() taken earlier. Returns true iff this was the last
  /// dependent and the owner has already released (terminate()), in which case
  /// the caller MUST `delete` this instance now. A still-owned instance
  /// (stack/member, or store-held before terminate()) always returns false.
  [[nodiscard]] bool unpin() const noexcept { return Life.releaseDependent(); }

  /// Storage-class marker for the async launch path. True iff this instance
  /// is heap-allocated with a terminate()-managed deletion contract, so a
  /// ModulePin taken by a detached async invocation can actually DEFER its
  /// deletion. Set by the runtime instantiation paths, the C API creation
  /// wrappers, and the VM's built-in/plugin registration; an
  /// embedder-constructed (stack/member) module never gets it, and
  /// Executor::asyncInvoke refuses such a target up front -- a pin on it
  /// could only turn a racing teardown into an abort, never defer it.
  void setDeferrableStorage() noexcept {
    DeferrableStorage.store(true, std::memory_order_release);
  }
  bool isDeferrableStorage() const noexcept {
    return DeferrableStorage.load(std::memory_order_acquire);
  }

  /// Mark this module instance finalized (immutable). Idempotent, thread-safe.
  void finalizeInstantiation() const noexcept {
    if (InstantiateFinalized.load(std::memory_order_acquire)) {
      return;
    }
    std::unique_lock Lock(Mutex);
    InstantiateFinalized.store(true, std::memory_order_release);
  }

  /// Return true if this module instance has been finalized (immutable).
  bool isInstantiateFinalized() const noexcept {
    return InstantiateFinalized.load(std::memory_order_acquire);
  }

  /// \name GC-capability flag, carried on the *instance*.
  ///
  /// AST::Module::getGCCompiled() says whether an artifact's native code was
  /// built with GC support; Executor::instantiate consumes it and deopts a
  /// non-capable module to the interpreter. That choke point only covers
  /// modules this executor instantiated. A module instantiated by a GC-off
  /// executor keeps its compiled functions bound, and registerModule or a
  /// direct Executor::invoke can then hand those to a GC-enabled executor,
  /// which would run native code that never polls a safepoint and publishes no
  /// shadow roots. Copying the bit onto the instance lets enterFunction refuse
  /// that at the call itself. Defaults to true so host modules, embedder-built
  /// instances, and every interpreter path keep prior behavior.
  /// @{
  void setGCCompiled(bool V) noexcept {
    GCCompiled.store(V, std::memory_order_release);
  }
  bool isGCCompiled() const noexcept {
    return GCCompiled.load(std::memory_order_acquire);
  }
  /// @}

  std::string_view getModuleName() const noexcept {
    std::shared_lock Lock(Mutex);
    return ModName;
  }

  void *getHostData() const noexcept { return HostData; }

  Span<const FunctionInstance *const> getFunctionInstances() const noexcept {
    return Span<const FunctionInstance *const>(
        const_cast<const FunctionInstance *const *>(FuncInsts.data()),
        FuncInsts.size());
  }

  Span<const MemoryInstance *const> getMemoryInstances() const noexcept {
    return Span<const MemoryInstance *const>(
        const_cast<const MemoryInstance *const *>(MemInsts.data()),
        MemInsts.size());
  }

  Span<const GlobalInstance *const> getGlobalInstances() const noexcept {
    return Span<const GlobalInstance *const>(
        const_cast<const GlobalInstance *const *>(GlobInsts.data()),
        GlobInsts.size());
  }

  Span<const DataInstance *const> getOwnedDataInstances() const noexcept {
    return Span<const DataInstance *const>(
        const_cast<const DataInstance *const *>(DataInsts.data()),
        DataInsts.size());
  }

  /// Add existing instances and move ownership with the export name.
  ///
  /// These functions fail with `ErrCode::Value::WrongVMWorkflow` if the module
  /// instance has already been finalized, i.e. it has been used during
  /// execution and become immutable. On failure the passed-in `unique_ptr` is
  /// not moved from, so the caller retains ownership of the instance.
  Expect<void> addHostFunc(std::string_view Name,
                           std::unique_ptr<HostFunctionBase> &&Func) {
    std::unique_lock Lock(Mutex);
    if (unlikely(InstantiateFinalized.load(std::memory_order_acquire))) {
      return Unexpect(ErrCode::Value::WrongVMWorkflow);
    }
    unsafeImportDefinedType(Func->getDefinedType());
    unsafeAddHostInstance(
        Name, OwnedFuncInsts, FuncInsts, ExpFuncs,
        std::make_unique<FunctionInstance>(
            this, static_cast<uint32_t>(Types.size()) - 1, std::move(Func)));
    return {};
  }
  Expect<void> addHostFunc(std::string_view Name,
                           std::unique_ptr<FunctionInstance> &&Func) {
    std::unique_lock Lock(Mutex);
    if (unlikely(InstantiateFinalized.load(std::memory_order_acquire))) {
      return Unexpect(ErrCode::Value::WrongVMWorkflow);
    }
    assuming(Func->isHostFunction());
    unsafeImportDefinedType(Func->getHostFunc().getDefinedType());
    Func->linkDefinedType(this, static_cast<uint32_t>(Types.size()) - 1);
    unsafeAddHostInstance(Name, OwnedFuncInsts, FuncInsts, ExpFuncs,
                          std::move(Func));
    return {};
  }

  Expect<void> addHostTable(std::string_view Name,
                            std::unique_ptr<TableInstance> &&Tab) {
    std::unique_lock Lock(Mutex);
    if (unlikely(InstantiateFinalized.load(std::memory_order_acquire))) {
      return Unexpect(ErrCode::Value::WrongVMWorkflow);
    }
    unsafeAddHostInstance(Name, OwnedTabInsts, TabInsts, ExpTables,
                          std::move(Tab));
    return {};
  }
  Expect<void> addHostMemory(std::string_view Name,
                             std::unique_ptr<MemoryInstance> &&Mem) {
    std::unique_lock Lock(Mutex);
    if (unlikely(InstantiateFinalized.load(std::memory_order_acquire))) {
      return Unexpect(ErrCode::Value::WrongVMWorkflow);
    }
    unsafeAddHostInstance(Name, OwnedMemInsts, MemInsts, ExpMems,
                          std::move(Mem));
    return {};
  }
  Expect<void> addHostGlobal(std::string_view Name,
                             std::unique_ptr<GlobalInstance> &&Glob) {
    std::unique_lock Lock(Mutex);
    if (unlikely(InstantiateFinalized.load(std::memory_order_acquire))) {
      return Unexpect(ErrCode::Value::WrongVMWorkflow);
    }
    unsafeAddHostInstance(Name, OwnedGlobInsts, GlobInsts, ExpGlobals,
                          std::move(Glob));
    return {};
  }

  /// Find and get the exported instance by name.
  FunctionInstance *findFuncExports(std::string_view ExtName) const noexcept {
    std::shared_lock Lock(Mutex);
    return unsafeFindExports(ExpFuncs, ExtName);
  }
  TableInstance *findTableExports(std::string_view ExtName) const noexcept {
    std::shared_lock Lock(Mutex);
    return unsafeFindExports(ExpTables, ExtName);
  }
  MemoryInstance *findMemoryExports(std::string_view ExtName) const noexcept {
    std::shared_lock Lock(Mutex);
    return unsafeFindExports(ExpMems, ExtName);
  }
  TagInstance *findTagExports(std::string_view ExtName) const noexcept {
    std::shared_lock Lock(Mutex);
    return unsafeFindExports(ExpTags, ExtName);
  }
  GlobalInstance *findGlobalExports(std::string_view ExtName) const noexcept {
    std::shared_lock Lock(Mutex);
    return unsafeFindExports(ExpGlobals, ExtName);
  }

  /// Get the exported instances count.
  uint32_t getFuncExportNum() const noexcept {
    std::shared_lock Lock(Mutex);
    return static_cast<uint32_t>(ExpFuncs.size());
  }
  uint32_t getTableExportNum() const noexcept {
    std::shared_lock Lock(Mutex);
    return static_cast<uint32_t>(ExpTables.size());
  }
  uint32_t getMemoryExportNum() const noexcept {
    std::shared_lock Lock(Mutex);
    return static_cast<uint32_t>(ExpMems.size());
  }
  uint32_t getTagExportNum() const noexcept {
    std::shared_lock Lock(Mutex);
    return static_cast<uint32_t>(ExpTags.size());
  }
  uint32_t getGlobalExportNum() const noexcept {
    std::shared_lock Lock(Mutex);
    return static_cast<uint32_t>(ExpGlobals.size());
  }

  /// Get the exported instances maps.
  template <typename CallbackT>
  auto getFuncExports(CallbackT &&CallBack) const noexcept {
    std::shared_lock Lock(Mutex);
    return std::forward<CallbackT>(CallBack)(ExpFuncs);
  }
  template <typename CallbackT>
  auto getTableExports(CallbackT &&CallBack) const noexcept {
    std::shared_lock Lock(Mutex);
    return std::forward<CallbackT>(CallBack)(ExpTables);
  }
  template <typename CallbackT>
  auto getMemoryExports(CallbackT &&CallBack) const noexcept {
    std::shared_lock Lock(Mutex);
    return std::forward<CallbackT>(CallBack)(ExpMems);
  }
  template <typename CallbackT>
  auto getTagExports(CallbackT &&CallBack) const noexcept {
    std::shared_lock Lock(Mutex);
    return std::forward<CallbackT>(CallBack)(ExpTags);
  }
  template <typename CallbackT>
  auto getGlobalExports(CallbackT &&CallBack) const noexcept {
    std::shared_lock Lock(Mutex);
    return std::forward<CallbackT>(CallBack)(ExpGlobals);
  }

protected:
  friend class Executor::Executor;
  friend class ComponentInstance;
  friend class Runtime::CallingFrame;

  /// Create and copy the defined type to this module instance.
  void addDefinedType(const AST::SubType &SType) {
    std::unique_lock Lock(Mutex);
    OwnedTypes.push_back(std::make_unique<AST::SubType>(SType));
    Types.push_back(OwnedTypes.back().get());
  }

  /// Create and add instances to this module instance.
  template <typename... Args> void addFunc(Args &&...Values) {
    std::unique_lock Lock(Mutex);
    unsafeAddInstance(OwnedFuncInsts, FuncInsts, this,
                      std::forward<Args>(Values)...);
  }
  template <typename... Args>
  Expect<void> addTable(GC::Allocator &A, const AST::TableType &TType,
                        Args &&...Values) {
    std::unique_lock Lock(Mutex);
    // Resolve the table's immutable can-hold-managed bit here, at the one call
    // site with both the table type and this module's type list: the heap-type
    // index in TType is defining-module-relative, so TableInstance itself
    // cannot compute it.
    const bool CanHoldManaged = AST::TypeMatcher::refTypeCanHoldGCObject(
        TType.getRefType(), getTypeList());
    unsafeAddInstance(OwnedTabInsts, TabInsts, TType,
                      std::forward<Args>(Values)..., CanHoldManaged);
    // Fallible attach: a freshly created in-module table is unattached, so its
    // first attach to this module's allocator always succeeds; propagate the
    // result so every caller honors the contract.
    return TabInsts.back()->setAllocator(A);
  }
  template <typename... Args> void addMemory(Args &&...Values) {
    std::unique_lock Lock(Mutex);
    unsafeAddInstance(OwnedMemInsts, MemInsts, std::forward<Args>(Values)...);
  }
  template <typename... Args> void addTag(Args &&...Values) {
    std::unique_lock Lock(Mutex);
    unsafeAddInstance(OwnedTagInsts, TagInsts, std::forward<Args>(Values)...);
  }
  template <typename... Args>
  Expect<void> addGlobal(GC::Allocator &A, const AST::GlobalType &GType,
                         Args &&...Values) {
    std::unique_lock Lock(Mutex);
    // Resolve the global's immutable can-hold-managed bit here, mirroring
    // addTable: this is the one site with both the global type and this
    // module's type list, needed to resolve a concrete heap-type index.
    const bool CanHoldManaged = AST::TypeMatcher::refTypeCanHoldGCObject(
        GType.getValType(), getTypeList());
    unsafeAddInstance(OwnedGlobInsts, GlobInsts, GType, CanHoldManaged,
                      std::forward<Args>(Values)...);
    return GlobInsts.back()->setAllocator(A);
  }
  template <typename... Args> void addElem(GC::Allocator &A, Args &&...Values) {
    std::unique_lock Lock(Mutex);
    unsafeAddInstance(OwnedElemInsts, ElemInsts, std::forward<Args>(Values)...);
    ElemInsts.back()->setAllocator(A);
  }
  template <typename... Args> void addData(Args &&...Values) {
    std::unique_lock Lock(Mutex);
    unsafeAddInstance(OwnedDataInsts, DataInsts, std::forward<Args>(Values)...);
  }
  template <typename... Args>
  ExceptionInstance *newException(GC::Allocator &A, Args &&...Values) {
    std::unique_lock Lock(Mutex);
    OwnedExceptionInsts.push_back(
        std::make_unique<ExceptionInstance>(std::forward<Args>(Values)...));
    OwnedExceptionInsts.back()->setAllocator(A);
    return OwnedExceptionInsts.back().get();
  }

  /// Import instances into this module instance.
  void importFunction(FunctionInstance *Func) {
    std::unique_lock Lock(Mutex);
    unsafeImportInstance(FuncInsts, Func);
  }
  void importTable(TableInstance *Tab) {
    std::unique_lock Lock(Mutex);
    unsafeImportInstance(TabInsts, Tab);
  }
  void importMemory(MemoryInstance *Mem) {
    std::unique_lock Lock(Mutex);
    unsafeImportInstance(MemInsts, Mem);
  }
  void importTag(TagInstance *Tg) {
    std::unique_lock Lock(Mutex);
    unsafeImportInstance(TagInsts, Tg);
  }
  void importGlobal(GlobalInstance *Glob) {
    std::unique_lock Lock(Mutex);
    ImpGlobalNum++;
    unsafeImportInstance(GlobInsts, Glob);
  }

  /// Export instances with name from this module instance.
  void exportFunction(std::string_view Name, uint32_t Idx) {
    std::unique_lock Lock(Mutex);
    ExpFuncs.insert_or_assign(std::string(Name), FuncInsts[Idx]);
  }
  void exportTable(std::string_view Name, uint32_t Idx) {
    std::unique_lock Lock(Mutex);
    ExpTables.insert_or_assign(std::string(Name), TabInsts[Idx]);
  }
  void exportMemory(std::string_view Name, uint32_t Idx) {
    std::unique_lock Lock(Mutex);
    ExpMems.insert_or_assign(std::string(Name), MemInsts[Idx]);
  }
  void exportGlobal(std::string_view Name, uint32_t Idx) {
    std::unique_lock Lock(Mutex);
    ExpGlobals.insert_or_assign(std::string(Name), GlobInsts[Idx]);
  }
  void exportTag(std::string_view Name, uint32_t Idx) {
    std::unique_lock Lock(Mutex);
    ExpTags.insert_or_assign(std::string(Name), TagInsts[Idx]);
  }

  /// Get defined type list.
  Span<const AST::SubType *const> getTypeList() const noexcept { return Types; }

  /// Get instance pointer by index.
  Expect<const AST::SubType *> getType(uint32_t Idx) const noexcept {
    std::shared_lock Lock(Mutex);
    if (unlikely(Idx >= Types.size())) {
      // Error logging needs to be handled by the caller.
      return Unexpect(ErrCode::Value::WrongInstanceIndex);
    }
    return unsafeGetType(Idx);
  }
  const AST::SubType *unsafeGetType(uint32_t Idx) const noexcept {
    return Types[Idx];
  }
  Expect<FunctionInstance *> getFunc(uint32_t Idx) const noexcept {
    std::shared_lock Lock(Mutex);
    if (Idx >= FuncInsts.size()) {
      // Error logging needs to be handled by the caller.
      return Unexpect(ErrCode::Value::WrongInstanceIndex);
    }
    return unsafeGetFunction(Idx);
  }
  FunctionInstance *unsafeGetFunction(uint32_t Idx) const noexcept {
    return FuncInsts[Idx];
  }
  Expect<TableInstance *> getTable(uint32_t Idx) const noexcept {
    std::shared_lock Lock(Mutex);
    if (Idx >= TabInsts.size()) {
      // Error logging needs to be handled by the caller.
      return Unexpect(ErrCode::Value::WrongInstanceIndex);
    }
    return unsafeGetTable(Idx);
  }
  TableInstance *unsafeGetTable(uint32_t Idx) const noexcept {
    return TabInsts[Idx];
  }
  Expect<MemoryInstance *> getMemory(uint32_t Idx) const noexcept {
    std::shared_lock Lock(Mutex);
    if (Idx >= MemInsts.size()) {
      // Error logging needs to be handled by the caller.
      return Unexpect(ErrCode::Value::WrongInstanceIndex);
    }
    return unsafeGetMemory(Idx);
  }
  MemoryInstance *unsafeGetMemory(uint32_t Idx) const noexcept {
    return MemInsts[Idx];
  }
  TagInstance *unsafeGetTag(uint32_t Idx) const noexcept {
    return TagInsts[Idx];
  }
  Expect<GlobalInstance *> getGlobal(uint32_t Idx) const noexcept {
    std::shared_lock Lock(Mutex);
    if (Idx >= GlobInsts.size()) {
      // Error logging needs to be handled by the caller.
      return Unexpect(ErrCode::Value::WrongInstanceIndex);
    }
    return unsafeGetGlobal(Idx);
  }
  GlobalInstance *unsafeGetGlobal(uint32_t Idx) const noexcept {
    return GlobInsts[Idx];
  }
  Expect<ElementInstance *> getElem(uint32_t Idx) const noexcept {
    std::shared_lock Lock(Mutex);
    if (Idx >= ElemInsts.size()) {
      // Error logging needs to be handled by the caller.
      return Unexpect(ErrCode::Value::WrongInstanceIndex);
    }
    return unsafeGetElem(Idx);
  }
  ElementInstance *unsafeGetElem(uint32_t Idx) const noexcept {
    return ElemInsts[Idx];
  }
  Expect<DataInstance *> getData(uint32_t Idx) const noexcept {
    std::shared_lock Lock(Mutex);
    if (Idx >= DataInsts.size()) {
      // Error logging needs to be handled by the caller.
      return Unexpect(ErrCode::Value::WrongInstanceIndex);
    }
    return unsafeGetData(Idx);
  }
  DataInstance *unsafeGetData(uint32_t Idx) const noexcept {
    return DataInsts[Idx];
  }

  /// Get the instances count.
  uint32_t getFuncNum() const noexcept {
    std::shared_lock Lock(Mutex);
    return static_cast<uint32_t>(FuncInsts.size());
  }
  uint32_t getTableNum() const noexcept {
    std::shared_lock Lock(Mutex);
    return static_cast<uint32_t>(TabInsts.size());
  }
  uint32_t getMemoryNum() const noexcept {
    std::shared_lock Lock(Mutex);
    return static_cast<uint32_t>(MemInsts.size());
  }
  uint32_t getGlobalNum() const noexcept {
    std::shared_lock Lock(Mutex);
    return static_cast<uint32_t>(GlobInsts.size());
  }

  /// Get imported global instances count.
  uint32_t getGlobalImportNum() const noexcept { return ImpGlobalNum; }

  /// Set the start function index and find the function instance.
  void setStartIdx(uint32_t Idx) noexcept {
    std::unique_lock Lock(Mutex);
    StartFunc = FuncInsts[Idx];
  }

  /// Get start function address in Store.
  const FunctionInstance *getStartFunc() const noexcept {
    std::shared_lock Lock(Mutex);
    return StartFunc;
  }

  /// Set the target imported WASI module during instantiation.
  void setWASIModule(const ModuleInstance *Mod) noexcept {
    std::unique_lock Lock(Mutex);
    WASIModInst = Mod;
  }

  /// Get the target imported WASI module during instantiation.
  const ModuleInstance *getWASIModule() const noexcept {
    std::shared_lock Lock(Mutex);
    return WASIModInst;
  }

  /// Unsafely import an instance into this module. Only valid before
  /// finalization: buildContext() caches the instance vectors' buffers, which a
  /// later push_back may reallocate.
  template <typename T>
  std::enable_if_t<IsEntityV<T>, void>
  unsafeImportInstance(std::vector<T *> &Vec, T *Ptr) {
    assuming(!isInstantiateFinalized());
    Vec.push_back(Ptr);
  }

  /// Unsafely import a defined type from a host function into this module.
  void unsafeImportDefinedType(const AST::SubType &SType) {
    Types.push_back(&SType);
    const_cast<AST::SubType *>(Types.back())
        ->setTypeIndex(static_cast<uint32_t>(Types.size()) - 1);
  }

  /// Unsafely create and add the instance to this module. Only valid before
  /// finalization: buildContext() caches the instance vectors' buffers, which a
  /// later push_back may reallocate.
  template <typename T, typename... Args>
  std::enable_if_t<IsInstanceV<T>, void>
  unsafeAddInstance(std::vector<std::unique_ptr<T>> &OwnedInstsVec,
                    std::vector<T *> &InstsVec, Args &&...Values) {
    assuming(!isInstantiateFinalized());
    OwnedInstsVec.push_back(std::make_unique<T>(std::forward<Args>(Values)...));
    InstsVec.push_back(OwnedInstsVec.back().get());
  }

  /// Unsafely add and export the existing instance to this module.
  template <typename T, typename... Args>
  std::enable_if_t<IsEntityV<T>, void>
  unsafeAddHostInstance(std::string_view Name,
                        std::vector<std::unique_ptr<T>> &OwnedInstsVec,
                        std::vector<T *> &InstsVec,
                        std::map<std::string, T *, std::less<>> &InstsMap,
                        std::unique_ptr<T> &&Inst) {
    OwnedInstsVec.push_back(std::move(Inst));
    InstsVec.push_back(OwnedInstsVec.back().get());
    InstsMap.insert_or_assign(std::string(Name), InstsVec.back());
  }

  /// Unsafely find and get the exported instance by name.
  template <typename T>
  std::enable_if_t<IsEntityV<T>, T *>
  unsafeFindExports(const std::map<std::string, T *, std::less<>> &Map,
                    std::string_view ExtName) const noexcept {
    auto Iter = Map.find(ExtName);
    if (likely(Iter != Map.cend())) {
      return Iter->second;
    }
    return nullptr;
  }

  /// \name Data for compiled functions.
  /// @{
#if WASMEDGE_ALLOCATOR_IS_STABLE
  std::vector<uint8_t *> MemoryPtrs;
#else
  std::vector<uint8_t **> MemoryPtrs;
#endif
  std::vector<const uint64_t *> MemorySizePtrs;
  std::vector<const uint64_t *> TableSizePtrs;
  std::vector<RefVariant **> TableRefPtrs;
  std::vector<ValVariant *> GlobalPtrs;
  /// @}

  struct ModuleContext {
#if WASMEDGE_ALLOCATOR_IS_STABLE
    uint8_t *const *Memories;
#else
    uint8_t **const *Memories;
#endif
    const uint64_t *const *MemorySizes;
    RefVariant **const *TableRefs;
    const uint64_t *const *TableSizes;
    ValVariant *const *Globals;
    const void *ModuleInst;
    void *const *Tags;
  };

  /// Compiled code reads this struct by field index through the mirrored ModCtx
  /// type built in lib/llvm/compiler/context.cpp. Keep both in the same order.
  static_assert(sizeof(ModuleContext) == 7 * sizeof(void *));
  static_assert(offsetof(ModuleContext, Memories) == 0 * sizeof(void *));
  static_assert(offsetof(ModuleContext, MemorySizes) == 1 * sizeof(void *));
  static_assert(offsetof(ModuleContext, TableRefs) == 2 * sizeof(void *));
  static_assert(offsetof(ModuleContext, TableSizes) == 3 * sizeof(void *));
  static_assert(offsetof(ModuleContext, Globals) == 4 * sizeof(void *));
  static_assert(offsetof(ModuleContext, ModuleInst) == 5 * sizeof(void *));
  static_assert(offsetof(ModuleContext, Tags) == 6 * sizeof(void *));

  ModuleContext ModCtx{};

  /// Snapshot the instance vectors' buffers for compiled code. Call once, after
  /// the last instance is added and before the start function runs; adding an
  /// instance afterwards may reallocate and leave the context dangling.
  void buildContext() noexcept {
    ModCtx.Memories = MemoryPtrs.data();
    ModCtx.MemorySizes = MemorySizePtrs.data();
    ModCtx.TableRefs = TableRefPtrs.data();
    ModCtx.TableSizes = TableSizePtrs.data();
    ModCtx.Globals = GlobalPtrs.data();
    ModCtx.ModuleInst = this;
    ModCtx.Tags = reinterpret_cast<void *const *>(TagInsts.data());
  }

  friend class Runtime::StoreManager;
  using LinkedStoreKey = std::pair<StoreManager *, std::string>;
  using BeforeModuleDestroyCallback = void(const LinkedStoreKey &Key,
                                           const ModuleInstance *Mod);
  void linkStore(StoreManager *Store, std::string_view Name,
                 BeforeModuleDestroyCallback Callback) {
    // Link to the store during registration.
    std::unique_lock Lock(Mutex);
    LinkedStore.insert_or_assign(LinkedStoreKey{Store, std::string(Name)},
                                 Callback);
  }

  void unlinkStore(StoreManager *Store, std::string_view Name) {
    // Unlink a specific (Store, Name) entry.
    std::unique_lock Lock(Mutex);
    LinkedStore.erase(LinkedStoreKey{Store, std::string(Name)});
  }

  void unlinkAllStores() noexcept {
    std::map<LinkedStoreKey, std::function<BeforeModuleDestroyCallback>>
        Snapshot;
    {
      std::unique_lock Lock(Mutex);
      Snapshot.swap(LinkedStore);
    }
    for (auto &&[Key, Callback] : Snapshot) {
      assuming(Callback);
      Callback(Key, this);
    }
  }

  void addDependency(ModuleInstance &Provider) {
    std::unique_lock Lock(Mutex);
    if (Providers.insert(&Provider).second) {
      Provider.Life.addDependent();
    }
  }

  void takeProviders(std::unordered_set<ModuleInstance *> &Snapshot) noexcept {
    std::unique_lock Lock(Mutex);
    Snapshot.swap(Providers);
  }

  static void drainPins(std::unordered_set<ModuleInstance *> &Provs,
                        std::vector<ModuleInstance *> &ToDelete) noexcept {
    for (auto *Provider : Provs) {
      if (Provider->Life.releaseDependent()) {
        ToDelete.push_back(Provider);
      }
    }
    Provs.clear();
  }

  void releaseProviders() noexcept {
    std::unordered_set<ModuleInstance *> ProvidersToRelease;
    takeProviders(ProvidersToRelease);
    if (ProvidersToRelease.empty()) {
      return;
    }
    std::vector<ModuleInstance *> ToDelete;
    drainPins(ProvidersToRelease, ToDelete);
    while (!ToDelete.empty()) {
      ModuleInstance *Next = ToDelete.back();
      ToDelete.pop_back();
      std::unordered_set<ModuleInstance *> NextProviders;
      Next->takeProviders(NextProviders);
      delete Next;
      drainPins(NextProviders, ToDelete);
    }
  }

  /// Mutex.
  mutable std::shared_mutex Mutex;

  /// Module name.
  const std::string ModName;

  /// Defined types.
  std::vector<const AST::SubType *> Types;
  std::vector<std::unique_ptr<const AST::SubType>> OwnedTypes;

  /// Owned instances in this module.
  std::vector<std::unique_ptr<FunctionInstance>> OwnedFuncInsts;
  std::vector<std::unique_ptr<TableInstance>> OwnedTabInsts;
  std::vector<std::unique_ptr<MemoryInstance>> OwnedMemInsts;
  std::vector<std::unique_ptr<TagInstance>> OwnedTagInsts;
  std::vector<std::unique_ptr<GlobalInstance>> OwnedGlobInsts;
  std::vector<std::unique_ptr<ElementInstance>> OwnedElemInsts;
  std::vector<std::unique_ptr<DataInstance>> OwnedDataInsts;
  std::vector<std::unique_ptr<ExceptionInstance>> OwnedExceptionInsts;

  /// Imported and added instances in this module.
  std::vector<FunctionInstance *> FuncInsts;
  std::vector<TableInstance *> TabInsts;
  std::vector<MemoryInstance *> MemInsts;
  std::vector<TagInstance *> TagInsts;
  std::vector<GlobalInstance *> GlobInsts;
  std::vector<ElementInstance *> ElemInsts;
  std::vector<DataInstance *> DataInsts;

  /// Imported instance counts.
  uint32_t ImpGlobalNum = 0;

  /// Exported name maps.
  std::map<std::string, FunctionInstance *, std::less<>> ExpFuncs;
  std::map<std::string, TableInstance *, std::less<>> ExpTables;
  std::map<std::string, MemoryInstance *, std::less<>> ExpMems;
  std::map<std::string, TagInstance *, std::less<>> ExpTags;
  std::map<std::string, GlobalInstance *, std::less<>> ExpGlobals;

  /// Start function instance.
  const FunctionInstance *StartFunc = nullptr;

  /// Imported WASI module instance during instantiation.
  const ModuleInstance *WASIModInst = nullptr;

  /// Linked store. Key is (StoreManager*, RegisteredName) to support
  /// the same module registered under multiple alias names.
  std::map<LinkedStoreKey, std::function<BeforeModuleDestroyCallback>>
      LinkedStore;

  /// External data and its finalizer function pointer.
  void *HostData;
  std::function<void(void *)> HostDataFinalizer;

  /// Whether this module instance has been finalized (immutable). Once set,
  /// the indexed instances will not be mutated anymore.
  mutable std::atomic<bool> InstantiateFinalized{false};

  /// Intrusive lifetime: owner flag plus importer count, packed into one
  /// atomic. Mutable so a lifetime pin (pin()/unpin(), addDependency) can be
  /// taken through a `const ModuleInstance *` without implying logical
  /// mutation.
  mutable RefLifetime Life;
  /// See setDeferrableStorage(). Atomic: set on the (single-threaded)
  /// creation path but read by any thread launching an async invocation.
  std::atomic<bool> DeferrableStorage{false};
  /// See setGCCompiled(). Atomic: stamped once at instantiation but read on
  /// every compiled call, potentially from another executor's threads.
  std::atomic<bool> GCCompiled{true};
  /// Provider instances this module imported from; each holds one dependent
  /// pin, released when this module is destroyed.
  std::unordered_set<ModuleInstance *> Providers;
};

/// Move-only RAII pin on a module instance's dependency lifetime. Holds one
/// pin() across its lifetime and releases it on destruction, deleting the
/// module if that release was the deferred owner's last dependent. A null
/// target (independent host function instance) is inert. Used to keep an
/// externally-owned module carrying a FunctionInstance alive across a detached
/// async invocation whose worker outlives the launching call.
class ModulePin {
public:
  ModulePin() noexcept = default;
  explicit ModulePin(const ModuleInstance *Mod) noexcept : Inst(Mod) {
    if (Inst != nullptr) {
      Inst->pin();
    }
  }
  ModulePin(const ModulePin &) = delete;
  ModulePin &operator=(const ModulePin &) = delete;
  ModulePin(ModulePin &&Other) noexcept : Inst(Other.Inst) {
    Other.Inst = nullptr;
  }
  ModulePin &operator=(ModulePin &&Other) noexcept {
    if (this != &Other) {
      release();
      Inst = Other.Inst;
      Other.Inst = nullptr;
    }
    return *this;
  }
  ~ModulePin() noexcept { release(); }

private:
  void release() noexcept {
    if (Inst != nullptr) {
      const ModuleInstance *Doomed = Inst;
      Inst = nullptr;
      if (Doomed->unpin()) {
        delete Doomed;
      }
    }
  }

  const ModuleInstance *Inst = nullptr;
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
