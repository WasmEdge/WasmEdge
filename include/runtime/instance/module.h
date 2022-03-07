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
#include "runtime/instance/data.h"
#include "runtime/instance/elem.h"
#include "runtime/instance/function.h"
#include "runtime/instance/global.h"
#include "runtime/instance/memory.h"
#include "runtime/instance/table.h"

#include <atomic>
#include <map>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <vector>

namespace WasmEdge {
namespace Runtime {
namespace Instance {

class ModuleInstance {
public:
  ModuleInstance(std::string_view Name) : ModName(Name) {}
  ~ModuleInstance() = default;

  std::string_view getModuleName() const {
    std::shared_lock Lock(Mutex);
    return ModName;
  }

  /// Copy the function types in type section to module instance.
  void addFuncType(const AST::FunctionType &FuncType) {
    std::unique_lock Lock(Mutex);
    FuncTypes.emplace_back(FuncType);
  }

  /// Register module owns instances with address in Store.
  void addFunc(FunctionInstance *Func) {
    std::unique_lock Lock(Mutex);
    unsafeAddInsts(FuncInsts, Func);
  }
  void addTable(TableInstance *Tab) {
    std::unique_lock Lock(Mutex);
    unsafeAddInsts(TabInsts, Tab);
  }
  void addMemory(MemoryInstance *Mem) {
    std::unique_lock Lock(Mutex);
    unsafeAddInsts(MemInsts, Mem);
  }
  void addGlobal(GlobalInstance *Glob) {
    std::unique_lock Lock(Mutex);
    unsafeAddInsts(GlobInsts, Glob);
  }
  void addElem(ElementInstance *Elem) {
    std::unique_lock Lock(Mutex);
    unsafeAddInsts(ElemInsts, Elem);
  }
  void addData(DataInstance *Data) {
    std::unique_lock Lock(Mutex);
    unsafeAddInsts(DataInsts, Data);
  }

  /// Import instances.
  void importFunction(FunctionInstance *Func) {
    std::unique_lock Lock(Mutex);
    ImpFuncNum++;
    unsafeAddInsts(FuncInsts, Func);
  }
  void importTable(TableInstance *Tab) {
    std::unique_lock Lock(Mutex);
    ImpTableNum++;
    unsafeAddInsts(TabInsts, Tab);
  }
  void importMemory(MemoryInstance *Mem) {
    std::unique_lock Lock(Mutex);
    ImpMemNum++;
    unsafeAddInsts(MemInsts, Mem);
  }
  void importGlobal(GlobalInstance *Glob) {
    std::unique_lock Lock(Mutex);
    ImpGlobalNum++;
    unsafeAddInsts(GlobInsts, Glob);
  }

  /// Export instances.
  void exportFunction(std::string_view Name, const uint32_t Idx) {
    std::unique_lock Lock(Mutex);
    ExpFuncs.insert_or_assign(std::string(Name), FuncInsts[Idx]);
  }
  void exportTable(std::string_view Name, const uint32_t Idx) {
    std::unique_lock Lock(Mutex);
    ExpTables.insert_or_assign(std::string(Name), TabInsts[Idx]);
  }
  void exportMemory(std::string_view Name, const uint32_t Idx) {
    std::unique_lock Lock(Mutex);
    ExpMems.insert_or_assign(std::string(Name), MemInsts[Idx]);
  }
  void exportGlobal(std::string_view Name, const uint32_t Idx) {
    std::unique_lock Lock(Mutex);
    ExpGlobals.insert_or_assign(std::string(Name), GlobInsts[Idx]);
  }

  /// Get import nums.
  uint32_t getFuncImportNum() const { return ImpFuncNum; }
  uint32_t getTableImportNum() const { return ImpTableNum; }
  uint32_t getMemImportNum() const { return ImpMemNum; }
  uint32_t getGlobalImportNum() const { return ImpGlobalNum; }

  /// Get export maps.
  FunctionInstance *findFuncExports(std::string_view ExtName) const {
    std::shared_lock Lock(Mutex);
    return unsafeFindExports(ExpFuncs, ExtName);
  }
  TableInstance *findTableExports(std::string_view ExtName) const {
    std::shared_lock Lock(Mutex);
    return unsafeFindExports(ExpTables, ExtName);
  }
  MemoryInstance *findMemExports(std::string_view ExtName) const {
    std::shared_lock Lock(Mutex);
    return unsafeFindExports(ExpMems, ExtName);
  }
  GlobalInstance *findGlobalExports(std::string_view ExtName) const {
    std::shared_lock Lock(Mutex);
    return unsafeFindExports(ExpGlobals, ExtName);
  }

  size_t getFuncExportsSize() const {
    std::shared_lock Lock(Mutex);
    return ExpFuncs.size();
  }
  size_t getTableExportsSize() const {
    std::shared_lock Lock(Mutex);
    return ExpTables.size();
  }
  size_t getMemExportsSize() const {
    std::shared_lock Lock(Mutex);
    return ExpMems.size();
  }
  size_t getGlobalExportsSize() const {
    std::shared_lock Lock(Mutex);
    return ExpGlobals.size();
  }

  template <typename CallbackT>
  auto getFuncExports(CallbackT &&CallBack) const {
    std::shared_lock Lock(Mutex);
    return std::forward<CallbackT>(CallBack)(ExpFuncs);
  }
  template <typename CallbackT>
  auto getTableExports(CallbackT &&CallBack) const {
    std::shared_lock Lock(Mutex);
    return std::forward<CallbackT>(CallBack)(ExpTables);
  }
  template <typename CallbackT> auto getMemExports(CallbackT &&CallBack) const {
    std::shared_lock Lock(Mutex);
    return std::forward<CallbackT>(CallBack)(ExpMems);
  }
  template <typename CallbackT>
  auto getGlobalExports(CallbackT &&CallBack) const {
    std::shared_lock Lock(Mutex);
    return std::forward<CallbackT>(CallBack)(ExpGlobals);
  }

  /// Get function type by index.
  Expect<const AST::FunctionType *> getFuncType(const uint32_t Idx) const {
    std::shared_lock Lock(Mutex);
    if (unlikely(Idx >= FuncTypes.size())) {
      // Error logging need to be handled in caller.
      return Unexpect(ErrCode::WrongInstanceIndex);
    }
    return &FuncTypes[Idx];
  }
  /// Get function by index.
  Expect<FunctionInstance *> getFunc(const uint32_t Idx) const {
    std::shared_lock Lock(Mutex);
    if (Idx >= FuncInsts.size()) {
      // Error logging need to be handled in caller.
      return Unexpect(ErrCode::WrongInstanceIndex);
    }
    return FuncInsts[Idx];
  }
  Expect<TableInstance *> getTable(const uint32_t Idx) const {
    std::shared_lock Lock(Mutex);
    if (Idx >= TabInsts.size()) {
      // Error logging need to be handled in caller.
      return Unexpect(ErrCode::WrongInstanceIndex);
    }
    return TabInsts[Idx];
  }
  Expect<MemoryInstance *> getMemory(const uint32_t Idx) const {
    std::shared_lock Lock(Mutex);
    if (Idx >= MemInsts.size()) {
      // Error logging need to be handled in caller.
      return Unexpect(ErrCode::WrongInstanceIndex);
    }
    return MemInsts[Idx];
  }
  Expect<GlobalInstance *> getGlobal(const uint32_t Idx) const {
    std::shared_lock Lock(Mutex);
    if (Idx >= GlobInsts.size()) {
      // Error logging need to be handled in caller.
      return Unexpect(ErrCode::WrongInstanceIndex);
    }
    return GlobInsts[Idx];
  }
  Expect<ElementInstance *> getElem(const uint32_t Idx) const {
    std::shared_lock Lock(Mutex);
    if (Idx >= ElemInsts.size()) {
      // Error logging need to be handled in caller.
      return Unexpect(ErrCode::WrongInstanceIndex);
    }
    return ElemInsts[Idx];
  }
  Expect<DataInstance *> getData(const uint32_t Idx) const {
    std::shared_lock Lock(Mutex);
    if (Idx >= DataInsts.size()) {
      // Error logging need to be handled in caller.
      return Unexpect(ErrCode::WrongInstanceIndex);
    }
    return DataInsts[Idx];
  }

  /// Get the added external values' numbers.
  uint32_t getFuncNum() const {
    std::shared_lock Lock(Mutex);
    return static_cast<uint32_t>(FuncInsts.size());
  }
  uint32_t getTableNum() const {
    std::shared_lock Lock(Mutex);
    return static_cast<uint32_t>(TabInsts.size());
  }
  uint32_t getMemNum() const {
    std::shared_lock Lock(Mutex);
    return static_cast<uint32_t>(MemInsts.size());
  }
  uint32_t getGlobalNum() const {
    std::shared_lock Lock(Mutex);
    return static_cast<uint32_t>(GlobInsts.size());
  }
  uint32_t getElemNum() const {
    std::shared_lock Lock(Mutex);
    return static_cast<uint32_t>(ElemInsts.size());
  }
  uint32_t getDataNum() const {
    std::shared_lock Lock(Mutex);
    return static_cast<uint32_t>(DataInsts.size());
  }

  /// Set the start function index and find the function instance.
  void setStartIdx(const uint32_t Idx) {
    std::unique_lock Lock(Mutex);
    StartFunc = FuncInsts[Idx];
  }

  /// Get start function address in Store.
  FunctionInstance *getStartFunc() const {
    std::shared_lock Lock(Mutex);
    return StartFunc;
  }

  /// \name Data for compiled functions.
  /// @{
  std::vector<uint8_t *> MemoryPtrs;
  std::vector<ValVariant *> GlobalPtrs;
  /// @}

private:
  /// Add the instances under the module by pointers.
  template <typename T> void unsafeAddInsts(std::vector<T *> &Vec, T *Ptr) {
    Vec.push_back(Ptr);
  }
  /// Export the instances with name.
  template <typename T>
  T *unsafeFindExports(const std::map<std::string, T *, std::less<>> &Map,
                       std::string_view ExtName) const {
    auto Iter = Map.find(ExtName);
    if (likely(Iter != Map.cend())) {
      return Iter->second;
    }
    return nullptr;
  }

  /// Mutex.
  mutable std::shared_mutex Mutex;

  /// Module name.
  const std::string ModName;

  /// Function types.
  std::vector<AST::FunctionType> FuncTypes;

  /// Instances of this module. The instances are owned by store.
  std::vector<FunctionInstance *> FuncInsts;
  std::vector<TableInstance *> TabInsts;
  std::vector<MemoryInstance *> MemInsts;
  std::vector<GlobalInstance *> GlobInsts;
  std::vector<ElementInstance *> ElemInsts;
  std::vector<DataInstance *> DataInsts;

  /// Imports.
  uint32_t ImpFuncNum = 0;
  uint32_t ImpTableNum = 0;
  uint32_t ImpMemNum = 0;
  uint32_t ImpGlobalNum = 0;

  /// Exports.
  std::map<std::string, FunctionInstance *, std::less<>> ExpFuncs;
  std::map<std::string, TableInstance *, std::less<>> ExpTables;
  std::map<std::string, MemoryInstance *, std::less<>> ExpMems;
  std::map<std::string, GlobalInstance *, std::less<>> ExpGlobals;

  /// Start function instance
  FunctionInstance *StartFunc = nullptr;
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
