//===-- ssvm/executor/instance/module.h - Module Instance definition ------===//
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

#include "ast/common.h"
#include "executor/common.h"
#include <memory>
#include <vector>

namespace SSVM {
namespace Executor {
namespace Instance {

class ModuleInstance {
public:
  ModuleInstance() = default;
  ~ModuleInstance() = default;

  /// Function type definition in this module.
  struct FType {
    std::vector<AST::ValType> Params;
    std::vector<AST::ValType> Returns;
  };

  /// Copy the function types in type section to module instance.
  ErrCode addFuncType(const std::vector<AST::ValType> &Params,
                      const std::vector<AST::ValType> &Returns);

  /// Map the external instences between Module and Store.
  ErrCode addFuncAddr(unsigned int StoreFuncAddr);
  ErrCode addTableAddr(unsigned int StoreTableAddr);
  ErrCode addMemAddr(unsigned int StoreMemAddr);
  ErrCode addGlobalAddr(unsigned int StoreGlobalAddr);

  /// Get the external values by index. Addr will be address in Store.
  ErrCode getFuncAddr(unsigned int Idx, unsigned int &Addr);
  ErrCode getTableAddr(unsigned int Idx, unsigned int &Addr);
  ErrCode getMemAddr(unsigned int Idx, unsigned int &Addr);
  ErrCode getGlobalAddr(unsigned int Idx, unsigned int &Addr);

  /// Get the added external values' numbers.
  unsigned int getFuncNum() { return FuncAddrs.size(); }
  unsigned int getTableNum() { return TableAddrs.size(); }
  unsigned int getMemNum() { return MemAddrs.size(); }
  unsigned int getGlobalNum() { return GlobalAddrs.size(); }

  /// Set start function index and find the address in Store.
  ErrCode setStartIdx(unsigned int Idx);

  /// Get start function address in Store.
  bool getStartAddr(unsigned int &Addr) {
    Addr = StartAddr;
    return HasStartFunc;
  };

  /// Get function type by index
  ErrCode getFuncType(unsigned int Idx, FType *&Type);

  /// Module Instance address in store manager.
  unsigned int Addr;

private:
  /// Function types.
  std::vector<std::unique_ptr<FType>> FuncTypes;

  /// Elements address index in this module in Store.
  std::vector<unsigned int> FuncAddrs;
  std::vector<unsigned int> TableAddrs;
  std::vector<unsigned int> MemAddrs;
  std::vector<unsigned int> GlobalAddrs;
  /// TODO: add export inst

  /// Start function address
  bool HasStartFunc = false;
  unsigned int StartAddr;
};

} // namespace Instance
} // namespace Executor
} // namespace SSVM
