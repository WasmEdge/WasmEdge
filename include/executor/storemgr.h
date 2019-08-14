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
#include "instance/function.h"
#include "instance/global.h"
#include "instance/memory.h"
#include "instance/module.h"
#include "instance/table.h"
#include <memory>
#include <vector>

namespace SSVM {
namespace Executor {

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
  ErrCode insertModuleInst(std::unique_ptr<Instance::ModuleInstance> &Mod,
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
  ErrCode insertFunctionInst(std::unique_ptr<Instance::FunctionInstance> &Func,
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
  ErrCode insertTableInst(std::unique_ptr<Instance::TableInstance> &Tab,
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
  ErrCode insertMemoryInst(std::unique_ptr<Instance::MemoryInstance> &Mem,
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
  ErrCode insertGlobalInst(std::unique_ptr<Instance::GlobalInstance> &Glob,
                           unsigned int &NewId);

  /// Get instance from store manager.
  ///
  /// Get the module instance by supplying module address.
  ///
  /// \param Addr the module address in Store.
  /// \param [out] Mod the module instance.
  ///
  /// \returns ErrCode.
  ErrCode getModule(unsigned int Addr, Instance::ModuleInstance *&Mod);

  /// Get instance from store manager.
  ///
  /// Get the function instance by supplying function address.
  ///
  /// \param Addr the function address in Store.
  /// \param [out] Func the function instance.
  ///
  /// \returns ErrCode.
  ErrCode getFunction(unsigned int Addr, Instance::FunctionInstance *&Func);

  /// Get instance from store manager.
  ///
  /// Get the table instance by supplying table address.
  ///
  /// \param Addr the table address in Store.
  /// \param [out] Tab the table instance.
  ///
  /// \returns ErrCode.
  ErrCode getTable(unsigned int Addr, Instance::TableInstance *&Tab);

  /// Get instance from store manager.
  ///
  /// Get the memory instance by supplying memory address.
  ///
  /// \param Addr the memory address in Store.
  /// \param [out] Mem the memory instance.
  ///
  /// \returns ErrCode.
  ErrCode getMemory(unsigned int Addr, Instance::MemoryInstance *&Mem);

  /// Get instance from store manager.
  ///
  /// Get the global instance by supplying global address.
  ///
  /// \param Addr the global address in Store.
  /// \param [out] Glob the global instance.
  ///
  /// \returns ErrCode.
  ErrCode getGlobal(unsigned int Addr, Instance::GlobalInstance *&Glob);

  /// Find function from store manager.
  ///
  /// Get the function instance by supplying global address.
  ///
  /// \param ModName the module name of function.
  /// \param FuncName the function name.
  /// \param [out] Func the function instance.
  ///
  /// \returns ErrCode.
  ErrCode findFunction(std::string &ModName, std::string &FuncName,
                       Instance::FunctionInstance *&Func);

private:
  std::vector<std::unique_ptr<Instance::ModuleInstance>> ModInsts;
  std::vector<std::unique_ptr<Instance::FunctionInstance>> FuncInsts;
  std::vector<std::unique_ptr<Instance::TableInstance>> TabInsts;
  std::vector<std::unique_ptr<Instance::MemoryInstance>> MemInsts;
  std::vector<std::unique_ptr<Instance::GlobalInstance>> GlobInsts;
};

} // namespace Executor
} // namespace SSVM
