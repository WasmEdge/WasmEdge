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

#include "ast/section.h"
#include "ast/type.h"
#include "common.h"
#include <vector>

class ModuleInstance {
public:
  /// Move the function types in type section to module instance.
  Executor::ErrCode setFuncType(std::unique_ptr<AST::FunctionType> &FType);

  /// Map the external instences between Module and Store.
  Executor::ErrCode setFuncAddr(unsigned int ModuleFuncID,
                                unsigned int StoreFuncID);
  Executor::ErrCode setTableAddr(unsigned int ModuleTableID,
                                 unsigned int StoreTableID);
  Executor::ErrCode setMemAddr(unsigned int ModuleMemID,
                               unsigned int StoreMemID);
  Executor::ErrCode setGlobalAddr(unsigned int ModuleGlobalID,
                                  unsigned int StoreGlobalID);

private:
  /// Function type definition in this module.
  std::vector<std::unique_ptr<AST::FunctionType>> FuncType;

  /// Elements address index in this module in Store.
  std::vector<unsigned int> FuncAddr;
  std::vector<unsigned int> TableAddr;
  std::vector<unsigned int> MemAddr;
  std::vector<unsigned int> GlobalAddr;
  // TODO: add export inst
};