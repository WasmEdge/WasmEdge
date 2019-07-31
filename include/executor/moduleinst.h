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
#include <vector>

class ModuleInstance {
public:
  ModuleInstance() = default;
  ~ModuleInstance() = default;

  /// Move the function types in type section to module instance.
  Executor::ErrCode addFuncType(std::vector<AST::ValType> &Params,
                                std::vector<AST::ValType> &Returns);

  /// Map the external instences between Module and Store.
  Executor::ErrCode addFuncAddr(unsigned int StoreFuncID);
  Executor::ErrCode addTableAddr(unsigned int StoreTableID);
  Executor::ErrCode addMemAddr(unsigned int StoreMemID);
  Executor::ErrCode addGlobalAddr(unsigned int StoreGlobalID);

  /// Module Instance ID in store manager.
  unsigned int Id;

private:
  /// Function type definition in this module.
  struct FType {
    std::vector<AST::ValType> Params;
    std::vector<AST::ValType> Returns;
  };
  std::vector<std::unique_ptr<FType>> FuncType;

  /// Elements address index in this module in Store.
  std::vector<unsigned int> FuncAddr;
  std::vector<unsigned int> TableAddr;
  std::vector<unsigned int> MemAddr;
  std::vector<unsigned int> GlobalAddr;
  // TODO: add export inst
};