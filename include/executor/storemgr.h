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
#include <type_traits>
#include <vector>

namespace SSVM {
namespace Executor {

namespace {
/// Return true if T is entities.
template <typename T>
inline constexpr const bool IsEntityV =
    std::is_same_v<T, Instance::FunctionInstance> ||
    std::is_same_v<T, Instance::GlobalInstance> ||
    std::is_same_v<T, Instance::MemoryInstance> ||
    std::is_same_v<T, Instance::TableInstance>;

/// Return true if T is instances.
template <typename T>
inline constexpr const bool IsInstanceV =
    IsEntityV<T> || std::is_same_v<T, Instance::ModuleInstance>;
} // namespace

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
  inline ErrCode
  insertModuleInst(std::unique_ptr<Instance::ModuleInstance> &Mod,
                   unsigned int &NewId) {
    return insertInstance(Mod, ModInsts, NewId);
  }

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
  inline ErrCode
  insertFunctionInst(std::unique_ptr<Instance::FunctionInstance> &Func,
                     unsigned int &NewId) {
    return insertInstance(Func, FuncInsts, NewId);
  }

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
  inline ErrCode insertTableInst(std::unique_ptr<Instance::TableInstance> &Tab,
                                 unsigned int &NewId) {
    return insertInstance(Tab, TabInsts, NewId);
  }

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
  inline ErrCode
  insertMemoryInst(std::unique_ptr<Instance::MemoryInstance> &Mem,
                   unsigned int &NewId) {
    return insertInstance(Mem, MemInsts, NewId);
  }

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
  inline ErrCode
  insertGlobalInst(std::unique_ptr<Instance::GlobalInstance> &Glob,
                   unsigned int &NewId) {
    return insertInstance(Glob, GlobInsts, NewId);
  }

  /// Get instance from store manager.
  ///
  /// Get the module instance by supplying module address.
  ///
  /// \param Addr the module address in Store.
  /// \param [out] Mod the module instance.
  ///
  /// \returns ErrCode.
  inline ErrCode getModule(const unsigned int Addr,
                           Instance::ModuleInstance *&Mod) {
    return getInstance(Addr, ModInsts, Mod);
  }

  /// Get instance from store manager.
  ///
  /// Get the function instance by supplying function address.
  ///
  /// \param Addr the function address in Store.
  /// \param [out] Func the function instance.
  ///
  /// \returns ErrCode.
  inline ErrCode getFunction(const unsigned int Addr,
                             Instance::FunctionInstance *&Func) {
    return getInstance(Addr, FuncInsts, Func);
  }

  /// Get instance from store manager.
  ///
  /// Get the table instance by supplying table address.
  ///
  /// \param Addr the table address in Store.
  /// \param [out] Tab the table instance.
  ///
  /// \returns ErrCode.
  inline ErrCode getTable(const unsigned int Addr,
                          Instance::TableInstance *&Tab) {
    return getInstance(Addr, TabInsts, Tab);
  }

  /// Get instance from store manager.
  ///
  /// Get the memory instance by supplying memory address.
  ///
  /// \param Addr the memory address in Store.
  /// \param [out] Mem the memory instance.
  ///
  /// \returns ErrCode.
  inline ErrCode getMemory(const unsigned int Addr,
                           Instance::MemoryInstance *&Mem) {
    return getInstance(Addr, MemInsts, Mem);
  }

  /// Get instance from store manager.
  ///
  /// Get the global instance by supplying global address.
  ///
  /// \param Addr the global address in Store.
  /// \param [out] Glob the global instance.
  ///
  /// \returns ErrCode.
  inline ErrCode getGlobal(const unsigned int Addr,
                           Instance::GlobalInstance *&Glob) {
    return getInstance(Addr, GlobInsts, Glob);
  }

  /// Find function from store manager.
  ///
  /// Get the function instance by mapping function name.
  ///
  /// \param ModName the module name of function.
  /// \param FuncName the function name.
  /// \param [out] Func the function instance.
  ///
  /// \returns ErrCode.
  inline ErrCode findFunction(const std::string &ModName,
                              const std::string &FuncName,
                              Instance::FunctionInstance *&Func) {
    return findEntity(ModName, FuncName, FuncInsts, Func);
  }

  /// Find table from store manager.
  ///
  /// Get the table instance by mapping table name.
  ///
  /// \param ModName the module name of table.
  /// \param TabName the table name.
  /// \param [out] Tab the table instance.
  ///
  /// \returns ErrCode.
  inline ErrCode findTable(const std::string &ModName,
                           const std::string &TabName,
                           Instance::TableInstance *&Tab) {
    return findEntity(ModName, TabName, TabInsts, Tab);
  }

  /// Find memory from store manager.
  ///
  /// Get the memory instance by mapping memory name.
  ///
  /// \param ModName the module name of memory.
  /// \param MemName the memory name.
  /// \param [out] Mem the memory instance.
  ///
  /// \returns ErrCode.
  inline ErrCode findMemory(const std::string &ModName,
                            const std::string &MemName,
                            Instance::MemoryInstance *&Mem) {
    return findEntity(ModName, MemName, MemInsts, Mem);
  }

  /// Find global from store manager.
  ///
  /// Get the global instance by mapping global name.
  ///
  /// \param ModName the module name of global.
  /// \param GlobName the global name.
  /// \param [out] Glob the global instance.
  ///
  /// \returns ErrCode.
  inline ErrCode findGlobal(const std::string &ModName,
                            const std::string &GlobName,
                            Instance::GlobalInstance *&Glob) {
    return findEntity(ModName, GlobName, GlobInsts, Glob);
  }

private:
  /// Helper function for inserting instance to instance vector.
  template <typename T>
  std::enable_if_t<IsInstanceV<T>, ErrCode>
  insertInstance(std::unique_ptr<T> &Inst,
                 std::vector<std::unique_ptr<T>> &InstsVec,
                 unsigned int &NewId);

  /// Helper function for getting instance from instance vector.
  template <typename T>
  std::enable_if_t<IsInstanceV<T>, ErrCode>
  getInstance(const unsigned int Addr,
              std::vector<std::unique_ptr<T>> &InstsVec, T *&Inst);

  /// Helper function for finding instance from instance vector.
  template <typename T>
  std::enable_if_t<IsEntityV<T>, ErrCode>
  findEntity(const std::string &ModName, const std::string &EntityName,
             std::vector<std::unique_ptr<T>> &InstsVec, T *&Inst);

  /// \name Data of instances.
  /// @{
  std::vector<std::unique_ptr<Instance::ModuleInstance>> ModInsts;
  std::vector<std::unique_ptr<Instance::FunctionInstance>> FuncInsts;
  std::vector<std::unique_ptr<Instance::TableInstance>> TabInsts;
  std::vector<std::unique_ptr<Instance::MemoryInstance>> MemInsts;
  std::vector<std::unique_ptr<Instance::GlobalInstance>> GlobInsts;
  /// @}
};

} // namespace Executor
} // namespace SSVM

#include "storemgr.ipp"