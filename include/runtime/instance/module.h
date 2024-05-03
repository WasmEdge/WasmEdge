// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

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

#include "ast/component/type.h"
#include "ast/module.h"
#include "ast/type.h"
#include "common/errcode.h"
#include "runtime/hostfunc.h"
#include "runtime/instance/array.h"
#include "runtime/instance/data.h"
#include "runtime/instance/elem.h"
#include "runtime/instance/function.h"
#include "runtime/instance/global.h"
#include "runtime/instance/memory.h"
#include "runtime/instance/struct.h"
#include "runtime/instance/table.h"
#include "runtime/instance/tag.h"

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

namespace Executor {
class Executor;
}

namespace Runtime {

class StoreManager;
class CallingFrame;

namespace Instance {

namespace {
/// Return true if T is an entity which can be exported or imported.
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

class ModuleInstance {
public:
  ModuleInstance(std::string_view Name, void *Data = nullptr,
                 std::function<void(void *)> Finalizer = nullptr)
      : ModName(Name), HostData(Data), HostDataFinalizer(Finalizer) {}
  virtual ~ModuleInstance() noexcept {
    // When destroying this module instance, call the callbacks to unlink to the
    // store managers.
    for (auto &&Pair : LinkedStore) {
      assuming(Pair.second);
      Pair.second(Pair.first, this);
    }
    if (HostDataFinalizer.operator bool()) {
      HostDataFinalizer(HostData);
    }
  }

  std::string_view getModuleName() const noexcept {
    std::shared_lock Lock(Mutex);
    return ModName;
  }

  void *getHostData() const noexcept { return HostData; }

  /// Add exist instances and move ownership with exporting name.
  void addHostFunc(std::string_view Name,
                   std::unique_ptr<HostFunctionBase> &&Func) {
    std::unique_lock Lock(Mutex);
    unsafeImportDefinedType(Func->getDefinedType());
    unsafeAddHostInstance(
        Name, OwnedFuncInsts, FuncInsts, ExpFuncs,
        std::make_unique<FunctionInstance>(
            this, static_cast<uint32_t>(Types.size()) - 1, std::move(Func)));
  }
  void addHostFunc(std::string_view Name,
                   std::unique_ptr<FunctionInstance> &&Func) {
    std::unique_lock Lock(Mutex);
    assuming(Func->isHostFunction());
    unsafeImportDefinedType(Func->getHostFunc().getDefinedType());
    Func->linkDefinedType(this, static_cast<uint32_t>(Types.size()) - 1);
    unsafeAddHostInstance(Name, OwnedFuncInsts, FuncInsts, ExpFuncs,
                          std::move(Func));
  }

  void addHostTable(std::string_view Name,
                    std::unique_ptr<TableInstance> &&Tab) {
    std::unique_lock Lock(Mutex);
    unsafeAddHostInstance(Name, OwnedTabInsts, TabInsts, ExpTables,
                          std::move(Tab));
  }
  void addHostMemory(std::string_view Name,
                     std::unique_ptr<MemoryInstance> &&Mem) {
    std::unique_lock Lock(Mutex);
    unsafeAddHostInstance(Name, OwnedMemInsts, MemInsts, ExpMems,
                          std::move(Mem));
  }
  void addHostGlobal(std::string_view Name,
                     std::unique_ptr<GlobalInstance> &&Glob) {
    std::unique_lock Lock(Mutex);
    unsafeAddHostInstance(Name, OwnedGlobInsts, GlobInsts, ExpGlobals,
                          std::move(Glob));
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

  /// Component model concepts
  ///
  /// Export functions with name, these functions are suppose not owned by this
  /// module, because the module is just a wrapper for component functions.
  ///
  /// See the example below, with statement below shows why we need this kind of
  /// exporting
  ///
  /// (component
  ///   (core module $A
  ///     (func (export "one") (result i32) (i32.const 1))
  ///   )
  ///   (core module $B
  ///     (func (import "a" "one") (result i32))
  ///   )
  ///   (core instance $a (instantiate $A))
  ///   (core instance $b (instantiate $B (with "a" (instance $a))))
  /// )
  void exportFunction(std::string_view Name, FunctionInstance *Func) {
    std::unique_lock Lock(Mutex);
    assuming(Func->isHostFunction());
    unsafeImportDefinedType(Func->getHostFunc().getDefinedType());
    Func->linkDefinedType(this, static_cast<uint32_t>(Types.size()) - 1);
    FuncInsts.push_back(Func);
    ExpFuncs.insert_or_assign(std::string(Name), FuncInsts.back());
  }
  void exportTable(std::string_view Name, TableInstance *Tab) {
    std::unique_lock Lock(Mutex);
    TabInsts.push_back(Tab);
    ExpTables.insert_or_assign(std::string(Name), TabInsts.back());
  }
  void exportMemory(std::string_view Name, MemoryInstance *Mem) {
    std::unique_lock Lock(Mutex);
    MemInsts.push_back(Mem);
    ExpMems.insert_or_assign(std::string(Name), MemInsts.back());
  }
  void exportGlobal(std::string_view Name, GlobalInstance *Glob) {
    std::unique_lock Lock(Mutex);
    GlobInsts.push_back(Glob);
    ExpGlobals.insert_or_assign(std::string(Name), GlobInsts.back());
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

  /// Create and add instances into this module instance.
  template <typename... Args> void addFunc(Args &&...Values) {
    std::unique_lock Lock(Mutex);
    unsafeAddInstance(OwnedFuncInsts, FuncInsts, this,
                      std::forward<Args>(Values)...);
  }
  template <typename... Args> void addTable(Args &&...Values) {
    std::unique_lock Lock(Mutex);
    unsafeAddInstance(OwnedTabInsts, TabInsts, std::forward<Args>(Values)...);
  }
  template <typename... Args> void addMemory(Args &&...Values) {
    std::unique_lock Lock(Mutex);
    unsafeAddInstance(OwnedMemInsts, MemInsts, std::forward<Args>(Values)...);
  }
  template <typename... Args> void addTag(Args &&...Values) {
    std::unique_lock Lock(Mutex);
    unsafeAddInstance(OwnedTagInsts, TagInsts, std::forward<Args>(Values)...);
  }
  template <typename... Args> void addGlobal(Args &&...Values) {
    std::unique_lock Lock(Mutex);
    unsafeAddInstance(OwnedGlobInsts, GlobInsts, std::forward<Args>(Values)...);
  }
  template <typename... Args> void addElem(Args &&...Values) {
    std::unique_lock Lock(Mutex);
    unsafeAddInstance(OwnedElemInsts, ElemInsts, std::forward<Args>(Values)...);
  }
  template <typename... Args> void addData(Args &&...Values) {
    std::unique_lock Lock(Mutex);
    unsafeAddInstance(OwnedDataInsts, DataInsts, std::forward<Args>(Values)...);
  }
  template <typename... Args> ArrayInstance *newArray(Args &&...Values) {
    std::unique_lock Lock(Mutex);
    OwnedArrayInsts.push_back(
        std::make_unique<ArrayInstance>(this, std::forward<Args>(Values)...));
    return OwnedArrayInsts.back().get();
  }
  template <typename... Args> StructInstance *newStruct(Args &&...Values) {
    std::unique_lock Lock(Mutex);
    OwnedStructInsts.push_back(
        std::make_unique<StructInstance>(this, std::forward<Args>(Values)...));
    return OwnedStructInsts.back().get();
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
      // Error logging need to be handled in caller.
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
      // Error logging need to be handled in caller.
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
      // Error logging need to be handled in caller.
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
      // Error logging need to be handled in caller.
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
      // Error logging need to be handled in caller.
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
      // Error logging need to be handled in caller.
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
      // Error logging need to be handled in caller.
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
  FunctionInstance *getStartFunc() const noexcept {
    std::shared_lock Lock(Mutex);
    return StartFunc;
  }

  /// Unsafe import instance into this module.
  template <typename T>
  std::enable_if_t<IsEntityV<T>, void>
  unsafeImportInstance(std::vector<T *> &Vec, T *Ptr) {
    Vec.push_back(Ptr);
  }

  /// Unsafe import defined type from host function into this module.
  void unsafeImportDefinedType(const AST::SubType &SType) {
    Types.push_back(&SType);
    const_cast<AST::SubType *>(Types.back())
        ->setTypeIndex(static_cast<uint32_t>(Types.size()) - 1);
  }

  /// Unsafe create and add the instance into this module.
  template <typename T, typename... Args>
  std::enable_if_t<IsInstanceV<T>, void>
  unsafeAddInstance(std::vector<std::unique_ptr<T>> &OwnedInstsVec,
                    std::vector<T *> &InstsVec, Args &&...Values) {
    OwnedInstsVec.push_back(std::make_unique<T>(std::forward<Args>(Values)...));
    InstsVec.push_back(OwnedInstsVec.back().get());
  }

  /// Unsafe add and export the existing instance into this module.
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

  /// Unsafe find and get the exported instance by name.
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
  std::vector<uint8_t *> MemoryPtrs;
  std::vector<ValVariant *> GlobalPtrs;
  /// @}

  friend class Runtime::StoreManager;
  using BeforeModuleDestroyCallback = void(StoreManager *Store,
                                           const ModuleInstance *Mod);
  void linkStore(StoreManager *Store, BeforeModuleDestroyCallback Callback) {
    // Link to store when registration.
    std::unique_lock Lock(Mutex);
    LinkedStore.insert_or_assign(Store, Callback);
  }

  void unlinkStore(StoreManager *Store) {
    // Unlink to store. Call by the store manager when the store manager being
    // destroyed before this module instance.
    std::unique_lock Lock(Mutex);
    LinkedStore.erase(Store);
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
  std::vector<std::unique_ptr<ArrayInstance>> OwnedArrayInsts;
  std::vector<std::unique_ptr<StructInstance>> OwnedStructInsts;

  /// Imported and added instances in this module.
  std::vector<FunctionInstance *> FuncInsts;
  std::vector<TableInstance *> TabInsts;
  std::vector<MemoryInstance *> MemInsts;
  std::vector<TagInstance *> TagInsts;
  std::vector<GlobalInstance *> GlobInsts;
  std::vector<ElementInstance *> ElemInsts;
  std::vector<DataInstance *> DataInsts;

  /// Imported instances counts.
  uint32_t ImpGlobalNum = 0;

  /// Exported name maps.
  std::map<std::string, FunctionInstance *, std::less<>> ExpFuncs;
  std::map<std::string, TableInstance *, std::less<>> ExpTables;
  std::map<std::string, MemoryInstance *, std::less<>> ExpMems;
  std::map<std::string, TagInstance *, std::less<>> ExpTags;
  std::map<std::string, GlobalInstance *, std::less<>> ExpGlobals;

  /// Start function instance.
  FunctionInstance *StartFunc = nullptr;

  /// Linked store.
  std::map<StoreManager *, std::function<BeforeModuleDestroyCallback>>
      LinkedStore;

  /// External data and its finalizer function pointer.
  void *HostData;
  std::function<void(void *)> HostDataFinalizer;
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
