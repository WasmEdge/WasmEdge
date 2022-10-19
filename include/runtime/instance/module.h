// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

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

#include "ast/type.h"
#include "common/errcode.h"
#include "runtime/hostfunc.h"
#include "runtime/instance/data.h"
#include "runtime/instance/elem.h"
#include "runtime/instance/function.h"
#include "runtime/instance/global.h"
#include "runtime/instance/memory.h"
#include "runtime/instance/table.h"

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
    std::is_same_v<T, Instance::GlobalInstance>;

/// Return true if T is an instance.
template <typename T>
inline constexpr const bool IsInstanceV =
    IsEntityV<T> || std::is_same_v<T, Instance::ElementInstance> ||
    std::is_same_v<T, Instance::DataInstance>;
} // namespace

class ModuleInstance {
public:
  ModuleInstance(std::string_view Name) : ModName(Name) {}
  virtual ~ModuleInstance() noexcept {
    // When destroying this module instance, call the callbacks to unlink to the
    // store managers.
    for (auto &&Pair : LinkedStore) {
      assuming(Pair.second);
      Pair.second(Pair.first, this);
    }
  }

  std::string_view getModuleName() const noexcept {
    std::shared_lock Lock(Mutex);
    return ModName;
  }

  /// Add exist instances and move ownership with exporting name.
  void addHostFunc(std::string_view Name,
                   std::unique_ptr<HostFunctionBase> &&Func) {
    std::unique_lock Lock(Mutex);
    unsafeAddHostInstance(Name, OwnedFuncInsts, FuncInsts, ExpFuncs,
                          std::make_unique<Runtime::Instance::FunctionInstance>(
                              this, std::move(Func)));
  }
  void addHostFunc(std::string_view Name,
                   std::unique_ptr<Instance::FunctionInstance> &&Func) {
    std::unique_lock Lock(Mutex);
    Func->setModule(this);
    unsafeAddHostInstance(Name, OwnedFuncInsts, FuncInsts, ExpFuncs,
                          std::move(Func));
  }
  void addHostTable(std::string_view Name,
                    std::unique_ptr<Instance::TableInstance> &&Tab) {
    std::unique_lock Lock(Mutex);
    unsafeAddHostInstance(Name, OwnedTabInsts, TabInsts, ExpTables,
                          std::move(Tab));
  }
  void addHostMemory(std::string_view Name,
                     std::unique_ptr<Instance::MemoryInstance> &&Mem) {
    std::unique_lock Lock(Mutex);
    unsafeAddHostInstance(Name, OwnedMemInsts, MemInsts, ExpMems,
                          std::move(Mem));
  }
  void addHostGlobal(std::string_view Name,
                     std::unique_ptr<Instance::GlobalInstance> &&Glob) {
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
  auto getGlobalExports(CallbackT &&CallBack) const noexcept {
    std::shared_lock Lock(Mutex);
    return std::forward<CallbackT>(CallBack)(ExpGlobals);
  }

protected:
  friend class Executor::Executor;
  friend class Runtime::CallingFrame;

  /// Copy the function types in type section to this module instance.
  void addFuncType(const AST::FunctionType &FuncType) {
    std::unique_lock Lock(Mutex);
    FuncTypes.emplace_back(FuncType);
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

  /// Get function type by index.
  Expect<const AST::FunctionType *> getFuncType(uint32_t Idx) const noexcept {
    std::shared_lock Lock(Mutex);
    if (unlikely(Idx >= FuncTypes.size())) {
      // Error logging need to be handled in caller.
      return Unexpect(ErrCode::Value::WrongInstanceIndex);
    }
    return &FuncTypes[Idx];
  }

  /// Get instance pointer by index.
  Expect<FunctionInstance *> getFunc(uint32_t Idx) const noexcept {
    std::shared_lock Lock(Mutex);
    if (Idx >= FuncInsts.size()) {
      // Error logging need to be handled in caller.
      return Unexpect(ErrCode::Value::WrongInstanceIndex);
    }
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

  /// Add the instances under the module by pointers.
  template <typename T>
  std::enable_if_t<IsEntityV<T>, void>
  unsafeImportInstance(std::vector<T *> &Vec, T *Ptr) {
    Vec.push_back(Ptr);
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

  /// Function types.
  std::vector<AST::FunctionType> FuncTypes;

  /// Owned instances in this module.
  std::vector<std::unique_ptr<Instance::FunctionInstance>> OwnedFuncInsts;
  std::vector<std::unique_ptr<Instance::TableInstance>> OwnedTabInsts;
  std::vector<std::unique_ptr<Instance::MemoryInstance>> OwnedMemInsts;
  std::vector<std::unique_ptr<Instance::GlobalInstance>> OwnedGlobInsts;
  std::vector<std::unique_ptr<Instance::ElementInstance>> OwnedElemInsts;
  std::vector<std::unique_ptr<Instance::DataInstance>> OwnedDataInsts;

  /// Imported and added instances in this module.
  std::vector<FunctionInstance *> FuncInsts;
  std::vector<TableInstance *> TabInsts;
  std::vector<MemoryInstance *> MemInsts;
  std::vector<GlobalInstance *> GlobInsts;
  std::vector<ElementInstance *> ElemInsts;
  std::vector<DataInstance *> DataInsts;

  /// Imported instances counts.
  uint32_t ImpGlobalNum = 0;

  /// Exported name maps.
  std::map<std::string, FunctionInstance *, std::less<>> ExpFuncs;
  std::map<std::string, TableInstance *, std::less<>> ExpTables;
  std::map<std::string, MemoryInstance *, std::less<>> ExpMems;
  std::map<std::string, GlobalInstance *, std::less<>> ExpGlobals;

  /// Start function instance.
  FunctionInstance *StartFunc = nullptr;

  /// Linked store.
  std::map<StoreManager *, std::function<BeforeModuleDestroyCallback>>
      LinkedStore;
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
