// SPDX-License-Identifier: Apache-2.0
//===-- wasmedge/runtime/storemgr.h - Store Manager definition ------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definition of Store Manager.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "runtime/instance/data.h"
#include "runtime/instance/elem.h"
#include "runtime/instance/function.h"
#include "runtime/instance/global.h"
#include "runtime/instance/memory.h"
#include "runtime/instance/module.h"
#include "runtime/instance/table.h"

#include <memory>
#include <type_traits>
#include <vector>

namespace WasmEdge {
namespace Runtime {

namespace {
/// Return true if T is entities which can be imported.
template <typename T>
inline constexpr const bool IsImportEntityV =
    std::is_same_v<T, Instance::FunctionInstance> ||
    std::is_same_v<T, Instance::TableInstance> ||
    std::is_same_v<T, Instance::MemoryInstance> ||
    std::is_same_v<T, Instance::GlobalInstance>;

/// Return true if T is entities.
template <typename T>
inline constexpr const bool IsEntityV =
    IsImportEntityV<T> || std::is_same_v<T, Instance::ElementInstance> ||
    std::is_same_v<T, Instance::DataInstance>;

/// Return true if T is instances.
template <typename T>
inline constexpr const bool IsInstanceV =
    IsEntityV<T> || std::is_same_v<T, Instance::ModuleInstance>;
} // namespace

class StoreManager {
public:
  StoreManager()
      : NumMod(0), NumFunc(0), NumTab(0), NumMem(0), NumGlob(0), NumElem(0),
        NumData(0) {}
  ~StoreManager() = default;

  /// Import instances and move owner to store manager.
  template <typename... Args> uint32_t importModule(Args &&...Values) {
    uint32_t ModAddr =
        importInstance(ImpModInsts, ModInsts, std::forward<Args>(Values)...);
    ModInsts.back()->Addr = ModAddr;
    return ModAddr;
  }
  template <typename... Args> uint32_t importFunction(Args &&...Values) {
    return importInstance(ImpFuncInsts, FuncInsts,
                          std::forward<Args>(Values)...);
  }
  template <typename... Args> uint32_t importTable(Args &&...Values) {
    return importInstance(ImpTabInsts, TabInsts, std::forward<Args>(Values)...);
  }
  template <typename... Args> uint32_t importMemory(Args &&...Values) {
    return importInstance(ImpMemInsts, MemInsts, std::forward<Args>(Values)...);
  }
  template <typename... Args> uint32_t importGlobal(Args &&...Values) {
    return importInstance(ImpGlobInsts, GlobInsts,
                          std::forward<Args>(Values)...);
  }
  template <typename... Args> uint32_t importElement(Args &&...Values) {
    return importInstance(ImpElemInsts, ElemInsts,
                          std::forward<Args>(Values)...);
  }
  template <typename... Args> uint32_t importData(Args &&...Values) {
    return importInstance(ImpDataInsts, DataInsts,
                          std::forward<Args>(Values)...);
  }

  /// Import host instances but not move ownership.
  uint32_t importHostFunction(Instance::FunctionInstance &Func) {
    return importHostInstance(Func, FuncInsts);
  }
  uint32_t importHostTable(Instance::TableInstance &Tab) {
    return importHostInstance(Tab, TabInsts);
  }
  uint32_t importHostMemory(Instance::MemoryInstance &Mem) {
    return importHostInstance(Mem, MemInsts);
  }
  uint32_t importHostGlobal(Instance::GlobalInstance &Glob) {
    return importHostInstance(Glob, GlobInsts);
  }

  /// Insert instances for instantiation and move ownership to store manager.
  template <typename... Args> uint32_t pushModule(Args &&...Values) {
    ++NumMod;
    uint32_t ModAddr =
        importInstance(ImpModInsts, ModInsts, std::forward<Args>(Values)...);
    ModInsts.back()->Addr = ModAddr;
    return ModAddr;
  }
  template <typename... Args> uint32_t pushFunction(Args &&...Values) {
    ++NumFunc;
    return importInstance(ImpFuncInsts, FuncInsts,
                          std::forward<Args>(Values)...);
  }
  template <typename... Args> uint32_t pushTable(Args &&...Values) {
    ++NumTab;
    return importInstance(ImpTabInsts, TabInsts, std::forward<Args>(Values)...);
  }
  template <typename... Args> uint32_t pushMemory(Args &&...Values) {
    ++NumMem;
    return importInstance(ImpMemInsts, MemInsts, std::forward<Args>(Values)...);
  }
  template <typename... Args> uint32_t pushGlobal(Args &&...Values) {
    ++NumGlob;
    return importInstance(ImpGlobInsts, GlobInsts,
                          std::forward<Args>(Values)...);
  }
  template <typename... Args> uint32_t pushElement(Args &&...Values) {
    ++NumElem;
    return importInstance(ImpElemInsts, ElemInsts,
                          std::forward<Args>(Values)...);
  }
  template <typename... Args> uint32_t pushData(Args &&...Values) {
    ++NumData;
    return importInstance(ImpDataInsts, DataInsts,
                          std::forward<Args>(Values)...);
  }

  /// Pop temp. module. Dangerous function for used when instantiating only.
  void popModule() {
    if (NumMod > 0) {
      --NumMod;
      ImpModInsts.pop_back();
      ModInsts.pop_back();
    }
  }

  /// Get instance from store manager by address.
  Expect<Instance::ModuleInstance *> getModule(const uint32_t Addr) {
    return getInstance(Addr, ModInsts);
  }
  Expect<Instance::FunctionInstance *> getFunction(const uint32_t Addr) {
    return getInstance(Addr, FuncInsts);
  }
  Expect<Instance::TableInstance *> getTable(const uint32_t Addr) {
    return getInstance(Addr, TabInsts);
  }
  Expect<Instance::MemoryInstance *> getMemory(const uint32_t Addr) {
    return getInstance(Addr, MemInsts);
  }
  Expect<Instance::GlobalInstance *> getGlobal(const uint32_t Addr) {
    return getInstance(Addr, GlobInsts);
  }
  Expect<Instance::ElementInstance *> getElement(const uint32_t Addr) {
    return getInstance(Addr, ElemInsts);
  }
  Expect<Instance::DataInstance *> getData(const uint32_t Addr) {
    return getInstance(Addr, DataInsts);
  }

  /// Get exported instances of instantiated module.
  const std::map<std::string, uint32_t, std::less<>> getFuncExports() const {
    if (NumMod > 0) {
      return ModInsts.back()->getFuncExports();
    }
    return {};
  }
  const std::map<std::string, uint32_t, std::less<>> getTableExports() const {
    if (NumMod > 0) {
      return ModInsts.back()->getTableExports();
    }
    return {};
  }
  const std::map<std::string, uint32_t, std::less<>> getMemExports() const {
    if (NumMod > 0) {
      return ModInsts.back()->getMemExports();
    }
    return {};
  }
  const std::map<std::string, uint32_t, std::less<>> getGlobalExports() const {
    if (NumMod > 0) {
      return ModInsts.back()->getGlobalExports();
    }
    return {};
  }

  /// Get list of registered modules.
  const std::map<std::string, uint32_t, std::less<>> getModuleList() const {
    std::map<std::string, uint32_t, std::less<>> ModMap;
    for (uint32_t I = 0; I < ModInsts.size() - NumMod; I++) {
      ModMap.emplace(ModInsts[I]->getModuleName(), I);
    }
    return ModMap;
  }

  /// Get active instance of instantiated module.
  Expect<Instance::ModuleInstance *> getActiveModule() const {
    if (NumMod > 0) {
      return ModInsts.back();
    }
    /// Error logging need to be handled in caller.
    return Unexpect(ErrCode::WrongInstanceAddress);
  }

  /// Find module by name.
  Expect<Instance::ModuleInstance *> findModule(std::string_view Name) const {
    for (uint32_t I = 0; I < ModInsts.size() - NumMod; I++) {
      if (ModInsts[I]->getModuleName() == Name) {
        return ModInsts[I];
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
      ModInsts.clear();
      FuncInsts.clear();
      TabInsts.clear();
      MemInsts.clear();
      GlobInsts.clear();
      ElemInsts.clear();
      DataInsts.clear();
      ImpModInsts.clear();
      ImpFuncInsts.clear();
      ImpTabInsts.clear();
      ImpMemInsts.clear();
      ImpGlobInsts.clear();
      ImpElemInsts.clear();
      ImpDataInsts.clear();
    } else {
      while (NumMod > 0) {
        --NumMod;
        ImpModInsts.pop_back();
        ModInsts.pop_back();
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
        ElemInsts.pop_back();
      }
      while (NumData > 0) {
        --NumData;
        ImpDataInsts.pop_back();
        DataInsts.pop_back();
      }
    }
  }

private:
  /// Helper function for importing instances and move ownership.
  template <typename T, typename... Args>
  std::enable_if_t<IsInstanceV<T>, uint32_t>
  importInstance(std::vector<std::unique_ptr<T>> &ImpInstsVec,
                 std::vector<T *> &InstsVec, Args &&...Values) {
    const auto Addr = static_cast<uint32_t>(InstsVec.size());
    ImpInstsVec.push_back(std::make_unique<T>(std::forward<Args>(Values)...));
    InstsVec.push_back(ImpInstsVec.back().get());
    return Addr;
  }

  /// Helper function for importing host instances.
  template <typename T>
  std::enable_if_t<IsImportEntityV<T>, uint32_t>
  importHostInstance(T &Inst, std::vector<T *> &InstsVec) {
    const auto Addr = static_cast<uint32_t>(InstsVec.size());
    InstsVec.push_back(&Inst);
    return Addr;
  }

  /// Helper function for getting instance from instance vector.
  template <typename T>
  std::enable_if_t<IsInstanceV<T>, Expect<T *>>
  getInstance(const uint32_t Addr, const std::vector<T *> &InstsVec) {
    if (Addr >= static_cast<uint32_t>(InstsVec.size())) {
      /// Error logging need to be handled in caller.
      return Unexpect(ErrCode::WrongInstanceAddress);
    }
    return InstsVec[Addr];
  }

  /// \name Store owned instances by StoreManager.
  /// @{
  std::vector<std::unique_ptr<Instance::ModuleInstance>> ImpModInsts;
  std::vector<std::unique_ptr<Instance::FunctionInstance>> ImpFuncInsts;
  std::vector<std::unique_ptr<Instance::TableInstance>> ImpTabInsts;
  std::vector<std::unique_ptr<Instance::MemoryInstance>> ImpMemInsts;
  std::vector<std::unique_ptr<Instance::GlobalInstance>> ImpGlobInsts;
  std::vector<std::unique_ptr<Instance::ElementInstance>> ImpElemInsts;
  std::vector<std::unique_ptr<Instance::DataInstance>> ImpDataInsts;
  /// @}

  /// \name Pointers to imported instances from modules or import objects.
  /// @{
  std::vector<Instance::ModuleInstance *> ModInsts;
  std::vector<Instance::FunctionInstance *> FuncInsts;
  std::vector<Instance::TableInstance *> TabInsts;
  std::vector<Instance::MemoryInstance *> MemInsts;
  std::vector<Instance::GlobalInstance *> GlobInsts;
  std::vector<Instance::ElementInstance *> ElemInsts;
  std::vector<Instance::DataInstance *> DataInsts;
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
} // namespace WasmEdge
