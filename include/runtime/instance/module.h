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
#include "runtime/typetable.h"

#include <atomic>
#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
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

namespace Host {
class FileSystem;
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

  /// Mark this module instance finalized (immutable). Idempotent, thread-safe.
  void finalizeInstantiation() const noexcept {
    if (InstantiateFinalized.load(std::memory_order_acquire)) {
      return;
    }
    std::unique_lock Lock(Mutex);
    InstantiateFinalized.store(true, std::memory_order_release);
  }

  /// The error of the first host function that could not be added, if any.
  /// A host module with such an error lacks an export that it declares.
  std::optional<ErrCode> getHostAddError() const noexcept {
    std::shared_lock Lock(Mutex);
    return HostAddError;
  }
  /// Record that a host function of this module could not be made, so that
  /// the module counts as lacking an export.
  void recordHostAddError(ErrCode Code) noexcept {
    std::unique_lock Lock(Mutex);
    static_cast<void>(unsafeRecordHostAddError(Code));
  }

  /// Return true if this module instance has been finalized (immutable).
  bool isInstantiateFinalized() const noexcept {
    return InstantiateFinalized.load(std::memory_order_acquire);
  }

  std::string_view getModuleName() const noexcept {
    std::shared_lock Lock(Mutex);
    return ModName;
  }

  /// Get the canonical type id of a defined type.
  uint32_t getCanonicalTypeId(uint32_t Idx) const noexcept {
    assuming(Idx < CanonTypeIds.size());
    return CanonTypeIds[Idx];
  }

  void *getHostData() const noexcept { return HostData; }

  /// Get the defined type by index.
  Expect<const AST::SubType *> getType(uint32_t Idx) const noexcept {
    std::shared_lock Lock(Mutex);
    if (unlikely(Idx >= Types.size())) {
      // Error logging needs to be handled by the caller.
      return Unexpect(ErrCode::Value::WrongInstanceIndex);
    }
    return unsafeGetType(Idx);
  }

  /// Get defined type list.
  Span<const AST::SubType *const> getTypeList() const noexcept { return Types; }

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

  Span<MemoryInstance *const> getMemoryInstances() noexcept { return MemInsts; }

  Span<const GlobalInstance *const> getGlobalInstances() const noexcept {
    return Span<const GlobalInstance *const>(
        const_cast<const GlobalInstance *const *>(GlobInsts.data()),
        GlobInsts.size());
  }

  Span<GlobalInstance *const> getGlobalInstances() noexcept {
    return GlobInsts;
  }

  Span<const TableInstance *const> getTableInstances() const noexcept {
    return Span<const TableInstance *const>(
        const_cast<const TableInstance *const *>(TabInsts.data()),
        TabInsts.size());
  }

  Span<TableInstance *const> getTableInstances() noexcept { return TabInsts; }

  Span<const ElementInstance *const> getElemInstances() const noexcept {
    return Span<const ElementInstance *const>(
        const_cast<const ElementInstance *const *>(ElemInsts.data()),
        ElemInsts.size());
  }

  Span<ElementInstance *const> getElemInstances() noexcept { return ElemInsts; }

  Span<const DataInstance *const> getOwnedDataInstances() const noexcept {
    return Span<const DataInstance *const>(
        const_cast<const DataInstance *const *>(DataInsts.data()),
        DataInsts.size());
  }

  Span<DataInstance *const> getOwnedDataInstances() noexcept {
    return DataInsts;
  }

  /// Check whether this module instance owns the instance.
  bool owns(const TableInstance *Instance) const noexcept {
    return std::any_of(
        OwnedTabInsts.begin(), OwnedTabInsts.end(),
        [Instance](const auto &Owned) { return Owned.get() == Instance; });
  }
  bool owns(const GlobalInstance *Instance) const noexcept {
    return std::any_of(
        OwnedGlobInsts.begin(), OwnedGlobInsts.end(),
        [Instance](const auto &Owned) { return Owned.get() == Instance; });
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
    if (auto Res = unsafeImportDefinedType(Func->getDefinedType());
        unlikely(!Res)) {
      return unsafeRecordHostAddError(Res.error());
    }
    auto Inst = FunctionInstance::create(
        this, static_cast<uint32_t>(Types.size()) - 1, std::move(Func));
    if (unlikely(!Inst)) {
      Types.pop_back();
      CanonTypeIds.pop_back();
      return unsafeRecordHostAddError(Inst.error());
    }
    unsafeAddHostInstance(Name, OwnedFuncInsts, FuncInsts, ExpFuncs,
                          std::move(*Inst));
    return {};
  }
  Expect<void> addHostFunc(std::string_view Name,
                           UniqueHeapPtr<FunctionInstance> &&Func) {
    std::unique_lock Lock(Mutex);
    if (unlikely(InstantiateFinalized.load(std::memory_order_acquire))) {
      return Unexpect(ErrCode::Value::WrongVMWorkflow);
    }
    assuming(Func->isHostFunction());
    if (auto Res =
            unsafeImportDefinedType(Func->getHostFunc().getDefinedType());
        unlikely(!Res)) {
      return unsafeRecordHostAddError(Res.error());
    }
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
  Expect<void> addDefinedType(const AST::SubType &SType) {
    std::unique_lock Lock(Mutex);
    OwnedTypes.push_back(std::make_unique<AST::SubType>(SType));
    Types.push_back(OwnedTypes.back().get());
    const auto Info = SType.getRecursiveInfo();
    if (Info.Index + 1 < Info.RecTypeSize) {
      return {};
    }
    const auto Start = static_cast<uint32_t>(Types.size()) - Info.RecTypeSize;
    assuming(CanonTypeIds.size() == Start);
    EXPECTED_TRY(auto First,
                 CanonicalTypeTable::getGlobal().registerRecGroup(
                     Span<const AST::SubType *const>(Types).subspan(Start),
                     Start, CanonTypeIds));
    for (uint32_t I = 0; I < Info.RecTypeSize; ++I) {
      CanonTypeIds.push_back(First + I);
    }
    return {};
  }

  /// Create and add instances to this module instance.
  template <typename... Args> Expect<void> addFunc(Args &&...Values) {
    std::unique_lock Lock(Mutex);
    assuming(!isInstantiateFinalized());
    EXPECTED_TRY(auto Inst,
                 FunctionInstance::create(this, std::forward<Args>(Values)...));
    OwnedFuncInsts.push_back(std::move(Inst));
    FuncInsts.push_back(OwnedFuncInsts.back().get());
    return {};
  }
  template <typename... Args> void addTable(Args &&...Values) {
    std::unique_lock Lock(Mutex);
    unsafeAddInstance(OwnedTabInsts, TabInsts, this,
                      std::forward<Args>(Values)...);
  }
  template <typename... Args> void addMemory(Args &&...Values) {
    std::unique_lock Lock(Mutex);
    unsafeAddInstance(OwnedMemInsts, MemInsts, std::forward<Args>(Values)...);
  }
  template <typename... Args> void addTag(Args &&...Values) {
    std::unique_lock Lock(Mutex);
    unsafeAddInstance(OwnedTagInsts, TagInsts, this,
                      std::forward<Args>(Values)...);
  }
  template <typename... Args> void addGlobal(Args &&...Values) {
    std::unique_lock Lock(Mutex);
    unsafeAddInstance(OwnedGlobInsts, GlobInsts, this,
                      std::forward<Args>(Values)...);
  }
  template <typename... Args> void addElem(Args &&...Values) {
    std::unique_lock Lock(Mutex);
    unsafeAddInstance(OwnedElemInsts, ElemInsts, std::forward<Args>(Values)...);
  }
  template <typename... Args> void addData(Args &&...Values) {
    std::unique_lock Lock(Mutex);
    unsafeAddInstance(OwnedDataInsts, DataInsts, std::forward<Args>(Values)...);
  }
  /// Make an array or a struct data slot that this module owns, and return
  /// a checked view of it.
  Expect<RefHeap::DataView> newArray(uint32_t TypeIdx, uint32_t Length,
                                     uint32_t ElemSize) {
    return newData(HeapKind::Array, TypeIdx, Length,
                   static_cast<uint64_t>(Length) * ElemSize);
  }
  Expect<RefHeap::DataView> newStruct(uint32_t TypeIdx, uint64_t FieldBytes) {
    return newData(HeapKind::Struct, TypeIdx, 0, FieldBytes);
  }
  /// Make an exception instance that this module owns. Fails with
  /// RefHeapFull if the instance or its owner entry cannot be allocated.
  Expect<ExceptionInstance *> newException(TagInstance *Tag,
                                           std::vector<ValVariant> &&Payload);

  /// Import instances into this module instance.
  void importFunction(FunctionInstance *Func) {
    std::unique_lock Lock(Mutex);
    unsafeImportInstance(FuncInsts, Func);
  }
  /// Import a host function (FunctionInstance owned elsewhere). Registers
  /// the host function's defined type into this module's Types list so that
  /// downstream matchType() walks find it, and re-links the FunctionInstance
  /// to point at the new TypeIdx. Use this when a host function is bridged
  /// into a different module than the one that originally created it (e.g.,
  /// the inline-instance machinery in component instantiation).
  Expect<void> importHostFunction(FunctionInstance *Func) {
    std::unique_lock Lock(Mutex);
    assuming(Func->isHostFunction());
    if (auto Res =
            unsafeImportDefinedType(Func->getHostFunc().getDefinedType());
        unlikely(!Res)) {
      return unsafeRecordHostAddError(Res.error());
    }
    Func->linkDefinedType(this, static_cast<uint32_t>(Types.size()) - 1);
    unsafeImportInstance(FuncInsts, Func);
    return {};
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

  /// Get instance pointer by index.
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

  /// Set the filesystem which this module provides as a WASI module.
  void setFileSystem(const Host::FileSystem *FS) noexcept {
    std::unique_lock Lock(Mutex);
    FileSys = FS;
  }

  /// Get the filesystem which this module provides as a WASI module. Null for
  /// the other modules.
  const Host::FileSystem *getFileSystem() const noexcept {
    std::shared_lock Lock(Mutex);
    return FileSys;
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

  /// Keep `Code` if it is the first host function error, and return it.
  Unexpected<ErrCode> unsafeRecordHostAddError(ErrCode Code) noexcept {
    if (!HostAddError) {
      HostAddError = Code;
    }
    return Unexpect(Code);
  }

  /// Unsafely import a defined type from a host function into this module.
  Expect<void> unsafeImportDefinedType(const AST::SubType &SType) {
    const auto Idx = static_cast<uint32_t>(Types.size());
    assuming(CanonTypeIds.size() == Idx);
    const AST::SubType *Group[] = {&SType};
    EXPECTED_TRY(auto Id, CanonicalTypeTable::getGlobal().registerRecGroup(
                              Group, Idx, CanonTypeIds));
    Types.push_back(&SType);
    const_cast<AST::SubType *>(Types.back())->setTypeIndex(Idx);
    CanonTypeIds.push_back(Id);
    return {};
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

  /// Make a data slot of the defined type `TypeIdx` that this module owns,
  /// and return a checked view of it. Fails with RefHeapFull if the slot or
  /// its owner entry cannot be allocated.
  Expect<RefHeap::DataView> newData(HeapKind Kind, uint32_t TypeIdx,
                                    uint32_t Length, uint64_t FieldBytes);

  /// Unsafely add and export the existing instance to this module.
  template <typename T, typename OwnerT>
  std::enable_if_t<IsEntityV<T>, void> unsafeAddHostInstance(
      std::string_view Name, std::vector<OwnerT> &OwnedInstsVec,
      std::vector<T *> &InstsVec,
      std::map<std::string, T *, std::less<>> &InstsMap, OwnerT &&Inst) {
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
  /// the last instance is added and before the segments are initialized; adding
  /// an instance afterwards may reallocate and leave the context dangling.
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
  std::vector<uint32_t> CanonTypeIds;

  /// Owned instances in this module.
  std::vector<UniqueHeapPtr<FunctionInstance>> OwnedFuncInsts;
  std::vector<std::unique_ptr<TableInstance>> OwnedTabInsts;
  std::vector<std::unique_ptr<MemoryInstance>> OwnedMemInsts;
  std::vector<std::unique_ptr<TagInstance>> OwnedTagInsts;
  std::vector<std::unique_ptr<GlobalInstance>> OwnedGlobInsts;
  std::vector<std::unique_ptr<ElementInstance>> OwnedElemInsts;
  std::vector<std::unique_ptr<DataInstance>> OwnedDataInsts;
  std::vector<UniqueDataSlot> OwnedDataSlots;
  std::vector<UniqueHeapPtr<ExceptionInstance>> OwnedExceptionInsts;

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

  /// Filesystem provided by this module as a WASI module.
  const Host::FileSystem *FileSys = nullptr;

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
  /// The first error of `addHostFunc`.
  std::optional<ErrCode> HostAddError;

  /// Intrusive lifetime: owner flag plus importer count, packed into one
  /// atomic.
  RefLifetime Life;
  /// Provider instances this module imported from; each holds one dependent
  /// pin, released when this module is destroyed.
  std::unordered_set<ModuleInstance *> Providers;
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
