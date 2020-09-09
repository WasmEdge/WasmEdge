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
#include "support/span.h"
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
  ModuleInstance(std::string_view Name) : ModName(Name) {}
  ~ModuleInstance() = default;

  std::string_view getModuleName() const { return ModName; }

  /// Copy the function types in type section to module instance.
  void addFuncType(Span<const ValType> Params, Span<const ValType> Returns) {
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
  void exportFunction(std::string_view Name, const uint32_t Idx) {
    ExpFuncs.emplace(Name, FuncAddrs[Idx]);
  }
  void exportTable(std::string_view Name, const uint32_t Idx) {
    ExpTables.emplace(Name, TableAddrs[Idx]);
  }
  void exportMemory(std::string_view Name, const uint32_t Idx) {
    ExpMems.emplace(Name, MemAddrs[Idx]);
  }
  void exportGlobal(std::string_view Name, const uint32_t Idx) {
    ExpGlobals.emplace(Name, GlobalAddrs[Idx]);
  }

  /// Get export maps.
  const std::map<std::string, uint32_t, std::less<>> &getFuncExports() const {
    return ExpFuncs;
  }
  const std::map<std::string, uint32_t, std::less<>> &getTableExports() const {
    return ExpTables;
  }
  const std::map<std::string, uint32_t, std::less<>> &getMemExports() const {
    return ExpMems;
  }
  const std::map<std::string, uint32_t, std::less<>> &getGlobalExports() const {
    return ExpGlobals;
  }

  /// Get function type by index.
  Expect<const FType *> getFuncType(const uint32_t Idx) const {
    if (Idx >= FuncTypes.size()) {
      /// Error logging need to be handled in caller.
      return Unexpect(ErrCode::WrongInstanceIndex);
    }
    return &FuncTypes[Idx];
  }
  /// Get the external values by index. Addr will be address in Store.
  Expect<uint32_t> getFuncAddr(const uint32_t Idx) const {
    if (Idx >= FuncAddrs.size()) {
      /// Error logging need to be handled in caller.
      return Unexpect(ErrCode::WrongInstanceIndex);
    }
    return FuncAddrs[Idx];
  }
  Expect<uint32_t> getTableAddr(const uint32_t Idx) const {
    if (Idx >= TableAddrs.size()) {
      /// Error logging need to be handled in caller.
      return Unexpect(ErrCode::WrongInstanceIndex);
    }
    return TableAddrs[Idx];
  }
  Expect<uint32_t> getMemAddr(const uint32_t Idx) const {
    if (Idx >= MemAddrs.size()) {
      /// Error logging need to be handled in caller.
      return Unexpect(ErrCode::WrongInstanceIndex);
    }
    return MemAddrs[Idx];
  }
  Expect<uint32_t> getGlobalAddr(const uint32_t Idx) const {
    if (Idx >= GlobalAddrs.size()) {
      /// Error logging need to be handled in caller.
      return Unexpect(ErrCode::WrongInstanceIndex);
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
} // namespace SSVM
