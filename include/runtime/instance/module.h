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
  void addFuncAddr(const uint32_t FuncAddr) {
    std::unique_lock Lock(Mutex);
    unsafeAddFuncAddr(FuncAddr);
  }
  void addTableAddr(const uint32_t TabAddr) {
    std::unique_lock Lock(Mutex);
    unsafeAddTableAddr(TabAddr);
  }
  void addMemAddr(const uint32_t MemAddr) {
    std::unique_lock Lock(Mutex);
    unsafeAddMemAddr(MemAddr);
  }
  void addGlobalAddr(const uint32_t GlobAddr) {
    std::unique_lock Lock(Mutex);
    unsafeAddGlobalAddr(GlobAddr);
  }
  void addElemAddr(const uint32_t ElemAddr) {
    std::unique_lock Lock(Mutex);
    unsafeAddElemAddr(ElemAddr);
  }
  void addDataAddr(const uint32_t DataAddr) {
    std::unique_lock Lock(Mutex);
    unsafeAddDataAddr(DataAddr);
  }

  /// Import instances.
  void importFunction(const uint32_t FuncAddr) {
    std::unique_lock Lock(Mutex);
    ImpFuncNum++;
    unsafeAddFuncAddr(FuncAddr);
  }
  void importTable(const uint32_t TabAddr) {
    std::unique_lock Lock(Mutex);
    ImpTableNum++;
    unsafeAddTableAddr(TabAddr);
  }
  void importMemory(const uint32_t MemAddr) {
    std::unique_lock Lock(Mutex);
    ImpMemNum++;
    unsafeAddMemAddr(MemAddr);
  }
  void importGlobal(const uint32_t GlobAddr) {
    std::unique_lock Lock(Mutex);
    ImpGlobalNum++;
    unsafeAddGlobalAddr(GlobAddr);
  }

  /// Export instances.
  void exportFunction(std::string_view Name, const uint32_t Idx) {
    std::unique_lock Lock(Mutex);
    ExpFuncs.insert_or_assign(std::string(Name), FuncAddrs[Idx]);
  }
  void exportTable(std::string_view Name, const uint32_t Idx) {
    std::unique_lock Lock(Mutex);
    ExpTables.insert_or_assign(std::string(Name), TableAddrs[Idx]);
  }
  void exportMemory(std::string_view Name, const uint32_t Idx) {
    std::unique_lock Lock(Mutex);
    ExpMems.insert_or_assign(std::string(Name), MemAddrs[Idx]);
  }
  void exportGlobal(std::string_view Name, const uint32_t Idx) {
    std::unique_lock Lock(Mutex);
    ExpGlobals.insert_or_assign(std::string(Name), GlobalAddrs[Idx]);
  }

  /// Get import nums.
  uint32_t getFuncImportNum() const { return ImpFuncNum; }
  uint32_t getTableImportNum() const { return ImpTableNum; }
  uint32_t getMemImportNum() const { return ImpMemNum; }
  uint32_t getGlobalImportNum() const { return ImpGlobalNum; }

  /// Get export maps.
  std::optional<uint32_t> findFuncExports(std::string_view ExtName) const {
    std::shared_lock Lock(Mutex);
    return unsafeFindExports(ExpFuncs, ExtName);
  }
  std::optional<uint32_t> findTableExports(std::string_view ExtName) const {
    std::shared_lock Lock(Mutex);
    return unsafeFindExports(ExpTables, ExtName);
  }
  std::optional<uint32_t> findMemExports(std::string_view ExtName) const {
    std::shared_lock Lock(Mutex);
    return unsafeFindExports(ExpMems, ExtName);
  }
  std::optional<uint32_t> findGlobalExports(std::string_view ExtName) const {
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
    if (Idx >= FuncTypes.size()) {
      // Error logging need to be handled in caller.
      return Unexpect(ErrCode::WrongInstanceIndex);
    }
    return &FuncTypes[Idx];
  }
  /// Get the external values by index. Addr will be address in Store.
  Expect<uint32_t> getFuncAddr(const uint32_t Idx) const {
    std::shared_lock Lock(Mutex);
    if (Idx >= FuncAddrs.size()) {
      // Error logging need to be handled in caller.
      return Unexpect(ErrCode::WrongInstanceIndex);
    }
    return FuncAddrs[Idx];
  }
  Expect<uint32_t> getTableAddr(const uint32_t Idx) const {
    std::shared_lock Lock(Mutex);
    if (Idx >= TableAddrs.size()) {
      // Error logging need to be handled in caller.
      return Unexpect(ErrCode::WrongInstanceIndex);
    }
    return TableAddrs[Idx];
  }
  Expect<uint32_t> getMemAddr(const uint32_t Idx) const {
    std::shared_lock Lock(Mutex);
    if (Idx >= MemAddrs.size()) {
      // Error logging need to be handled in caller.
      return Unexpect(ErrCode::WrongInstanceIndex);
    }
    return MemAddrs[Idx];
  }
  Expect<uint32_t> getGlobalAddr(const uint32_t Idx) const {
    std::shared_lock Lock(Mutex);
    if (Idx >= GlobalAddrs.size()) {
      // Error logging need to be handled in caller.
      return Unexpect(ErrCode::WrongInstanceIndex);
    }
    return GlobalAddrs[Idx];
  }
  Expect<uint32_t> getElemAddr(const uint32_t Idx) const {
    std::shared_lock Lock(Mutex);
    if (Idx >= ElemAddrs.size()) {
      // Error logging need to be handled in caller.
      return Unexpect(ErrCode::WrongInstanceIndex);
    }
    return ElemAddrs[Idx];
  }
  Expect<uint32_t> getDataAddr(const uint32_t Idx) const {
    std::shared_lock Lock(Mutex);
    if (Idx >= DataAddrs.size()) {
      // Error logging need to be handled in caller.
      return Unexpect(ErrCode::WrongInstanceIndex);
    }
    return DataAddrs[Idx];
  }

  /// Get the added external values' numbers.
  uint32_t getFuncNum() const {
    std::shared_lock Lock(Mutex);
    return static_cast<uint32_t>(FuncAddrs.size());
  }
  uint32_t getTableNum() const {
    std::shared_lock Lock(Mutex);
    return static_cast<uint32_t>(TableAddrs.size());
  }
  uint32_t getMemNum() const {
    std::shared_lock Lock(Mutex);
    return static_cast<uint32_t>(MemAddrs.size());
  }
  uint32_t getGlobalNum() const {
    std::shared_lock Lock(Mutex);
    return static_cast<uint32_t>(GlobalAddrs.size());
  }
  uint32_t getElemNum() const {
    std::shared_lock Lock(Mutex);
    return static_cast<uint32_t>(ElemAddrs.size());
  }
  uint32_t getDataNum() const {
    std::shared_lock Lock(Mutex);
    return static_cast<uint32_t>(DataAddrs.size());
  }

  /// Set start function index and find the address in Store.
  void setStartIdx(const uint32_t Idx) {
    std::unique_lock Lock(Mutex);
    StartAddr = FuncAddrs[Idx];
    HasStartFunc = true;
  }

  /// Get start function address in Store.
  std::optional<uint32_t> getStartAddr() const {
    std::shared_lock Lock(Mutex);
    if (!HasStartFunc) {
      return std::nullopt;
    }
    return {StartAddr};
  }

  /// Module Instance address in store manager.
  uint32_t Addr;

  /// \name Data for compiled functions.
  /// @{
  std::vector<uint8_t *> MemoryPtrs;
  std::vector<ValVariant *> GlobalPtrs;
  /// @}

private:
  /// Register module owns instances with address in Store.
  void unsafeAddFuncAddr(const uint32_t FuncAddr) {
    FuncAddrs.push_back(FuncAddr);
  }
  void unsafeAddTableAddr(const uint32_t TabAddr) {
    TableAddrs.push_back(TabAddr);
  }
  void unsafeAddMemAddr(const uint32_t MemAddr) { MemAddrs.push_back(MemAddr); }
  void unsafeAddGlobalAddr(const uint32_t GlobAddr) {
    GlobalAddrs.push_back(GlobAddr);
  }
  void unsafeAddElemAddr(const uint32_t ElemAddr) {
    ElemAddrs.push_back(ElemAddr);
  }
  void unsafeAddDataAddr(const uint32_t DataAddr) {
    DataAddrs.push_back(DataAddr);
  }
  static std::optional<uint32_t>
  unsafeFindExports(const std::map<std::string, uint32_t, std::less<>> &Map,
                    std::string_view ExtName) {
    if (auto Iter = Map.find(ExtName); Iter != Map.cend()) {
      return Iter->second;
    }
    return std::nullopt;
  }

  /// Mutex.
  mutable std::shared_mutex Mutex;

  /// Module name.
  const std::string ModName;

  /// Function types.
  std::vector<AST::FunctionType> FuncTypes;

  /// Elements address index in this module in Store.
  std::vector<uint32_t> FuncAddrs;
  std::vector<uint32_t> TableAddrs;
  std::vector<uint32_t> MemAddrs;
  std::vector<uint32_t> GlobalAddrs;
  std::vector<uint32_t> ElemAddrs;
  std::vector<uint32_t> DataAddrs;

  /// Imports.
  uint32_t ImpFuncNum = 0;
  uint32_t ImpTableNum = 0;
  uint32_t ImpMemNum = 0;
  uint32_t ImpGlobalNum = 0;

  /// Exports.
  std::map<std::string, uint32_t, std::less<>> ExpFuncs;
  std::map<std::string, uint32_t, std::less<>> ExpTables;
  std::map<std::string, uint32_t, std::less<>> ExpMems;
  std::map<std::string, uint32_t, std::less<>> ExpGlobals;

  /// Start function address
  bool HasStartFunc = false;
  uint32_t StartAddr;
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
