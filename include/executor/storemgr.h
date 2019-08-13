//===-- ssvm/executor/storemgr.h - Store Manager definition ---------------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definition of Store Manager.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common.h"
#include "functioninst.h"
#include "globalinst.h"
#include "memoryinst.h"
#include "moduleinst.h"
#include "tableinst.h"
#include <memory>
#include <vector>

namespace SSVM {

class StoreManager {
public:
  StoreManager() = default;
  ~StoreManager() = default;
  /// Insert instance to store manager.
  ///
  /// Insert new module instance to store manager.
  /// Query a new address in Store for insert and set it to module instance.
  /// Get the address of inserted module instance.
  ///
  /// \param Mod the pointer to module instance.
  /// \param [out] NewId the module address in Store.
  ///
  /// \returns ErrCode.
  Executor::ErrCode insertModuleInst(std::unique_ptr<ModuleInstance> &Mod,
                                     unsigned int &NewId);

  /// Insert instance to store manager.
  ///
  /// Insert new function instance to store manager.
  /// Query a new address in Store for insert and set it to function instance.
  /// Get the address of inserted function instance.
  ///
  /// \param Func the pointer to function instance.
  /// \param [out] NewId the function address in Store.
  ///
  /// \returns ErrCode.
  Executor::ErrCode insertFunctionInst(std::unique_ptr<FunctionInstance> &Func,
                                       unsigned int &NewId);

  /// Insert instance to store manager.
  ///
  /// Insert new table instance to store manager.
  /// Query a new address in Store for insert and set it to table instance.
  /// Get the address of inserted table instance.
  ///
  /// \param Tab the pointer to table instance.
  /// \param [out] NewId the table address in Store.
  ///
  /// \returns ErrCode.
  Executor::ErrCode insertTableInst(std::unique_ptr<TableInstance> &Tab,
                                    unsigned int &NewId);

  /// Insert instance to store manager.
  ///
  /// Insert new memory instance to store manager.
  /// Query a new address in Store for insert and set it to memory instance.
  /// Get the address of inserted memory instance.
  ///
  /// \param Mem the pointer to memory instance.
  /// \param [out] NewId the memory address in Store.
  ///
  /// \returns ErrCode.
  Executor::ErrCode insertMemoryInst(std::unique_ptr<MemoryInstance> &Mem,
                                     unsigned int &NewId);

  /// Insert instance to store manager.
  ///
  /// Insert new global instance to store manager.
  /// Query a new address in Store for insert and set it to global instance.
  /// Get the address of inserted global instance.
  ///
  /// \param Glob the pointer to global instance.
  /// \param [out] NewId the global address in Store.
  ///
  /// \returns ErrCode.
  Executor::ErrCode insertGlobalInst(std::unique_ptr<GlobalInstance> &Glob,
                                     unsigned int &NewId);

  /// Get instance from store manager.
  ///
  /// Get the module instance by supplying module address.
  ///
  /// \param Addr the module address in Store.
  /// \param [out] Mod the module instance.
  ///
  /// \returns ErrCode.
  Executor::ErrCode getModule(unsigned int Addr, ModuleInstance *&Mod);

  /// Get instance from store manager.
  ///
  /// Get the function instance by supplying function address.
  ///
  /// \param Addr the function address in Store.
  /// \param [out] Func the function instance.
  ///
  /// \returns ErrCode.
  Executor::ErrCode getFunction(unsigned int Addr, FunctionInstance *&Func);

  /// Get instance from store manager.
  ///
  /// Get the table instance by supplying table address.
  ///
  /// \param Addr the table address in Store.
  /// \param [out] Tab the table instance.
  ///
  /// \returns ErrCode.
  Executor::ErrCode getTable(unsigned int Addr, TableInstance *&Tab);

  /// Get instance from store manager.
  ///
  /// Get the memory instance by supplying memory address.
  ///
  /// \param Addr the memory address in Store.
  /// \param [out] Mem the memory instance.
  ///
  /// \returns ErrCode.
  Executor::ErrCode getMemory(unsigned int Addr, MemoryInstance *&Mem);

  /// Get instance from store manager.
  ///
  /// Get the global instance by supplying global address.
  ///
  /// \param Addr the global address in Store.
  /// \param [out] Glob the global instance.
  ///
  /// \returns ErrCode.
  Executor::ErrCode getGlobal(unsigned int Addr, GlobalInstance *&Glob);

  /// Find function from store manager.
  ///
  /// Get the function instance by supplying global address.
  ///
  /// \param ModName the module name of function.
  /// \param FuncName the function name.
  /// \param [out] Func the function instance.
  ///
  /// \returns ErrCode.
  Executor::ErrCode findFunction(std::string &ModName, std::string &FuncName,
                                 FunctionInstance *&Func);

private:
  std::vector<std::unique_ptr<ModuleInstance>> ModInsts;
  std::vector<std::unique_ptr<FunctionInstance>> FuncInsts;
  std::vector<std::unique_ptr<TableInstance>> TabInsts;
  std::vector<std::unique_ptr<MemoryInstance>> MemInsts;
  std::vector<std::unique_ptr<GlobalInstance>> GlobInsts;
};

} // namespace SSVM
