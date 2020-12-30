// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/runtime/storemgr.h - Store Manager definition ----------------===//
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

#include "instance/data.h"
#include "instance/elem.h"
#include "instance/function.h"
#include "instance/global.h"
#include "instance/memory.h"
#include "instance/module.h"
#include "instance/table.h"

#include <memory>
#include <type_traits>
#include <vector>

namespace SSVM {
namespace Runtime {

namespace {
/// Return true if T is entities which can be imported.
template <typename T>
inline constexpr const bool IsImportEntityV =
    std::is_same_v<T, Instance::FunctionInstance> ||
    std::is_same_v<T, Instance::TableInstance> ||
    std::is_same_v<T, Instance::MemoryInstance> ||
    std::is_same_v<T, Instance::GlobalInstance>;

/// Return true if T is entities which can not be imported.
template <typename T>
inline constexpr const bool IsNotImportEntityV =
    std::is_same_v<T, Instance::ElementInstance> ||
    std::is_same_v<T, Instance::DataInstance> ||
    std::is_same_v<T, Instance::ModuleInstance>;
} // namespace

class StoreManager {
public:
  StoreManager()
      : NumMod(0), NumFunc(0), NumTab(0), NumMem(0), NumGlob(0), NumElem(0),
        NumData(0) {}
  ~StoreManager() = default;

  /// Import instances and move owner to store manager.
  template <typename... Args> uint32_t importModule(Args &&... Values) {
    uint32_t ModAddr =
        importInstance(ImpModInsts, std::forward<Args>(Values)...);
    ImpModInsts.back().Addr = ModAddr;
    return ModAddr;
  }
  template <typename... Args> uint32_t importFunction(Args &&... Values) {
    return importInstance(ImpFuncInsts, FuncInsts,
                          std::forward<Args>(Values)...);
  }
  template <typename... Args> uint32_t importTable(Args &&... Values) {
    return importInstance(ImpTabInsts, TabInsts, std::forward<Args>(Values)...);
  }
  template <typename... Args> uint32_t importMemory(Args &&... Values) {
    return importInstance(ImpMemInsts, MemInsts, std::forward<Args>(Values)...);
  }
  template <typename... Args> uint32_t importGlobal(Args &&... Values) {
    return importInstance(ImpGlobInsts, GlobInsts,
                          std::forward<Args>(Values)...);
  }
  template <typename... Args> uint32_t importElement(Args &&... Values) {
    return importInstance(ImpElemInsts, std::forward<Args>(Values)...);
  }
  template <typename... Args> uint32_t importData(Args &&... Values) {
    return importInstance(ImpDataInsts, std::forward<Args>(Values)...);
  }

  /// Import host instances but not move ownership.
  uint32_t importHostFunction(Instance::FunctionInstance &Func) {
    return importHostInstance(Func, HostFuncInsts, FuncInsts);
  }
  uint32_t importHostTable(Instance::TableInstance &Tab) {
    return importHostInstance(Tab, HostTabInsts, TabInsts);
  }
  uint32_t importHostMemory(Instance::MemoryInstance &Mem) {
    return importHostInstance(Mem, HostMemInsts, MemInsts);
  }
  uint32_t importHostGlobal(Instance::GlobalInstance &Glob) {
    return importHostInstance(Glob, HostGlobInsts, GlobInsts);
  }

  /// Insert instances for instantiation and move ownership to store manager.
  template <typename... Args> uint32_t pushModule(Args &&... Values) {
    ++NumMod;
    uint32_t ModAddr =
        importInstance(ImpModInsts, std::forward<Args>(Values)...);
    ImpModInsts.back().Addr = ModAddr;
    return ModAddr;
  }
  template <typename... Args> uint32_t pushFunction(Args &&... Values) {
    ++NumFunc;
    return importInstance(ImpFuncInsts, FuncInsts,
                          std::forward<Args>(Values)...);
  }
  template <typename... Args> uint32_t pushTable(Args &&... Values) {
    ++NumTab;
    return importInstance(ImpTabInsts, TabInsts, std::forward<Args>(Values)...);
  }
  template <typename... Args> uint32_t pushMemory(Args &&... Values) {
    ++NumMem;
    return importInstance(ImpMemInsts, MemInsts, std::forward<Args>(Values)...);
  }
  template <typename... Args> uint32_t pushGlobal(Args &&... Values) {
    ++NumGlob;
    return importInstance(ImpGlobInsts, GlobInsts,
                          std::forward<Args>(Values)...);
  }
  template <typename... Args> uint32_t pushElement(Args &&... Values) {
    ++NumElem;
    return importInstance(ImpElemInsts, std::forward<Args>(Values)...);
  }
  template <typename... Args> uint32_t pushData(Args &&... Values) {
    ++NumData;
    return importInstance(ImpDataInsts, std::forward<Args>(Values)...);
  }

  /// Pop temp. module. Dangerous function for used when instantiating only.
  void popModule() {
    if (NumMod > 0) {
      --NumMod;
      ImpModInsts.pop_back();
    }
  }

  /// Get instance from store manager by address.
  Expect<Instance::ModuleInstance *> getModule(const uint32_t Addr) {
    return getInstance(Addr, ImpModInsts);
  }
  Expect<Instance::FunctionInstance *> getFunction(const uint32_t Addr) {
    return getInstance(Addr, ImpFuncInsts, HostFuncInsts, FuncInsts);
  }
  Expect<Instance::TableInstance *> getTable(const uint32_t Addr) {
    return getInstance(Addr, ImpTabInsts, HostTabInsts, TabInsts);
  }
  Expect<Instance::MemoryInstance *> getMemory(const uint32_t Addr) {
    return getInstance(Addr, ImpMemInsts, HostMemInsts, MemInsts);
  }
  Expect<Instance::GlobalInstance *> getGlobal(const uint32_t Addr) {
    return getInstance(Addr, ImpGlobInsts, HostGlobInsts, GlobInsts);
  }
  Expect<Instance::ElementInstance *> getElement(const uint32_t Addr) {
    return getInstance(Addr, ImpElemInsts);
  }
  Expect<Instance::DataInstance *> getData(const uint32_t Addr) {
    return getInstance(Addr, ImpDataInsts);
  }

  /// Get exported instances of instantiated module.
  const std::map<std::string, uint32_t, std::less<>> getFuncExports() const {
    if (NumMod > 0) {
      return ImpModInsts.back().getFuncExports();
    }
    return {};
  }
  const std::map<std::string, uint32_t, std::less<>> getTableExports() const {
    if (NumMod > 0) {
      return ImpModInsts.back().getTableExports();
    }
    return {};
  }
  const std::map<std::string, uint32_t, std::less<>> getMemExports() const {
    if (NumMod > 0) {
      return ImpModInsts.back().getMemExports();
    }
    return {};
  }
  const std::map<std::string, uint32_t, std::less<>> getGlobalExports() const {
    if (NumMod > 0) {
      return ImpModInsts.back().getGlobalExports();
    }
    return {};
  }

  /// Get list of registered modules.
  const std::map<std::string, uint32_t, std::less<>> getModuleList() const {
    std::map<std::string, uint32_t, std::less<>> ModMap;
    for (uint32_t I = 0; I < ImpModInsts.size() - NumMod; I++) {
      ModMap.emplace(ImpModInsts[I].getModuleName(), I);
    }
    return ModMap;
  }

  /// Get active instance of instantiated module.
  Expect<Instance::ModuleInstance *> getActiveModule() {
    if (NumMod > 0) {
      return &(ImpModInsts.back());
    }
    /// Error logging need to be handled in caller.
    return Unexpect(ErrCode::WrongInstanceAddress);
  }

  /// Find module by name.
  Expect<Instance::ModuleInstance *> findModule(std::string_view Name) {
    for (uint32_t I = 0; I < ImpModInsts.size() - NumMod; I++) {
      if (ImpModInsts[I].getModuleName() == Name) {
        return &ImpModInsts[I];
      }
    }
    /// Error logging need to be handled in caller.
    return Unexpect(ErrCode::WrongInstanceAddress);
  }

  /// Reset store.
  void reset(bool IsResetRegistered = false) {
    if (IsResetRegistered) {
      NumMod = 0;
      NumFunc = 0;
      NumTab = 0;
      NumMem = 0;
      NumGlob = 0;
      NumElem = 0;
      NumData = 0;
      ImpModInsts.clear();
      ImpFuncInsts.clear();
      ImpTabInsts.clear();
      ImpMemInsts.clear();
      ImpGlobInsts.clear();
      ImpElemInsts.clear();
      ImpDataInsts.clear();
      HostFuncInsts.clear();
      HostTabInsts.clear();
      HostMemInsts.clear();
      HostGlobInsts.clear();
      FuncInsts.clear();
      TabInsts.clear();
      MemInsts.clear();
      GlobInsts.clear();
    } else {
      while (NumMod > 0) {
        --NumMod;
        ImpModInsts.pop_back();
      }
      while (NumFunc > 0) {
        --NumFunc;
        ImpFuncInsts.pop_back();
        FuncInsts.pop_back();
      }
      while (NumTab > 0) {
        --NumTab;
        ImpTabInsts.pop_back();
        TabInsts.pop_back();
      }
      while (NumMem > 0) {
        --NumMem;
        ImpMemInsts.pop_back();
        MemInsts.pop_back();
      }
      while (NumGlob > 0) {
        --NumGlob;
        ImpGlobInsts.pop_back();
        GlobInsts.pop_back();
      }
      while (NumElem > 0) {
        --NumElem;
        ImpElemInsts.pop_back();
      }
      while (NumData > 0) {
        --NumData;
        ImpDataInsts.pop_back();
      }
    }
  }

private:
  /// Helper function for importing instances and move ownership.
  template <typename T, typename... Args>
  std::enable_if_t<IsImportEntityV<T>, uint32_t>
  importInstance(std::vector<T> &ImpInstsVec,
                 std::vector<std::pair<bool, uint32_t>> &InstsIdx,
                 Args &&... Values) {
    uint32_t Addr = InstsIdx.size();
    InstsIdx.emplace_back(true, ImpInstsVec.size());
    ImpInstsVec.emplace_back(std::forward<Args>(Values)...);
    return Addr;
  }
  template <typename T, typename... Args>
  std::enable_if_t<IsNotImportEntityV<T>, uint32_t>
  importInstance(std::vector<T> &ImpInstsVec, Args &&... Values) {
    uint32_t Addr = ImpInstsVec.size();
    ImpInstsVec.emplace_back(std::forward<Args>(Values)...);
    return Addr;
  }

  /// Helper function for importing host instances.
  template <typename T>
  std::enable_if_t<IsImportEntityV<T>, uint32_t>
  importHostInstance(T &Inst, std::vector<T *> &InstsVec,
                     std::vector<std::pair<bool, uint32_t>> &InstsIdx) {
    uint32_t Addr = InstsIdx.size();
    InstsIdx.emplace_back(false, InstsVec.size());
    InstsVec.push_back(&Inst);
    return Addr;
  }

  /// Helper function for getting instance from instance vector.
  template <typename T>
  std::enable_if_t<IsImportEntityV<T>, Expect<T *>>
  getInstance(const uint32_t Addr, std::vector<T> &ImpInstsVec,
              std::vector<T *> &HostInstsVec,
              std::vector<std::pair<bool, uint32_t>> &InstsIdx) {
    if (Addr >= InstsIdx.size()) {
      /// Error logging need to be handled in caller.
      return Unexpect(ErrCode::WrongInstanceAddress);
    }
    if (InstsIdx[Addr].first) {
      return &ImpInstsVec[InstsIdx[Addr].second];
    } else {
      return HostInstsVec[InstsIdx[Addr].second];
    }
  }
  template <typename T>
  std::enable_if_t<IsNotImportEntityV<T>, Expect<T *>>
  getInstance(const uint32_t Addr, std::vector<T> &ImpInstsVec) {
    if (Addr >= ImpInstsVec.size()) {
      /// Error logging need to be handled in caller.
      return Unexpect(ErrCode::WrongInstanceAddress);
    }
    return &ImpInstsVec[Addr];
  }

  /// \name Store owned instances by StoreManager.
  /// @{
  std::vector<Instance::ModuleInstance> ImpModInsts;
  std::vector<Instance::FunctionInstance> ImpFuncInsts;
  std::vector<Instance::TableInstance> ImpTabInsts;
  std::vector<Instance::MemoryInstance> ImpMemInsts;
  std::vector<Instance::GlobalInstance> ImpGlobInsts;
  std::vector<Instance::ElementInstance> ImpElemInsts;
  std::vector<Instance::DataInstance> ImpDataInsts;
  /// @}

  /// \name Pointers to imported instances from host objects.
  /// @{
  std::vector<Instance::FunctionInstance *> HostFuncInsts;
  std::vector<Instance::TableInstance *> HostTabInsts;
  std::vector<Instance::MemoryInstance *> HostMemInsts;
  std::vector<Instance::GlobalInstance *> HostGlobInsts;
  /// @}

  /// \name Indices for instances which may be host or owned.
  /// @{
  /// First element: true means owned instance, false means imported instance.
  /// Second element: index in the corresponding vector.
  std::vector<std::pair<bool, uint32_t>> FuncInsts;
  std::vector<std::pair<bool, uint32_t>> TabInsts;
  std::vector<std::pair<bool, uint32_t>> MemInsts;
  std::vector<std::pair<bool, uint32_t>> GlobInsts;
  /// @}

  /// \name Data for instantiated module.
  /// @{
  uint32_t NumMod;
  uint32_t NumFunc;
  uint32_t NumTab;
  uint32_t NumMem;
  uint32_t NumGlob;
  uint32_t NumElem;
  uint32_t NumData;
  /// @}
};

} // namespace Runtime
} // namespace SSVM
