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
  ErrCode getFuncNum(unsigned int &Num);
  ErrCode getTableNum(unsigned int &Num);
  ErrCode getMemNum(unsigned int &Num);
  ErrCode getGlobalNum(unsigned int &Num);

  /// Set start function index and find the address in Store.
  ErrCode setStartIdx(unsigned int Idx);

  /// Get start function address in Store.
  ErrCode getStartAddr(unsigned int &Addr);

  /// Module Instance address in store manager.
  unsigned int Addr;

private:
  /// Function type definition in this module.
  struct FType {
    FType() = default;
    ~FType() = default;
    std::vector<AST::ValType> Params;
    std::vector<AST::ValType> Returns;
  };
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
