// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/runtime/instance/module.h - Module Instance definition ---=---===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the module instance definition in store manager.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/errcode.h"
#include "common/types.h"
#include "type.h"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace SSVM {
namespace Runtime {
namespace Instance {

class ModuleInstance {
public:
  ModuleInstance(const std::string &Name) : ModName(Name) {}
  ~ModuleInstance() = default;

  const std::string &getModuleName() const { return ModName; }

  /// Copy the function types in type section to module instance.
  void addFuncType(const std::vector<ValType> &Params,
                   const std::vector<ValType> &Returns) {
    FuncTypes.emplace_back(Params, Returns);
  }

  /// Map the external instences between Module and Store.
  void addFuncAddr(const uint32_t FuncAddr) { FuncAddrs.push_back(FuncAddr); }
  void addTableAddr(const uint32_t TabAddr) { TableAddrs.push_back(TabAddr); }
  void addMemAddr(const uint32_t MemAddr) { MemAddrs.push_back(MemAddr); }
  void addGlobalAddr(const uint32_t GlobAddr) {
    GlobalAddrs.push_back(GlobAddr);
  }

  /// Exports functions.
  void exportFuncion(const std::string &Name, const uint32_t Idx) {
    ExpFuncs[Name] = FuncAddrs[Idx];
  }
  void exportTable(const std::string &Name, const uint32_t Idx) {
    ExpTables[Name] = TableAddrs[Idx];
  }
  void exportMemory(const std::string &Name, const uint32_t Idx) {
    ExpMems[Name] = MemAddrs[Idx];
  }
  void exportGlobal(const std::string &Name, const uint32_t Idx) {
    ExpGlobals[Name] = GlobalAddrs[Idx];
  }

  /// Get export maps.
  const std::map<std::string, uint32_t> &getFuncExports() const {
    return ExpFuncs;
  }
  const std::map<std::string, uint32_t> &getTableExports() const {
    return ExpTables;
  }
  const std::map<std::string, uint32_t> &getMemExports() const {
    return ExpMems;
  }
  const std::map<std::string, uint32_t> &getGlobalExports() const {
    return ExpGlobals;
  }

  /// Get function type by index.
  Expect<const FType *> getFuncType(const uint32_t Idx) const {
    if (Idx >= FuncTypes.size()) {
      return Unexpect(ErrCode::WrongInstanceAddress);
    }
    return &FuncTypes[Idx];
  }
  /// Get the external values by index. Addr will be address in Store.
  Expect<uint32_t> getFuncAddr(const uint32_t Idx) const {
    if (Idx >= FuncAddrs.size()) {
      return Unexpect(ErrCode::WrongInstanceAddress);
    }
    return FuncAddrs[Idx];
  }
  Expect<uint32_t> getTableAddr(const uint32_t Idx) const {
    if (Idx >= TableAddrs.size()) {
      return Unexpect(ErrCode::WrongInstanceAddress);
    }
    return TableAddrs[Idx];
  }
  Expect<uint32_t> getMemAddr(const uint32_t Idx) const {
    if (Idx >= MemAddrs.size()) {
      return Unexpect(ErrCode::WrongInstanceAddress);
    }
    return MemAddrs[Idx];
  }
  Expect<uint32_t> getGlobalAddr(const uint32_t Idx) const {
    if (Idx >= GlobalAddrs.size()) {
      return Unexpect(ErrCode::WrongInstanceAddress);
    }
    return GlobalAddrs[Idx];
  }

  /// Get the added external values' numbers.
  uint32_t getFuncNum() const { return FuncAddrs.size(); }
  uint32_t getTableNum() const { return TableAddrs.size(); }
  uint32_t getMemNum() const { return MemAddrs.size(); }
  uint32_t getGlobalNum() const { return GlobalAddrs.size(); }

  /// Set start function index and find the address in Store.
  void setStartIdx(const uint32_t Idx) {
    StartAddr = FuncAddrs[Idx];
    HasStartFunc = true;
  }

  /// Get start function address in Store.
  std::optional<uint32_t> getStartAddr() const {
    if (!HasStartFunc) {
      return std::nullopt;
    }
    return {StartAddr};
  };

  /// Module Instance address in store manager.
  uint32_t Addr;

private:
  /// Module name.
  const std::string ModName;

  /// Function types.
  std::vector<FType> FuncTypes;

  /// Elements address index in this module in Store.
  std::vector<uint32_t> FuncAddrs;
  std::vector<uint32_t> TableAddrs;
  std::vector<uint32_t> MemAddrs;
  std::vector<uint32_t> GlobalAddrs;

  /// Exports.
  std::map<std::string, uint32_t> ExpFuncs;
  std::map<std::string, uint32_t> ExpTables;
  std::map<std::string, uint32_t> ExpMems;
  std::map<std::string, uint32_t> ExpGlobals;

  /// Start function address
  bool HasStartFunc = false;
  uint32_t StartAddr;
};

} // namespace Instance
} // namespace Runtime
} // namespace SSVM
