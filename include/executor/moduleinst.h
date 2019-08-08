//===-- ssvm/executor/moduleinst.h - Module Instance definition -----------===//
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
#include "common.h"
#include <memory>
#include <vector>

namespace SSVM {

class ModuleInstance {
public:
  ModuleInstance() = default;
  ~ModuleInstance() = default;

  /// Move the function types in type section to module instance.
  Executor::ErrCode addFuncType(std::vector<AST::ValType> &Params,
                                std::vector<AST::ValType> &Returns);

  /// Map the external instences between Module and Store.
  Executor::ErrCode addFuncAddr(unsigned int StoreFuncAddr);
  Executor::ErrCode addTableAddr(unsigned int StoreTableAddr);
  Executor::ErrCode addMemAddr(unsigned int StoreMemAddr);
  Executor::ErrCode addGlobalAddr(unsigned int StoreGlobalAddr);

  /// Get the external values by index. Addr will be address in Store.
  Executor::ErrCode getFuncAddr(unsigned int Idx, unsigned int &Addr);
  Executor::ErrCode getTableAddr(unsigned int Idx, unsigned int &Addr);
  Executor::ErrCode getMemAddr(unsigned int Idx, unsigned int &Addr);
  Executor::ErrCode getGlobalAddr(unsigned int Idx, unsigned int &Addr);

  /// Get the added external values' numbers.
  Executor::ErrCode getFuncNum(unsigned int &Num);
  Executor::ErrCode getTableNum(unsigned int &Num);
  Executor::ErrCode getMemNum(unsigned int &Num);
  Executor::ErrCode getGlobalNum(unsigned int &Num);

  /// Set start function index and find the address in Store.
  Executor::ErrCode setStartIdx(unsigned int Idx);

  /// Get start function address in Store.
  Executor::ErrCode getStartAddr(unsigned int &Addr);

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

} // namespace SSVM